#include <Arduino.h>
#include "config.h"
#include "modbus_rtu.h"
#include "sofar_inverter.h"

ModbusRTU modbus;
SofarInverter inverter(modbus, MODBUS_SLAVE_ID);

// Otomatik okuma kontrolu
bool autoPollEnabled = false;
unsigned long lastPollTime = 0;

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
    else if (strcmp(parts[0], "scan") == 0) {
        scanSlaveIds();
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
