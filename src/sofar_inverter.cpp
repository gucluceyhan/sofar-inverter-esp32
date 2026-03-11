#include "sofar_inverter.h"

SofarInverter::SofarInverter(ModbusRTU& modbus, uint8_t slaveId)
    : _modbus(modbus), _slaveId(slaveId) {}

// --- Retry Mekanizmasi ---

ModbusError SofarInverter::readWithRetry(uint16_t addr, uint16_t count, uint16_t* buffer) {
    ModbusError err;
    for (int attempt = 0; attempt < MODBUS_MAX_RETRIES; attempt++) {
        err = _modbus.readHoldingRegisters(_slaveId, addr, count, buffer);
        if (err == MB_OK) return MB_OK;

        DEBUG_SERIAL.printf("[SOFAR] Okuma hatasi 0x%04X (deneme %d/%d): %s\n",
                            addr, attempt + 1, MODBUS_MAX_RETRIES,
                            ModbusRTU::errorToString(err));

        if (err == MB_ERR_EXCEPTION) break;  // Exception'da tekrar deneme
        delay(MODBUS_RETRY_DELAY);
    }
    return err;
}

// --- Tek Register / Coklu Register ---

bool SofarInverter::readRegister(uint16_t addr, uint16_t& value) {
    return readWithRetry(addr, 1, &value) == MB_OK;
}

bool SofarInverter::readRegisters(uint16_t addr, uint16_t count, uint16_t* buffer) {
    return readWithRetry(addr, count, buffer) == MB_OK;
}

bool SofarInverter::readU32(uint16_t addr, uint32_t& value) {
    uint16_t regs[2];
    if (readWithRetry(addr, 2, regs) != MB_OK) return false;
    // Sofar: big-endian — ilk register yuksek word
    value = ((uint32_t)regs[0] << 16) | regs[1];
    return true;
}

bool SofarInverter::writeRegister(uint16_t addr, uint16_t value) {
    ModbusError err = _modbus.writeSingleRegister(_slaveId, addr, value);
    if (err != MB_OK) {
        DEBUG_SERIAL.printf("[SOFAR] Yazma hatasi 0x%04X: %s\n",
                            addr, ModbusRTU::errorToString(err));
        return false;
    }
    return true;
}

bool SofarInverter::writeRegisters(uint16_t addr, uint16_t count, const uint16_t* values) {
    ModbusError err = _modbus.writeMultipleRegisters(_slaveId, addr, count, values);
    if (err != MB_OK) {
        DEBUG_SERIAL.printf("[SOFAR] Coklu yazma hatasi 0x%04X: %s\n",
                            addr, ModbusRTU::errorToString(err));
        return false;
    }
    return true;
}

// --- Sistem Bilgisi Okuma ---

bool SofarInverter::readSystemInfo(SofarSystemInfo& info) {
    // Block 1: 0x0404 - 0x0431 (46 register)
    // Sofar bloklar arasi okuma desteklemiyor, max 60 register
    uint16_t regs[46];
    if (!readRegisters(0x0404, 46, regs)) return false;

    info.state = regs[0];  // 0x0404
    info.stateName = (info.state < SYSTEM_STATE_COUNT)
                     ? SYSTEM_STATE_NAMES[info.state] : "Bilinmiyor";

    info.faults[0] = regs[1];  // 0x0405
    info.faults[1] = regs[2];  // 0x0406
    info.faults[2] = regs[3];  // 0x0407
    info.faults[3] = regs[4];  // 0x0408
    info.faults[4] = regs[5];  // 0x0409

    info.genTimeToday = regs[0x0417 - 0x0404];  // index 19
    info.ambientTemp1 = (int16_t)regs[0x0418 - 0x0404];
    info.ambientTemp2 = (int16_t)regs[0x0419 - 0x0404];
    info.radiatorTemp1 = (int16_t)regs[0x041A - 0x0404];
    info.radiatorTemp2 = (int16_t)regs[0x041B - 0x0404];
    info.moduleTemp1 = (int16_t)regs[0x0420 - 0x0404];
    info.moduleTemp2 = (int16_t)regs[0x0421 - 0x0404];

    info.genTimeToday = regs[0x0426 - 0x0404];
    info.totalGenTime = ((uint32_t)regs[0x0427 - 0x0404] << 16) | regs[0x0428 - 0x0404];
    info.totalRunTime = ((uint32_t)regs[0x0429 - 0x0404] << 16) | regs[0x042A - 0x0404];
    info.insulationImp = regs[0x042B - 0x0404];

    // Fan hizi ayri blokta: 0x043E
    uint16_t fanReg;
    if (readRegister(0x043E, fanReg)) {
        info.fanSpeed = fanReg;
    } else {
        info.fanSpeed = 0;
    }

    return true;
}

// --- Sebeke Cikis Okuma ---

bool SofarInverter::readGridOutput(SofarGridOutput& grid) {
    // Block: 0x0484 - 0x04BF (60 register, tam sinir)
    uint16_t regs[60];
    if (!readRegisters(0x0484, 60, regs)) return false;

    grid.gridFreq = regs[0] * 0.01f;           // 0x0484
    grid.totalActivePower = (int16_t)regs[1] * 0.01f;  // 0x0485
    grid.totalReactivePower = (int16_t)regs[2] * 0.01f; // 0x0486
    grid.totalApparentPower = (int16_t)regs[3] * 0.01f; // 0x0487
    grid.totalPccActive = (int16_t)regs[4] * 0.01f;     // 0x0488

    // R fazı (0x048D = index 9)
    grid.voltageR = regs[0x048D - 0x0484] * 0.1f;
    grid.currentR = regs[0x048E - 0x0484] * 0.01f;
    grid.activePowerR = (int16_t)regs[0x048F - 0x0484] * 0.01f;

    // S fazı (0x0498 = index 20)
    grid.voltageS = regs[0x0498 - 0x0484] * 0.1f;
    grid.currentS = regs[0x0499 - 0x0484] * 0.01f;
    grid.activePowerS = (int16_t)regs[0x049A - 0x0484] * 0.01f;

    // T fazı (0x04A3 = index 31)
    grid.voltageT = regs[0x04A3 - 0x0484] * 0.1f;
    grid.currentT = regs[0x04A4 - 0x0484] * 0.01f;
    grid.activePowerT = (int16_t)regs[0x04A5 - 0x0484] * 0.01f;

    // Toplam guc faktoru ve verim (0x04BD = index 57, 0x04BF = index 59)
    grid.powerFactor = (int16_t)regs[0x04BD - 0x0484] * 0.001f;
    grid.efficiency = regs[0x04BF - 0x0484] * 0.01f;

    return true;
}

// --- PV Giris Okuma ---

bool SofarInverter::readPVInput(SofarPVInput& pv) {
    // Block: 0x0584 - 0x058F (12 register = 4 kanal x 3 register)
    uint16_t regs[12];
    if (!readRegisters(0x0584, 12, regs)) return false;

    for (int i = 0; i < 4; i++) {
        pv.voltage[i] = regs[i * 3] * 0.1f;
        pv.current[i] = (int16_t)regs[i * 3 + 1] * 0.01f;
        pv.power[i] = regs[i * 3 + 2] * 0.01f;
    }

    // Toplam PV gucu: 0x05C4
    uint16_t totalReg;
    if (readRegister(0x05C4, totalReg)) {
        pv.totalPower = totalReg * 0.1f;
    } else {
        pv.totalPower = 0;
        for (int i = 0; i < 4; i++) pv.totalPower += pv.power[i];
    }

    return true;
}

// --- Batarya Okuma ---

bool SofarInverter::readBattery(SofarBattery& bat) {
    // Block 1: Batarya 1 temel bilgiler 0x0604-0x060A (7 register)
    uint16_t regs[7];
    if (!readRegisters(0x0604, 7, regs)) return false;

    bat.voltage = regs[0] * 0.1f;           // 0x0604
    bat.current = (int16_t)regs[1] * 0.01f; // 0x0605
    bat.power = (int16_t)regs[2] * 0.01f;   // 0x0606
    bat.temperature = (int16_t)regs[3];      // 0x0607
    bat.soc = regs[4];                       // 0x0608
    bat.soh = regs[5];                       // 0x0609
    bat.cycles = regs[6];                    // 0x060A

    // Block 2: Genel batarya bilgileri 0x0667-0x066B (5 register)
    uint16_t regs2[5];
    if (readRegisters(0x0667, 5, regs2)) {
        bat.totalPower = (int16_t)regs2[0] * 0.1f;  // 0x0667
        bat.avgSoc = regs2[1];                        // 0x0668
        bat.packCount = regs2[3];                     // 0x066A
    }

    return true;
}

// --- Enerji Istatistikleri Okuma ---

bool SofarInverter::readEnergy(SofarEnergy& energy) {
    // Block: 0x0684 - 0x069B (24 register = 12 x U32)
    uint16_t regs[24];
    if (!readRegisters(0x0684, 24, regs)) return false;

    auto toU32 = [&](int idx) -> uint32_t {
        return ((uint32_t)regs[idx] << 16) | regs[idx + 1];
    };

    energy.dailyGen = toU32(0) * 0.01f;            // 0x0684-0685
    energy.totalGen = toU32(2) * 0.1f;             // 0x0686-0687
    energy.dailyLoad = toU32(4) * 0.01f;           // 0x0688-0689
    energy.totalLoad = toU32(6) * 0.1f;            // 0x068A-068B
    energy.dailyBought = toU32(8) * 0.01f;         // 0x068C-068D
    energy.totalBought = toU32(10) * 0.1f;         // 0x068E-068F
    energy.dailySold = toU32(12) * 0.01f;          // 0x0690-0691
    energy.totalSold = toU32(14) * 0.1f;           // 0x0692-0693
    energy.dailyBatCharge = toU32(16) * 0.01f;     // 0x0694-0695
    energy.totalBatCharge = toU32(18) * 0.1f;      // 0x0696-0697
    energy.dailyBatDischarge = toU32(20) * 0.01f;  // 0x0698-0699
    energy.totalBatDischarge = toU32(22) * 0.1f;   // 0x069A-069B

    return true;
}

// --- Yazdirma Fonksiyonlari ---

void SofarInverter::printSystemInfo(const SofarSystemInfo& info) {
    DEBUG_SERIAL.println("\n========== SISTEM BILGISI ==========");
    DEBUG_SERIAL.printf("Durum: %s (%d)\n", info.stateName, info.state);
    DEBUG_SERIAL.printf("Ortam sicakligi: %d / %d C\n", info.ambientTemp1, info.ambientTemp2);
    DEBUG_SERIAL.printf("Radyator sicakligi: %d / %d C\n", info.radiatorTemp1, info.radiatorTemp2);
    DEBUG_SERIAL.printf("Modul sicakligi: %d / %d C\n", info.moduleTemp1, info.moduleTemp2);
    DEBUG_SERIAL.printf("Fan hizi: %d r/min\n", info.fanSpeed);
    DEBUG_SERIAL.printf("Bugun uretim suresi: %d dk\n", info.genTimeToday);
    DEBUG_SERIAL.printf("Toplam uretim suresi: %u dk\n", info.totalGenTime);
    DEBUG_SERIAL.printf("Toplam calisma suresi: %u dk\n", info.totalRunTime);
    DEBUG_SERIAL.printf("Izolasyon empedansi: %d kOhm\n", info.insulationImp);

    bool hasFault = false;
    for (int i = 0; i < 5; i++) {
        if (info.faults[i] != 0) {
            DEBUG_SERIAL.printf("Hata %d: 0x%04X\n", i + 1, info.faults[i]);
            hasFault = true;
        }
    }
    if (!hasFault) DEBUG_SERIAL.println("Hata: YOK");
}

void SofarInverter::printGridOutput(const SofarGridOutput& grid) {
    DEBUG_SERIAL.println("\n========== SEBEKE CIKISI ==========");
    DEBUG_SERIAL.printf("Sebeke frekansi: %.2f Hz\n", grid.gridFreq);
    DEBUG_SERIAL.printf("Toplam aktif guc: %.2f kW\n", grid.totalActivePower);
    DEBUG_SERIAL.printf("Toplam reaktif guc: %.2f kVar\n", grid.totalReactivePower);
    DEBUG_SERIAL.printf("Toplam gorunen guc: %.2f kVA\n", grid.totalApparentPower);
    DEBUG_SERIAL.printf("Toplam PCC aktif guc: %.2f kW\n", grid.totalPccActive);
    DEBUG_SERIAL.println("--- Faz R ---");
    DEBUG_SERIAL.printf("  Gerilim: %.1f V  Akim: %.2f A  Guc: %.2f kW\n",
                        grid.voltageR, grid.currentR, grid.activePowerR);
    DEBUG_SERIAL.println("--- Faz S ---");
    DEBUG_SERIAL.printf("  Gerilim: %.1f V  Akim: %.2f A  Guc: %.2f kW\n",
                        grid.voltageS, grid.currentS, grid.activePowerS);
    DEBUG_SERIAL.println("--- Faz T ---");
    DEBUG_SERIAL.printf("  Gerilim: %.1f V  Akim: %.2f A  Guc: %.2f kW\n",
                        grid.voltageT, grid.currentT, grid.activePowerT);
    DEBUG_SERIAL.printf("Guc faktoru: %.3f\n", grid.powerFactor);
    DEBUG_SERIAL.printf("Verim: %.2f %%\n", grid.efficiency);
}

void SofarInverter::printPVInput(const SofarPVInput& pv) {
    DEBUG_SERIAL.println("\n========== PV GIRIS ==========");
    for (int i = 0; i < 4; i++) {
        if (pv.voltage[i] > 0 || pv.current[i] > 0 || pv.power[i] > 0) {
            DEBUG_SERIAL.printf("PV%d: %.1f V  %.2f A  %.2f kW\n",
                                i + 1, pv.voltage[i], pv.current[i], pv.power[i]);
        }
    }
    DEBUG_SERIAL.printf("Toplam PV gucu: %.1f kW\n", pv.totalPower);
}

void SofarInverter::printBattery(const SofarBattery& bat) {
    DEBUG_SERIAL.println("\n========== BATARYA ==========");
    DEBUG_SERIAL.printf("Gerilim: %.1f V\n", bat.voltage);
    DEBUG_SERIAL.printf("Akim: %.2f A (%s)\n", bat.current,
                        bat.current > 0 ? "Sarj" : (bat.current < 0 ? "Desarj" : "Bosta"));
    DEBUG_SERIAL.printf("Guc: %.2f kW\n", bat.power);
    DEBUG_SERIAL.printf("Sicaklik: %d C\n", bat.temperature);
    DEBUG_SERIAL.printf("SOC: %d%%  SOH: %d%%\n", bat.soc, bat.soh);
    DEBUG_SERIAL.printf("Dongu sayisi: %d\n", bat.cycles);
    DEBUG_SERIAL.printf("Toplam guc: %.1f kW\n", bat.totalPower);
    DEBUG_SERIAL.printf("Ortalama SOC: %d%%  Paket sayisi: %d\n", bat.avgSoc, bat.packCount);
}

void SofarInverter::printEnergy(const SofarEnergy& energy) {
    DEBUG_SERIAL.println("\n========== ENERJI ISTATISTIKLERI ==========");
    DEBUG_SERIAL.printf("Gunluk uretim: %.2f kWh   Toplam: %.1f kWh\n",
                        energy.dailyGen, energy.totalGen);
    DEBUG_SERIAL.printf("Gunluk tuketim: %.2f kWh  Toplam: %.1f kWh\n",
                        energy.dailyLoad, energy.totalLoad);
    DEBUG_SERIAL.printf("Gunluk alinan: %.2f kWh   Toplam: %.1f kWh\n",
                        energy.dailyBought, energy.totalBought);
    DEBUG_SERIAL.printf("Gunluk satilan: %.2f kWh  Toplam: %.1f kWh\n",
                        energy.dailySold, energy.totalSold);
    DEBUG_SERIAL.printf("Gunluk bat sarj: %.2f kWh Toplam: %.1f kWh\n",
                        energy.dailyBatCharge, energy.totalBatCharge);
    DEBUG_SERIAL.printf("Gunluk bat desarj: %.2f kWh Toplam: %.1f kWh\n",
                        energy.dailyBatDischarge, energy.totalBatDischarge);
}

void SofarInverter::printAllData() {
    SofarSystemInfo sysInfo;
    SofarGridOutput gridOut;
    SofarPVInput pvIn;
    SofarBattery bat;
    SofarEnergy energy;

    DEBUG_SERIAL.println("\n################################################");
    DEBUG_SERIAL.println("#       SOFAR INVERTOR - TUM VERILER            #");
    DEBUG_SERIAL.println("################################################");

    if (readSystemInfo(sysInfo))  printSystemInfo(sysInfo);
    else DEBUG_SERIAL.println("[HATA] Sistem bilgisi okunamadi!");

    delay(50);  // Bloklar arasi kisa bekleme

    if (readGridOutput(gridOut))  printGridOutput(gridOut);
    else DEBUG_SERIAL.println("[HATA] Sebeke cikisi okunamadi!");

    delay(50);

    if (readPVInput(pvIn))       printPVInput(pvIn);
    else DEBUG_SERIAL.println("[HATA] PV giris okunamadi!");

    delay(50);

    if (readBattery(bat))        printBattery(bat);
    else DEBUG_SERIAL.println("[HATA] Batarya bilgisi okunamadi!");

    delay(50);

    if (readEnergy(energy))      printEnergy(energy);
    else DEBUG_SERIAL.println("[HATA] Enerji istatistikleri okunamadi!");

    DEBUG_SERIAL.println("\n################################################\n");
}
