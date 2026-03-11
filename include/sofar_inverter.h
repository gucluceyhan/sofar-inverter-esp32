#pragma once

#include <Arduino.h>
#include "modbus_rtu.h"
#include "sofar_registers.h"
#include "config.h"

// Invertor veri yapilari
struct SofarSystemInfo {
    uint16_t state;
    const char* stateName;
    uint16_t faults[5];
    int16_t ambientTemp1;
    int16_t ambientTemp2;
    int16_t radiatorTemp1;
    int16_t radiatorTemp2;
    int16_t moduleTemp1;
    int16_t moduleTemp2;
    uint16_t fanSpeed;
    uint16_t genTimeToday;  // dakika
    uint32_t totalGenTime;  // dakika
    uint32_t totalRunTime;  // dakika
    uint16_t insulationImp; // kOhm
};

struct SofarGridOutput {
    float gridFreq;         // Hz
    float totalActivePower; // kW
    float totalReactivePower; // kVar
    float totalApparentPower; // kVA
    float totalPccActive;   // kW
    float voltageR;         // V
    float currentR;         // A
    float activePowerR;     // kW
    float voltageS;         // V
    float currentS;         // A
    float activePowerS;     // kW
    float voltageT;         // V
    float currentT;         // A
    float activePowerT;     // kW
    float powerFactor;
    float efficiency;       // %
};

struct SofarPVInput {
    float voltage[4];       // V (4 kanal)
    float current[4];       // A
    float power[4];         // kW
    float totalPower;       // kW
};

struct SofarBattery {
    float voltage;          // V
    float current;          // A (+ sarj, - desarj)
    float power;            // kW
    int16_t temperature;    // C
    uint16_t soc;           // %
    uint16_t soh;           // %
    uint16_t cycles;
    float totalPower;       // kW
    uint16_t avgSoc;        // %
    uint16_t packCount;
};

struct SofarEnergy {
    float dailyGen;         // kWh
    float totalGen;         // kWh
    float dailyLoad;        // kWh
    float totalLoad;        // kWh
    float dailyBought;      // kWh
    float totalBought;      // kWh
    float dailySold;        // kWh
    float totalSold;        // kWh
    float dailyBatCharge;   // kWh
    float totalBatCharge;   // kWh
    float dailyBatDischarge; // kWh
    float totalBatDischarge; // kWh
};

class SofarInverter {
public:
    SofarInverter(ModbusRTU& modbus, uint8_t slaveId = MODBUS_SLAVE_ID);

    void setSlaveId(uint8_t id) { _slaveId = id; }
    uint8_t getSlaveId() const { return _slaveId; }

    // Toplu okuma fonksiyonlari (retry destekli)
    bool readSystemInfo(SofarSystemInfo& info);
    bool readGridOutput(SofarGridOutput& grid);
    bool readPVInput(SofarPVInput& pv);
    bool readBattery(SofarBattery& bat);
    bool readEnergy(SofarEnergy& energy);

    // Tek register okuma
    bool readRegister(uint16_t addr, uint16_t& value);
    bool readRegisters(uint16_t addr, uint16_t count, uint16_t* buffer);

    // U32 okuma (2 ardisik register, big-endian)
    bool readU32(uint16_t addr, uint32_t& value);

    // Tek register yazma
    bool writeRegister(uint16_t addr, uint16_t value);
    bool writeRegisters(uint16_t addr, uint16_t count, const uint16_t* values);

    // Yazdirma yardimcilari
    void printSystemInfo(const SofarSystemInfo& info);
    void printGridOutput(const SofarGridOutput& grid);
    void printPVInput(const SofarPVInput& pv);
    void printBattery(const SofarBattery& bat);
    void printEnergy(const SofarEnergy& energy);
    void printAllData();

private:
    ModbusRTU& _modbus;
    uint8_t _slaveId;

    // Retry mekanizmasi
    ModbusError readWithRetry(uint16_t addr, uint16_t count, uint16_t* buffer);
};
