#pragma once

#include <Arduino.h>
#include "config.h"

// Modbus Function Codes
#define FC_READ_HOLDING_REGISTERS   0x03
#define FC_WRITE_SINGLE_REGISTER    0x06
#define FC_WRITE_MULTIPLE_REGISTERS 0x10

// Modbus Hata Kodlari
enum ModbusError {
    MB_OK = 0,
    MB_ERR_TIMEOUT,
    MB_ERR_CRC,
    MB_ERR_EXCEPTION,
    MB_ERR_INVALID_RESPONSE,
    MB_ERR_BUFFER_OVERFLOW
};

// Modbus Exception Kodlari (Sofar dokumantasyonundan)
enum ModbusException {
    MB_EX_ILLEGAL_FUNCTION = 0x01,
    MB_EX_ILLEGAL_ADDRESS  = 0x02,
    MB_EX_ILLEGAL_DATA     = 0x03,
    MB_EX_DEVICE_FAULT     = 0x04,
    MB_EX_DEVICE_BUSY      = 0x07
};

class ModbusRTU {
public:
    void begin();

    // Okuma: FC 0x03 - Read Holding Registers
    ModbusError readHoldingRegisters(uint8_t slaveId, uint16_t startAddr,
                                     uint16_t count, uint16_t* buffer);

    // Yazma: FC 0x06 - Write Single Register
    ModbusError writeSingleRegister(uint8_t slaveId, uint16_t addr,
                                    uint16_t value);

    // Yazma: FC 0x10 - Write Multiple Registers
    ModbusError writeMultipleRegisters(uint8_t slaveId, uint16_t startAddr,
                                       uint16_t count, const uint16_t* values);

    // Hata aciklamasi
    static const char* errorToString(ModbusError err);
    static const char* exceptionToString(uint8_t code);

    // Son hata bilgisi
    uint8_t getLastException() const { return _lastException; }

private:
    uint8_t _txBuffer[256];
    uint8_t _rxBuffer[256];
    uint8_t _lastException = 0;

    // RS485 yon kontrolu
    void setTxMode();
    void setRxMode();

    // CRC-16 Modbus
    static uint16_t calculateCRC(const uint8_t* data, uint8_t length);

    // Veri gonderme
    void sendRequest(uint8_t length);

    // Yanit okuma
    int receiveResponse(uint8_t expectedMinLen);

    // RX buffer temizle
    void flushRxBuffer();
};
