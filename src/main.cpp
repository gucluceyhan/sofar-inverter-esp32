#include <Arduino.h>
#include "config.h"
#include "modbus_rtu.h"
#include "sofar_inverter.h"

ModbusRTU modbus;
SofarInverter inverter(modbus, MODBUS_SLAVE_ID);

// Otomatik okuma kontrolu
bool autoPollEnabled = false;
unsigned long lastPollTime = 0;

// Tarama kontrol
volatile bool scanAbort = false;

// Serial komut buffer
char cmdBuffer[256];
uint8_t cmdIndex = 0;

// --- Yardim Menusunu Yazdir ---

void printHelp() {
    DEBUG_SERIAL.println("\n============ SOFAR INVERTOR - KOMUT MENUSU ============");
    DEBUG_SERIAL.println("  all          - Tum verileri oku ve yazdir");
    DEBUG_SERIAL.println("  sys          - Sistem bilgisi");
    DEBUG_SERIAL.println("  grid         - Sebeke cikis verileri");
    DEBUG_SERIAL.println("  pv           - PV giris verileri");
    DEBUG_SERIAL.println("  bat          - Batarya bilgileri");
    DEBUG_SERIAL.println("  energy       - Enerji istatistikleri");
    DEBUG_SERIAL.println("  read XXXX    - Tek register oku (hex adres, orn: read 0404)");
    DEBUG_SERIAL.println("  readn XXXX N - N adet register oku (orn: readn 0484 10)");
    DEBUG_SERIAL.println("  write XXXX YYYY - Tek register yaz FC 0x06 (hex)");
    DEBUG_SERIAL.println("  writem XXXX V1 V2.. - Coklu register yaz FC 0x10 (hex)");
    DEBUG_SERIAL.println("  scan         - Slave ID tara (1-247)");
    DEBUG_SERIAL.println("  auto         - Otomatik okuma ac/kapat");
    DEBUG_SERIAL.println("  slave N      - Slave ID degistir (orn: slave 1)");
    DEBUG_SERIAL.println("  --- REGISTER TARAMA ---");
    DEBUG_SERIAL.println("  fullscan         - Tam register taramasi (0x0000-0x1FFF)");
    DEBUG_SERIAL.println("  scanrange XXXX YYYY - Belirli aralik tara (hex)");
    DEBUG_SERIAL.println("  scanwrite XXXX YYYY - Yazilabilirlik testi (hex aralik)");
    DEBUG_SERIAL.println("  --- SCHEDULE KOMUTLARI ---");
    DEBUG_SERIAL.println("  slist        - Tum schedule kurallarini goster");
    DEBUG_SERIAL.println("  smode N      - Enerji depolama modu (0-6)");
    DEBUG_SERIAL.println("  sadd I M HHMM HHMM SOC WATT - Time-sharing kural ekle");
    DEBUG_SERIAL.println("                 I=index(0-255) M=mod(0-6) SOC=%(10-100)");
    DEBUG_SERIAL.println("  sdel I       - Time-sharing kurali devre disi birak");
    DEBUG_SERIAL.println("  tadd I HHMM HHMM HHMM HHMM CW DW - Zamanli sarj kural ekle");
    DEBUG_SERIAL.println("                 I=index(0-3) sarjBas sarjBit desarjBas desarjBit CW=sarjW DW=desarjW");
    DEBUG_SERIAL.println("  tdel I       - Zamanli sarj kurali devre disi birak");
    DEBUG_SERIAL.println("  help         - Bu menuyu goster");
    DEBUG_SERIAL.println("=======================================================\n");
}

// --- Hex String -> U16 Donustur ---

uint16_t parseHex(const char* str) {
    return (uint16_t)strtol(str, NULL, 16);
}

uint16_t parseDec(const char* str) {
    return (uint16_t)atoi(str);
}

// --- Slave ID Tarama ---

void scanSlaveIds() {
    DEBUG_SERIAL.println("\n[SCAN] Modbus slave ID taramasi basliyor (1-247)...");
    int found = 0;
    uint16_t val;

    for (uint8_t id = 1; id <= 247; id++) {
        ModbusError err = modbus.readHoldingRegisters(id, 0x0404, 1, &val);
        if (err == MB_OK) {
            DEBUG_SERIAL.printf("[SCAN] Slave ID %d BULUNDU! (deger: 0x%04X)\n", id, val);
            found++;
        }
        if (id % 50 == 0) {
            DEBUG_SERIAL.printf("[SCAN] %d/%d tarand...\n", id, 247);
        }
        delay(50);
    }

    if (found == 0) {
        DEBUG_SERIAL.println("[SCAN] Hicbir cihaz bulunamadi!");
        DEBUG_SERIAL.println("  - Kablo baglantilarini kontrol edin (A+/B-)");
        DEBUG_SERIAL.println("  - RS485 modulu yon kontrolunu kontrol edin");
        DEBUG_SERIAL.println("  - Invertor Modbus ayarlarini kontrol edin");
    } else {
        DEBUG_SERIAL.printf("[SCAN] Toplam %d cihaz bulundu.\n", found);
    }
}

// --- Tek Register Oku ve Yazdir ---

void cmdReadRegister(const char* addrStr) {
    uint16_t addr = parseHex(addrStr);
    uint16_t value;

    DEBUG_SERIAL.printf("\n[READ] Register 0x%04X okunuyor...\n", addr);
    if (inverter.readRegister(addr, value)) {
        DEBUG_SERIAL.printf("[READ] 0x%04X = 0x%04X (dec: %d, signed: %d)\n",
                            addr, value, value, (int16_t)value);
    } else {
        DEBUG_SERIAL.printf("[READ] 0x%04X okunamadi!\n", addr);
    }
}

// --- Coklu Register Oku ve Yazdir ---

void cmdReadRegisters(const char* addrStr, const char* countStr) {
    uint16_t addr = parseHex(addrStr);
    uint16_t count = parseDec(countStr);

    if (count == 0 || count > MODBUS_MAX_REGS) {
        DEBUG_SERIAL.printf("[READ] Gecersiz sayi: %d (1-%d arasi olmali)\n",
                            count, MODBUS_MAX_REGS);
        return;
    }

    uint16_t buffer[MODBUS_MAX_REGS];
    DEBUG_SERIAL.printf("\n[READ] Register 0x%04X - 0x%04X (%d adet) okunuyor...\n",
                        addr, addr + count - 1, count);

    if (inverter.readRegisters(addr, count, buffer)) {
        for (uint16_t i = 0; i < count; i++) {
            DEBUG_SERIAL.printf("  0x%04X = 0x%04X  (dec: %5d  signed: %6d)\n",
                                addr + i, buffer[i], buffer[i], (int16_t)buffer[i]);
        }
    } else {
        DEBUG_SERIAL.println("[READ] Okuma basarisiz!");
    }
}

// --- Register Yaz ---

void cmdWriteRegister(const char* addrStr, const char* valStr) {
    uint16_t addr = parseHex(addrStr);
    uint16_t value = parseHex(valStr);

    DEBUG_SERIAL.printf("\n[WRITE] Register 0x%04X <- 0x%04X yaziliyor...\n", addr, value);

    // Guvenlik uyarisi
    DEBUG_SERIAL.println("[UYARI] Yazma islemi invertor ayarlarini degistirir!");
    DEBUG_SERIAL.println("[UYARI] Sadece RW (Read/Write) ozellikli registerlere yazin.");

    if (inverter.writeRegister(addr, value)) {
        DEBUG_SERIAL.printf("[WRITE] Basarili! 0x%04X = 0x%04X\n", addr, value);

        // Dogrulama okumasi
        uint16_t readBack;
        delay(100);
        if (inverter.readRegister(addr, readBack)) {
            DEBUG_SERIAL.printf("[VERIFY] Dogrulama: 0x%04X = 0x%04X %s\n",
                                addr, readBack,
                                readBack == value ? "(ESLESME)" : "(FARKLI!)");
        }
    } else {
        DEBUG_SERIAL.printf("[WRITE] Basarisiz! (RO register olabilir)\n");
    }
}

// --- Coklu Register Yaz (FC 0x10) ---

void cmdWriteMultipleRegisters(const char* input) {
    // Format: "writem XXXX V1 V2 V3..."
    // input zaten "writem" cikarilmis halinde degil, tum komutu parse edecegiz
    char parts[22][32] = {};
    int partCount = 0;
    const char* p = input;

    while (*p && partCount < 22) {
        while (*p == ' ') p++;
        if (!*p) break;
        int i = 0;
        while (*p && *p != ' ' && i < 31) {
            parts[partCount][i++] = *p++;
        }
        parts[partCount][i] = '\0';
        partCount++;
    }

    // parts[0] = "writem", parts[1] = addr, parts[2..] = values
    if (partCount < 3) {
        DEBUG_SERIAL.println("[WRITE] Kullanim: writem XXXX V1 V2 V3...");
        return;
    }

    uint16_t addr = parseHex(parts[1]);
    uint16_t count = partCount - 2;
    uint16_t values[20];

    if (count > 20) {
        DEBUG_SERIAL.println("[WRITE] En fazla 20 register yazilabilir!");
        return;
    }

    for (int i = 0; i < count; i++) {
        values[i] = parseHex(parts[i + 2]);
    }

    DEBUG_SERIAL.printf("\n[WRITE] FC 0x10: 0x%04X adresine %d register yaziliyor...\n", addr, count);
    DEBUG_SERIAL.println("[UYARI] Yazma islemi invertor ayarlarini degistirir!");

    for (int i = 0; i < count; i++) {
        DEBUG_SERIAL.printf("  0x%04X <- 0x%04X\n", addr + i, values[i]);
    }

    if (inverter.writeRegisters(addr, count, values)) {
        DEBUG_SERIAL.println("[WRITE] Coklu yazma basarili!");
    } else {
        DEBUG_SERIAL.println("[WRITE] Coklu yazma basarisiz!");
    }
}

// --- Enerji Depolama Modu Isimleri ---

const char* getEnergyModeName(uint16_t mode) {
    switch (mode) {
        case 0: return "Self-generation (Kendi uretim)";
        case 1: return "Time-sharing tariff (Zaman dilimli)";
        case 2: return "Timed charge/discharge (Zamanli sarj)";
        case 3: return "Passive";
        case 4: return "Peak-shaving (Tepe tiras)";
        case 5: return "Off-grid (Sebekeden bagimsiz)";
        case 6: return "Generator";
        default: return "Bilinmeyen";
    }
}

const char* getTimeSharingModeName(uint16_t mode) {
    switch (mode) {
        case 0: return "Zorla sarj";
        case 1: return "Zorla desarj";
        case 2: return "Peak-shaving";
        case 3: return "Besleme onceligi";
        case 4: return "Spontan kendi kullanim";
        case 5: return "Sarj bakim";
        case 6: return "Desarj bakim";
        default: return "Bilinmeyen";
    }
}

// --- Schedule Listele ---

void cmdScheduleList() {
    uint16_t mode;
    DEBUG_SERIAL.println("\n========== SCHEDULE DURUMU ==========");

    // Enerji depolama modu
    if (inverter.readRegister(0x1110, mode)) {
        DEBUG_SERIAL.printf("[MOD] Enerji depolama modu: %d - %s\n", mode, getEnergyModeName(mode));
    } else {
        DEBUG_SERIAL.println("[HATA] Mod okunamadi!");
    }

    // Time-Sharing kurallari (4 kural kontrol et)
    DEBUG_SERIAL.println("\n--- TIME-SHARING KURALLARI ---");
    uint16_t tsRegs[16];
    bool anyTsFound = false;

    for (int rule = 0; rule < 4; rule++) {
        // Her kural icin shadow registerlere yaz ve oku
        // Once shadow'a kural index'i yazalim ve okuyalim
        uint16_t writeRegs[16] = {};
        writeRegs[0] = rule;  // kural index
        writeRegs[15] = 0x0001;  // enable read

        // Kural okumak icin: once index yazalim, sonra okuyalim
        // Aslinda readn ile mevcut durumu okuyabiliriz
        if (inverter.readRegisters(0x1120, 16, tsRegs)) {
            // Sadece ilk okumada mevcut kurali gosteriyoruz
            if (rule == 0) {
                uint16_t idx = tsRegs[0];
                uint16_t en = tsRegs[1];
                uint16_t startH = tsRegs[2] >> 8, startM = tsRegs[2] & 0xFF;
                uint16_t endH = tsRegs[3] >> 8, endM = tsRegs[3] & 0xFF;
                uint16_t soc = tsRegs[4];
                uint32_t power = ((uint32_t)tsRegs[5] << 16) | tsRegs[6];
                uint16_t dateStart = tsRegs[7];
                uint16_t dateEnd = tsRegs[8];
                uint16_t days = tsRegs[9];
                uint16_t ctrlMode = tsRegs[10];
                uint16_t extraCfg = tsRegs[11];
                uint16_t soc2 = tsRegs[12];

                DEBUG_SERIAL.printf("  Kural %d: %s\n", idx, en ? "AKTIF" : "DEVRE DISI");
                DEBUG_SERIAL.printf("    Zaman : %02d:%02d - %02d:%02d\n", startH, startM, endH, endM);
                DEBUG_SERIAL.printf("    Mod   : %d (%s)\n", ctrlMode, getTimeSharingModeName(ctrlMode));
                DEBUG_SERIAL.printf("    SOC   : %%%d  Guc: %uW\n", soc, power);
                DEBUG_SERIAL.printf("    Tarih : %02d/%02d - %02d/%02d\n",
                    dateStart >> 8, dateStart & 0xFF, dateEnd >> 8, dateEnd & 0xFF);

                // Gunleri goster
                DEBUG_SERIAL.print("    Gunler: ");
                const char* dayNames[] = {"Pzt", "Sal", "Car", "Per", "Cum", "Cmt", "Paz"};
                for (int d = 0; d < 7; d++) {
                    if (days & (1 << d)) {
                        DEBUG_SERIAL.printf("%s ", dayNames[d]);
                    }
                }
                DEBUG_SERIAL.println();

                if (extraCfg) DEBUG_SERIAL.printf("    Ek cfg: 0x%04X\n", extraCfg);
                if (soc2) DEBUG_SERIAL.printf("    SOC2  : %%%d\n", soc2);
                DEBUG_SERIAL.printf("    Enable: 0x%04X\n", tsRegs[15]);
                anyTsFound = true;
            }
            break;
        } else {
            DEBUG_SERIAL.println("[HATA] Time-sharing registerleri okunamadi!");
            break;
        }
    }

    if (!anyTsFound) {
        DEBUG_SERIAL.println("  (Kural bulunamadi veya okunamadi)");
    }

    // Zamanli Sarj/Desarj kurallari
    DEBUG_SERIAL.println("\n--- ZAMANLI SARJ/DESARJ KURALLARI ---");
    uint16_t tcRegs[15];
    if (inverter.readRegisters(0x1111, 15, tcRegs)) {
        uint16_t idx = tcRegs[0];
        uint16_t en = tcRegs[1];
        uint16_t chStartH = tcRegs[2] >> 8, chStartM = tcRegs[2] & 0xFF;
        uint16_t chEndH = tcRegs[3] >> 8, chEndM = tcRegs[3] & 0xFF;
        uint16_t dcStartH = tcRegs[4] >> 8, dcStartM = tcRegs[4] & 0xFF;
        uint16_t dcEndH = tcRegs[5] >> 8, dcEndM = tcRegs[5] & 0xFF;
        uint32_t chPower = ((uint32_t)tcRegs[6] << 16) | tcRegs[7];
        uint32_t dcPower = ((uint32_t)tcRegs[8] << 16) | tcRegs[9];

        DEBUG_SERIAL.printf("  Kural %d: Enable=0x%04X", idx, en);
        if (en & 0x01) DEBUG_SERIAL.print(" [SARJ]");
        if (en & 0x02) DEBUG_SERIAL.print(" [DESARJ]");
        if (!en) DEBUG_SERIAL.print(" [DEVRE DISI]");
        DEBUG_SERIAL.println();

        DEBUG_SERIAL.printf("    Sarj   : %02d:%02d - %02d:%02d  Guc: %uW\n",
            chStartH, chStartM, chEndH, chEndM, chPower);
        DEBUG_SERIAL.printf("    Desarj : %02d:%02d - %02d:%02d  Guc: %uW\n",
            dcStartH, dcStartM, dcEndH, dcEndM, dcPower);
        DEBUG_SERIAL.printf("    Enable : 0x%04X\n", tcRegs[14]);
    } else {
        DEBUG_SERIAL.println("  (Okunamadi)");
    }

    DEBUG_SERIAL.println("=====================================\n");
}

// --- Enerji Depolama Modu Degistir ---

void cmdSetEnergyMode(const char* modeStr) {
    uint16_t mode = parseDec(modeStr);
    if (mode > 6) {
        DEBUG_SERIAL.println("[SMODE] Gecersiz mod! (0-6 arasi)");
        DEBUG_SERIAL.println("  0=Self-generation  1=Time-sharing  2=Timed charge");
        DEBUG_SERIAL.println("  3=Passive  4=Peak-shaving  5=Off-grid  6=Generator");
        return;
    }

    DEBUG_SERIAL.printf("\n[SMODE] Enerji depolama modu -> %d (%s)\n", mode, getEnergyModeName(mode));

    if (inverter.writeRegisters(0x1110, 1, &mode)) {
        DEBUG_SERIAL.println("[SMODE] Basarili!");
        // Dogrulama
        uint16_t readBack;
        delay(100);
        if (inverter.readRegister(0x1110, readBack)) {
            DEBUG_SERIAL.printf("[SMODE] Dogrulama: %d (%s)\n", readBack, getEnergyModeName(readBack));
        }
    } else {
        DEBUG_SERIAL.println("[SMODE] Basarisiz!");
    }
}

// --- Time-Sharing Kural Ekle ---

void cmdScheduleAdd(const char* input) {
    // Format: sadd <idx> <mode> <HHMM_start> <HHMM_end> <soc> <power_w> [days_mask]
    char parts[10][32] = {};
    int partCount = 0;
    const char* p = input;

    while (*p && partCount < 10) {
        while (*p == ' ') p++;
        if (!*p) break;
        int i = 0;
        while (*p && *p != ' ' && i < 31) {
            parts[partCount][i++] = *p++;
        }
        parts[partCount][i] = '\0';
        partCount++;
    }

    // parts[0]="sadd" [1]=idx [2]=mode [3]=start [4]=end [5]=soc [6]=power [7]=days(opsiyonel)
    if (partCount < 7) {
        DEBUG_SERIAL.println("[SADD] Kullanim: sadd <idx> <mod> <HHMM_bas> <HHMM_bit> <soc%> <watt> [gun_mask]");
        DEBUG_SERIAL.println("  idx: 0-255 (dusuk=yuksek oncelik)");
        DEBUG_SERIAL.println("  mod: 0=Sarj 1=Desarj 2=Peak 3=Besleme 4=Spontan 5=SarjBakim 6=DesarjBakim");
        DEBUG_SERIAL.println("  HHMM: saat+dakika (orn: 0100=01:00, 1830=18:30)");
        DEBUG_SERIAL.println("  soc: 10-100 (%)");
        DEBUG_SERIAL.println("  watt: hedef guc (W)");
        DEBUG_SERIAL.println("  gun_mask: 7F=tumhafta, 1F=hafta ici, 60=haftasonu (hex, varsayilan: 7F)");
        return;
    }

    uint16_t idx = parseDec(parts[1]);
    uint16_t ctrlMode = parseDec(parts[2]);
    uint16_t startTime = parseHex(parts[3]);
    uint16_t endTime = parseHex(parts[4]);
    uint16_t soc = parseDec(parts[5]);
    uint32_t power = (uint32_t)atoi(parts[6]);
    uint16_t days = (partCount >= 8) ? parseHex(parts[7]) : 0x007F;

    // Validasyonlar
    if (ctrlMode > 6) { DEBUG_SERIAL.println("[SADD] Mod 0-6 arasi olmali!"); return; }
    if (soc < 10 || soc > 100) { DEBUG_SERIAL.println("[SADD] SOC 10-100 arasi olmali!"); return; }
    if ((startTime >> 8) > 23 || (startTime & 0xFF) > 59) { DEBUG_SERIAL.println("[SADD] Baslangic saati gecersiz!"); return; }
    if ((endTime >> 8) > 23 || (endTime & 0xFF) > 59) { DEBUG_SERIAL.println("[SADD] Bitis saati gecersiz!"); return; }

    // 16 register: 0x1120-0x112F
    uint16_t regs[16] = {};
    regs[0] = idx;                      // 0x1120: Kural index
    regs[1] = 0x0001;                   // 0x1121: Enable
    regs[2] = startTime;                // 0x1122: Baslangic saati
    regs[3] = endTime;                  // 0x1123: Bitis saati
    regs[4] = soc;                      // 0x1124: Hedef SOC
    regs[5] = (uint16_t)(power >> 16);  // 0x1125: Guc high
    regs[6] = (uint16_t)(power & 0xFFFF); // 0x1126: Guc low
    regs[7] = 0x0101;                   // 0x1127: Gecerlilik bas (1 Ocak)
    regs[8] = 0x0C1F;                   // 0x1128: Gecerlilik bit (31 Aralik)
    regs[9] = days;                     // 0x1129: Gunler
    regs[10] = ctrlMode;                // 0x112A: Kontrol modu
    regs[11] = (ctrlMode == 0) ? 0x0001 : 0x0000; // 0x112B: Ek cfg (sarj modunda grid sarj izni)
    regs[12] = 0x0000;                  // 0x112C: SOC2
    regs[13] = 0x0000;                  // 0x112D: reserved
    regs[14] = 0x0000;                  // 0x112E: reserved
    regs[15] = 0x0001;                  // 0x112F: Write enable

    DEBUG_SERIAL.printf("\n[SADD] Time-Sharing Kural %d yaziliyor...\n", idx);
    DEBUG_SERIAL.printf("  Mod   : %d (%s)\n", ctrlMode, getTimeSharingModeName(ctrlMode));
    DEBUG_SERIAL.printf("  Zaman : %02d:%02d - %02d:%02d\n",
        startTime >> 8, startTime & 0xFF, endTime >> 8, endTime & 0xFF);
    DEBUG_SERIAL.printf("  SOC   : %%%d  Guc: %uW\n", soc, power);
    DEBUG_SERIAL.printf("  Gunler: 0x%04X\n", days);

    if (inverter.writeRegisters(0x1120, 16, regs)) {
        DEBUG_SERIAL.println("[SADD] Yazma basarili!");

        // Enable register dogrula
        delay(200);
        uint16_t enResult;
        if (inverter.readRegister(0x112F, enResult)) {
            DEBUG_SERIAL.printf("[SADD] Enable sonucu: 0x%04X %s\n", enResult,
                enResult == 0x0000 ? "(BASARILI)" :
                enResult == 0x0001 ? "(ISLENIYOR...)" : "(HATA!)");
        }
    } else {
        DEBUG_SERIAL.println("[SADD] Yazma basarisiz!");
    }
}

// --- Time-Sharing Kural Sil ---

void cmdScheduleDelete(const char* idxStr) {
    uint16_t idx = parseDec(idxStr);

    // Kurali devre disi birak (enable=0)
    uint16_t regs[16] = {};
    regs[0] = idx;          // Kural index
    regs[1] = 0x0000;       // Enable = DEVRE DISI
    regs[2] = 0x0000;       // Baslangic
    regs[3] = 0x0000;       // Bitis
    regs[4] = 0x0064;       // SOC 100%
    regs[5] = 0x0000;       // Guc high
    regs[6] = 0x0000;       // Guc low
    regs[7] = 0x0101;       // Tarih bas
    regs[8] = 0x0C1F;       // Tarih bit
    regs[9] = 0x0000;       // Gunler = hicbiri
    regs[10] = 0x0000;      // Mod
    regs[11] = 0x0000;      // Ek cfg
    regs[12] = 0x0000;      // SOC2
    regs[13] = 0x0000;
    regs[14] = 0x0000;
    regs[15] = 0x0001;      // Write enable

    DEBUG_SERIAL.printf("\n[SDEL] Time-Sharing Kural %d devre disi birakiliyor...\n", idx);

    if (inverter.writeRegisters(0x1120, 16, regs)) {
        DEBUG_SERIAL.println("[SDEL] Basarili! Kural devre disi birakildi.");
        delay(200);
        uint16_t enResult;
        if (inverter.readRegister(0x112F, enResult)) {
            DEBUG_SERIAL.printf("[SDEL] Enable sonucu: 0x%04X %s\n", enResult,
                enResult == 0x0000 ? "(BASARILI)" : "(HATA!)");
        }
    } else {
        DEBUG_SERIAL.println("[SDEL] Basarisiz!");
    }
}

// --- Zamanli Sarj Kural Ekle ---

void cmdTimedChargeAdd(const char* input) {
    // Format: tadd <idx> <ch_start> <ch_end> <dc_start> <dc_end> <ch_power> <dc_power>
    char parts[10][32] = {};
    int partCount = 0;
    const char* p = input;

    while (*p && partCount < 10) {
        while (*p == ' ') p++;
        if (!*p) break;
        int i = 0;
        while (*p && *p != ' ' && i < 31) {
            parts[partCount][i++] = *p++;
        }
        parts[partCount][i] = '\0';
        partCount++;
    }

    // parts[0]="tadd" [1]=idx [2]=chStart [3]=chEnd [4]=dcStart [5]=dcEnd [6]=chPower [7]=dcPower
    if (partCount < 8) {
        DEBUG_SERIAL.println("[TADD] Kullanim: tadd <idx> <sarjBas> <sarjBit> <desarjBas> <desarjBit> <sarjW> <desarjW>");
        DEBUG_SERIAL.println("  idx: 0-3");
        DEBUG_SERIAL.println("  HHMM: saat (orn: 0100=01:00, 2200=22:00)");
        DEBUG_SERIAL.println("  sarjW/desarjW: guc (Watt)");
        return;
    }

    uint16_t idx = parseDec(parts[1]);
    uint16_t chStart = parseHex(parts[2]);
    uint16_t chEnd = parseHex(parts[3]);
    uint16_t dcStart = parseHex(parts[4]);
    uint16_t dcEnd = parseHex(parts[5]);
    uint32_t chPower = (uint32_t)atoi(parts[6]);
    uint32_t dcPower = (uint32_t)atoi(parts[7]);

    if (idx > 3) { DEBUG_SERIAL.println("[TADD] Index 0-3 arasi olmali!"); return; }

    // Mevcut modu oku — aktif schedule modundayken blok kilitli
    uint16_t currentMode = 0;
    bool modeSwitch = false;
    if (inverter.readRegister(0x1110, currentMode) && currentMode != 0) {
        DEBUG_SERIAL.printf("[TADD] Mod %d aktif, yazma icin mode 0'a geciliyor...\n", currentMode);
        uint16_t modeZero = 0;
        if (!inverter.writeRegisters(0x1110, 1, &modeZero)) {
            DEBUG_SERIAL.println("[TADD] Mod degisimi basarisiz! Yazma iptal.");
            return;
        }
        delay(100);
        modeSwitch = true;
    }

    // 15 register: 0x1111-0x111F
    uint16_t regs[15] = {};
    regs[0] = idx;                          // 0x1111: Kural index
    regs[1] = 0x0003;                       // 0x1112: Enable (Bit0=sarj + Bit1=desarj)
    regs[2] = chStart;                      // 0x1113: Sarj baslangic
    regs[3] = chEnd;                        // 0x1114: Sarj bitis
    regs[4] = dcStart;                      // 0x1115: Desarj baslangic
    regs[5] = dcEnd;                        // 0x1116: Desarj bitis
    regs[6] = (uint16_t)(chPower >> 16);    // 0x1117: Sarj guc high
    regs[7] = (uint16_t)(chPower & 0xFFFF); // 0x1118: Sarj guc low
    regs[8] = (uint16_t)(dcPower >> 16);    // 0x1119: Desarj guc high
    regs[9] = (uint16_t)(dcPower & 0xFFFF); // 0x111A: Desarj guc low
    regs[10] = 0x0000;                      // 0x111B: reserved
    regs[11] = 0x0000;                      // 0x111C: reserved
    regs[12] = 0x0000;                      // 0x111D: reserved
    regs[13] = 0x0000;                      // 0x111E: reserved
    regs[14] = 0x0001;                      // 0x111F: Write enable

    DEBUG_SERIAL.printf("\n[TADD] Zamanli Sarj/Desarj Kural %d yaziliyor...\n", idx);
    DEBUG_SERIAL.printf("  Sarj   : %02d:%02d - %02d:%02d  Guc: %uW\n",
        chStart >> 8, chStart & 0xFF, chEnd >> 8, chEnd & 0xFF, chPower);
    DEBUG_SERIAL.printf("  Desarj : %02d:%02d - %02d:%02d  Guc: %uW\n",
        dcStart >> 8, dcStart & 0xFF, dcEnd >> 8, dcEnd & 0xFF, dcPower);

    if (inverter.writeRegisters(0x1111, 15, regs)) {
        DEBUG_SERIAL.println("[TADD] Yazma basarili!");
        delay(200);
        uint16_t enResult;
        if (inverter.readRegister(0x111F, enResult)) {
            DEBUG_SERIAL.printf("[TADD] Enable sonucu: 0x%04X %s\n", enResult,
                enResult == 0x0000 ? "(BASARILI)" : "(HATA!)");
        }
    } else {
        DEBUG_SERIAL.println("[TADD] Yazma basarisiz!");
    }

    // Modu geri yukle
    if (modeSwitch) {
        delay(100);
        uint16_t targetMode = 2; // Timed charge modu
        if (inverter.writeRegisters(0x1110, 1, &targetMode)) {
            DEBUG_SERIAL.printf("[TADD] Mod %d'ye geri donuldu.\n", targetMode);
        } else {
            DEBUG_SERIAL.printf("[TADD] UYARI: Mod geri yuklenemedi! Mevcut mod: 0\n");
        }
    }
}

// --- Zamanli Sarj Kural Sil ---

void cmdTimedChargeDelete(const char* idxStr) {
    uint16_t idx = parseDec(idxStr);
    if (idx > 3) { DEBUG_SERIAL.println("[TDEL] Index 0-3 arasi olmali!"); return; }

    uint16_t regs[15] = {};
    regs[0] = idx;
    regs[1] = 0x0000;   // Devre disi
    regs[14] = 0x0001;  // Write enable

    DEBUG_SERIAL.printf("\n[TDEL] Zamanli Sarj Kural %d devre disi birakiliyor...\n", idx);

    if (inverter.writeRegisters(0x1111, 15, regs)) {
        DEBUG_SERIAL.println("[TDEL] Basarili!");
        delay(200);
        uint16_t enResult;
        if (inverter.readRegister(0x111F, enResult)) {
            DEBUG_SERIAL.printf("[TDEL] Enable sonucu: 0x%04X %s\n", enResult,
                enResult == 0x0000 ? "(BASARILI)" : "(HATA!)");
        }
    } else {
        DEBUG_SERIAL.println("[TDEL] Basarisiz!");
    }
}

// ============================================================
// REGISTER TARAMA MOTORU
// ============================================================

// Tarama sirasinda serial input'u kontrol et (abort icin)
bool checkScanAbort() {
    while (DEBUG_SERIAL.available()) {
        char c = DEBUG_SERIAL.read();
        if (c == 'q' || c == 'Q' || c == 27) {  // q, Q veya ESC
            scanAbort = true;
            DEBUG_SERIAL.println("\n[SCAN] IPTAL EDILDI! (kullanici durdurdu)");
            return true;
        }
    }
    return false;
}

// Akilli blok tarama: 10'arli bloklar, basarisiz bloklarda tekli tarama
void cmdFullScan(uint16_t startAddr, uint16_t endAddr) {
    const uint16_t BLOCK_SIZE = 10;
    uint32_t totalReadable = 0;
    uint32_t totalException = 0;
    uint32_t totalTimeout = 0;
    uint32_t totalOther = 0;
    uint32_t scanStart = millis();

    scanAbort = false;

    uint16_t totalRegs = endAddr - startAddr + 1;

    DEBUG_SERIAL.println("\n##############################################");
    DEBUG_SERIAL.println("#    SOFAR REGISTER TARAMA BASLIYOR          #");
    DEBUG_SERIAL.println("##############################################");
    DEBUG_SERIAL.printf("Aralik : 0x%04X - 0x%04X (%d register)\n", startAddr, endAddr, totalRegs);
    DEBUG_SERIAL.printf("Blok   : %d register/blok\n", BLOCK_SIZE);
    DEBUG_SERIAL.printf("Slave  : %d\n", inverter.getSlaveId());
    DEBUG_SERIAL.println("Durdurmak icin 'q' basin.");
    DEBUG_SERIAL.println("----------------------------------------------");
    DEBUG_SERIAL.println("CSV FORMAT: addr_hex,value_hex,value_dec,value_signed,status");
    DEBUG_SERIAL.println("----------------------------------------------");

    uint16_t addr = startAddr;

    while (addr <= endAddr) {
        if (checkScanAbort()) break;

        uint16_t blockEnd = addr + BLOCK_SIZE - 1;
        if (blockEnd > endAddr) blockEnd = endAddr;
        uint16_t count = blockEnd - addr + 1;

        uint16_t buffer[60];
        ModbusError err = modbus.readHoldingRegisters(
            inverter.getSlaveId(), addr, count, buffer);

        if (err == MB_OK) {
            // Blok basarili — tum registerlari yazdir
            for (uint16_t i = 0; i < count; i++) {
                DEBUG_SERIAL.printf("0x%04X,0x%04X,%u,%d,OK\n",
                    addr + i, buffer[i], buffer[i], (int16_t)buffer[i]);
                totalReadable++;
            }
            addr += count;
        }
        else if (err == MB_ERR_TIMEOUT) {
            // Tum blok oldu — hepsini timeout olarak kaydet, atla
            totalTimeout += count;
            addr += count;
        }
        else if (err == MB_ERR_EXCEPTION) {
            // Blok exception — tekli tarama yap
            for (uint16_t i = 0; i < count; i++) {
                if (checkScanAbort()) break;

                uint16_t singleAddr = addr + i;
                uint16_t val;
                ModbusError sErr = modbus.readHoldingRegisters(
                    inverter.getSlaveId(), singleAddr, 1, &val);

                if (sErr == MB_OK) {
                    DEBUG_SERIAL.printf("0x%04X,0x%04X,%u,%d,OK\n",
                        singleAddr, val, val, (int16_t)val);
                    totalReadable++;
                }
                else if (sErr == MB_ERR_TIMEOUT) {
                    totalTimeout++;
                }
                else if (sErr == MB_ERR_EXCEPTION) {
                    uint8_t ex = modbus.getLastException();
                    DEBUG_SERIAL.printf("0x%04X,,,, EX_%02X\n", singleAddr, ex);
                    totalException++;
                }
                else {
                    totalOther++;
                }
                delay(30);
            }
            addr += count;
        }
        else {
            // Diger hatalar (CRC vb.)
            totalOther += count;
            addr += count;
        }

        // Ilerleme goster (her 256 registerde bir)
        if ((addr - startAddr) % 256 < BLOCK_SIZE && addr > startAddr) {
            uint32_t elapsed = (millis() - scanStart) / 1000;
            uint16_t done = addr - startAddr;
            uint16_t pct = (uint32_t)done * 100 / totalRegs;
            DEBUG_SERIAL.printf("# --- ILERLEME: %d/%d (%%%d) | %ds | OK:%u EX:%u TO:%u ---\n",
                done, totalRegs, pct, elapsed, totalReadable, totalException, totalTimeout);
        }

        delay(30);  // Register'lar arasi kisa bekleme
    }

    // Ozet
    uint32_t elapsed = (millis() - scanStart) / 1000;
    DEBUG_SERIAL.println("\n##############################################");
    DEBUG_SERIAL.println("#          TARAMA TAMAMLANDI                 #");
    DEBUG_SERIAL.println("##############################################");
    DEBUG_SERIAL.printf("Aralik    : 0x%04X - 0x%04X\n", startAddr, endAddr);
    DEBUG_SERIAL.printf("Sure      : %u saniye\n", elapsed);
    DEBUG_SERIAL.printf("Okunabilir: %u register\n", totalReadable);
    DEBUG_SERIAL.printf("Exception : %u register\n", totalException);
    DEBUG_SERIAL.printf("Timeout   : %u register\n", totalTimeout);
    DEBUG_SERIAL.printf("Diger hata: %u register\n", totalOther);
    DEBUG_SERIAL.printf("Toplam    : %u register\n",
        totalReadable + totalException + totalTimeout + totalOther);
    DEBUG_SERIAL.println("##############################################\n");
}

// Yazilabilirlik testi: Mevcut degeri oku, ayni degeri geri yaz, sonucu kontrol et
void cmdScanWrite(uint16_t startAddr, uint16_t endAddr) {
    uint32_t totalWritable = 0;
    uint32_t totalReadOnly = 0;
    uint32_t totalUnreadable = 0;
    uint32_t scanStart = millis();

    scanAbort = false;

    DEBUG_SERIAL.println("\n##############################################");
    DEBUG_SERIAL.println("#    YAZILABILIRLIK TESTI BASLIYOR           #");
    DEBUG_SERIAL.println("##############################################");
    DEBUG_SERIAL.printf("Aralik : 0x%04X - 0x%04X\n", startAddr, endAddr);
    DEBUG_SERIAL.println("Yontem : Mevcut degeri oku, ayni degeri geri yaz (GUVENLI)");
    DEBUG_SERIAL.println("Durdurmak icin 'q' basin.");
    DEBUG_SERIAL.println("----------------------------------------------");
    DEBUG_SERIAL.println("CSV: addr_hex,value_hex,write_result");
    DEBUG_SERIAL.println("----------------------------------------------");

    for (uint16_t addr = startAddr; addr <= endAddr; addr++) {
        if (checkScanAbort()) break;

        // Once oku
        uint16_t val;
        ModbusError rErr = modbus.readHoldingRegisters(
            inverter.getSlaveId(), addr, 1, &val);

        if (rErr != MB_OK) {
            totalUnreadable++;
            delay(30);
            continue;
        }

        // Ayni degeri geri yaz (FC 0x10)
        delay(50);
        ModbusError wErr = modbus.writeMultipleRegisters(
            inverter.getSlaveId(), addr, 1, &val);

        if (wErr == MB_OK) {
            DEBUG_SERIAL.printf("0x%04X,0x%04X,RW\n", addr, val);
            totalWritable++;
        } else if (wErr == MB_ERR_EXCEPTION) {
            uint8_t ex = modbus.getLastException();
            DEBUG_SERIAL.printf("0x%04X,0x%04X,RO_EX%02X\n", addr, val, ex);
            totalReadOnly++;
        } else {
            DEBUG_SERIAL.printf("0x%04X,0x%04X,ERR\n", addr, val);
            totalReadOnly++;
        }

        delay(50);
    }

    uint32_t elapsed = (millis() - scanStart) / 1000;
    DEBUG_SERIAL.println("\n##############################################");
    DEBUG_SERIAL.println("#    YAZILABILIRLIK TESTI TAMAMLANDI         #");
    DEBUG_SERIAL.println("##############################################");
    DEBUG_SERIAL.printf("Sure       : %u saniye\n", elapsed);
    DEBUG_SERIAL.printf("Yazilabilir: %u (RW)\n", totalWritable);
    DEBUG_SERIAL.printf("Salt okunur: %u (RO)\n", totalReadOnly);
    DEBUG_SERIAL.printf("Okunamadi  : %u\n", totalUnreadable);
    DEBUG_SERIAL.println("##############################################\n");
}

// --- Komut Isleyici ---

void processCommand(const char* cmd) {
    // Bosluktan ayir
    char parts[4][32] = {};
    int partCount = 0;
    const char* p = cmd;

    while (*p && partCount < 4) {
        while (*p == ' ') p++;
        if (!*p) break;

        int i = 0;
        while (*p && *p != ' ' && i < 31) {
            parts[partCount][i++] = *p++;
        }
        parts[partCount][i] = '\0';
        partCount++;
    }

    if (partCount == 0) return;

    // Komut eslestirme
    if (strcmp(parts[0], "help") == 0 || strcmp(parts[0], "?") == 0) {
        printHelp();
    }
    else if (strcmp(parts[0], "all") == 0) {
        inverter.printAllData();
    }
    else if (strcmp(parts[0], "sys") == 0) {
        SofarSystemInfo info;
        if (inverter.readSystemInfo(info)) inverter.printSystemInfo(info);
        else DEBUG_SERIAL.println("[HATA] Sistem bilgisi okunamadi!");
    }
    else if (strcmp(parts[0], "grid") == 0) {
        SofarGridOutput grid;
        if (inverter.readGridOutput(grid)) inverter.printGridOutput(grid);
        else DEBUG_SERIAL.println("[HATA] Sebeke cikisi okunamadi!");
    }
    else if (strcmp(parts[0], "pv") == 0) {
        SofarPVInput pv;
        if (inverter.readPVInput(pv)) inverter.printPVInput(pv);
        else DEBUG_SERIAL.println("[HATA] PV giris okunamadi!");
    }
    else if (strcmp(parts[0], "bat") == 0) {
        SofarBattery bat;
        if (inverter.readBattery(bat)) inverter.printBattery(bat);
        else DEBUG_SERIAL.println("[HATA] Batarya bilgisi okunamadi!");
    }
    else if (strcmp(parts[0], "energy") == 0) {
        SofarEnergy energy;
        if (inverter.readEnergy(energy)) inverter.printEnergy(energy);
        else DEBUG_SERIAL.println("[HATA] Enerji istatistikleri okunamadi!");
    }
    else if (strcmp(parts[0], "read") == 0 && partCount >= 2) {
        cmdReadRegister(parts[1]);
    }
    else if (strcmp(parts[0], "readn") == 0 && partCount >= 3) {
        cmdReadRegisters(parts[1], parts[2]);
    }
    else if (strcmp(parts[0], "write") == 0 && partCount >= 3) {
        cmdWriteRegister(parts[1], parts[2]);
    }
    else if (strcmp(parts[0], "writem") == 0) {
        cmdWriteMultipleRegisters(cmd);
    }
    else if (strcmp(parts[0], "slist") == 0) {
        cmdScheduleList();
    }
    else if (strcmp(parts[0], "smode") == 0 && partCount >= 2) {
        cmdSetEnergyMode(parts[1]);
    }
    else if (strcmp(parts[0], "sadd") == 0) {
        cmdScheduleAdd(cmd);
    }
    else if (strcmp(parts[0], "sdel") == 0 && partCount >= 2) {
        cmdScheduleDelete(parts[1]);
    }
    else if (strcmp(parts[0], "tadd") == 0) {
        cmdTimedChargeAdd(cmd);
    }
    else if (strcmp(parts[0], "tdel") == 0 && partCount >= 2) {
        cmdTimedChargeDelete(parts[1]);
    }
    else if (strcmp(parts[0], "scan") == 0) {
        scanSlaveIds();
    }
    else if (strcmp(parts[0], "fullscan") == 0) {
        cmdFullScan(0x0000, 0x1FFF);
    }
    else if (strcmp(parts[0], "scanrange") == 0 && partCount >= 3) {
        uint16_t sAddr = parseHex(parts[1]);
        uint16_t eAddr = parseHex(parts[2]);
        if (eAddr < sAddr) {
            DEBUG_SERIAL.println("[SCANRANGE] Bitis adresi baslangictan buyuk olmali!");
        } else {
            cmdFullScan(sAddr, eAddr);
        }
    }
    else if (strcmp(parts[0], "scanwrite") == 0 && partCount >= 3) {
        uint16_t sAddr = parseHex(parts[1]);
        uint16_t eAddr = parseHex(parts[2]);
        if (eAddr < sAddr) {
            DEBUG_SERIAL.println("[SCANWRITE] Bitis adresi baslangictan buyuk olmali!");
        } else {
            cmdScanWrite(sAddr, eAddr);
        }
    }
    else if (strcmp(parts[0], "auto") == 0) {
        autoPollEnabled = !autoPollEnabled;
        DEBUG_SERIAL.printf("[AUTO] Otomatik okuma: %s (aralik: %d ms)\n",
                            autoPollEnabled ? "ACIK" : "KAPALI", AUTO_POLL_INTERVAL);
    }
    else if (strcmp(parts[0], "slave") == 0 && partCount >= 2) {
        uint8_t newId = atoi(parts[1]);
        if (newId >= 1 && newId <= 247) {
            DEBUG_SERIAL.printf("[SLAVE] Slave ID degistirildi: %d\n", newId);
            inverter.setSlaveId(newId);
        } else {
            DEBUG_SERIAL.println("[SLAVE] Gecersiz ID! (1-247 arasi olmali)");
        }
    }
    else {
        DEBUG_SERIAL.printf("[?] Bilinmeyen komut: '%s'. 'help' yazin.\n", parts[0]);
    }
}

// ============================================================
// SETUP
// ============================================================

void setup() {
    // Debug serial
    DEBUG_SERIAL.begin(DEBUG_BAUD_RATE);
    delay(1000);

    DEBUG_SERIAL.println("\n\n");
    DEBUG_SERIAL.println("================================================");
    DEBUG_SERIAL.println("  SOFAR INVERTOR - ESP32 Modbus RTU Okuyucu");
    DEBUG_SERIAL.println("  Sofar Modbus G3 Protocol V1.39");
    DEBUG_SERIAL.println("  RS485: 9600 8N1");
    DEBUG_SERIAL.printf("  Slave ID: %d\n", MODBUS_SLAVE_ID);
    DEBUG_SERIAL.println("================================================");
    DEBUG_SERIAL.println();

    // Modbus LED (opsiyonel)
    pinMode(LED_MODBUS_PIN, OUTPUT);
    pinMode(LED_BUILTIN_PIN, OUTPUT);
    digitalWrite(LED_MODBUS_PIN, LOW);
    digitalWrite(LED_BUILTIN_PIN, LOW);

    // Modbus baslat
    modbus.begin();

    DEBUG_SERIAL.println("[HAZIR] Komut girebilirsiniz. 'help' yazin.");
    DEBUG_SERIAL.println();
}

// ============================================================
// LOOP
// ============================================================

void loop() {
    // Serial komut okuma
    while (DEBUG_SERIAL.available()) {
        char c = DEBUG_SERIAL.read();
        if (c == '\n' || c == '\r') {
            if (cmdIndex > 0) {
                cmdBuffer[cmdIndex] = '\0';
                DEBUG_SERIAL.printf("> %s\n", cmdBuffer);

                // LED yak
                digitalWrite(LED_MODBUS_PIN, HIGH);
                processCommand(cmdBuffer);
                digitalWrite(LED_MODBUS_PIN, LOW);

                cmdIndex = 0;
            }
        } else if (cmdIndex < sizeof(cmdBuffer) - 1) {
            cmdBuffer[cmdIndex++] = c;
        }
    }

    // Otomatik okuma
    if (autoPollEnabled && (millis() - lastPollTime >= AUTO_POLL_INTERVAL)) {
        lastPollTime = millis();
        DEBUG_SERIAL.println("\n--- [AUTO POLL] ---");
        digitalWrite(LED_MODBUS_PIN, HIGH);
        inverter.printAllData();
        digitalWrite(LED_MODBUS_PIN, LOW);
    }
}
