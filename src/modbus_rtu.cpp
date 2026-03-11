#include "modbus_rtu.h"

void ModbusRTU::begin() {
    // RS485 yon kontrol pini
    pinMode(RS485_ENABLE_PIN, OUTPUT);
    setRxMode();

    // UART baslat
    RS485_SERIAL.begin(MODBUS_BAUD_RATE, MODBUS_CONFIG, RS485_RX_PIN, RS485_TX_PIN);

    DEBUG_SERIAL.println("[MODBUS] RS485 baslatildi");
    DEBUG_SERIAL.printf("[MODBUS] TX:%d RX:%d DE/RE:%d Baud:%d\n",
                        RS485_TX_PIN, RS485_RX_PIN, RS485_ENABLE_PIN, MODBUS_BAUD_RATE);
}

// --- RS485 Yon Kontrolu ---

void ModbusRTU::setTxMode() {
    digitalWrite(RS485_ENABLE_PIN, HIGH);
    delayMicroseconds(100);
}

void ModbusRTU::setRxMode() {
    digitalWrite(RS485_ENABLE_PIN, LOW);
}

// --- CRC-16 Modbus ---

uint16_t ModbusRTU::calculateCRC(const uint8_t* data, uint8_t length) {
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

// --- Veri Gonder ---

void ModbusRTU::sendRequest(uint8_t length) {
    flushRxBuffer();
    setTxMode();
    RS485_SERIAL.write(_txBuffer, length);
    RS485_SERIAL.flush();
    delayMicroseconds(500);  // Son bit'in iletimini bekle
    setRxMode();
    delay(MODBUS_INTER_FRAME);
}

// --- Yanit Oku ---

int ModbusRTU::receiveResponse(uint8_t expectedMinLen) {
    uint8_t bytesRead = 0;
    unsigned long startTime = millis();

    while (bytesRead < sizeof(_rxBuffer)) {
        if ((millis() - startTime) > MODBUS_TIMEOUT_MS) {
            break;
        }
        if (RS485_SERIAL.available() > 0) {
            _rxBuffer[bytesRead++] = RS485_SERIAL.read();
            startTime = millis();  // Her byte'ta timeout'u sifirla
        } else {
            // 3.5 karakter suresi kadar sessizlik = frame sonu
            // 9600 baud'da 1 karakter ~1.04ms, 3.5 karakter ~3.6ms
            if (bytesRead > 0) {
                delay(4);
                if (!RS485_SERIAL.available()) {
                    break;  // Frame tamamlandi
                }
            } else {
                delay(1);
            }
        }
    }
    return bytesRead;
}

// --- RX Buffer Temizle ---

void ModbusRTU::flushRxBuffer() {
    while (RS485_SERIAL.available()) {
        RS485_SERIAL.read();
    }
}

// --- FC 0x03: Read Holding Registers ---

ModbusError ModbusRTU::readHoldingRegisters(uint8_t slaveId, uint16_t startAddr,
                                             uint16_t count, uint16_t* buffer) {
    if (count == 0 || count > MODBUS_MAX_REGS) {
        return MB_ERR_BUFFER_OVERFLOW;
    }

    // Request olustur: [SlaveID][0x03][AddrHi][AddrLo][CountHi][CountLo][CRCLo][CRCHi]
    _txBuffer[0] = slaveId;
    _txBuffer[1] = FC_READ_HOLDING_REGISTERS;
    _txBuffer[2] = (startAddr >> 8) & 0xFF;
    _txBuffer[3] = startAddr & 0xFF;
    _txBuffer[4] = (count >> 8) & 0xFF;
    _txBuffer[5] = count & 0xFF;

    uint16_t crc = calculateCRC(_txBuffer, 6);
    _txBuffer[6] = crc & 0xFF;         // CRC Low
    _txBuffer[7] = (crc >> 8) & 0xFF;  // CRC High

    // Debug: gonderilen frame
    DEBUG_SERIAL.printf("[TX] ");
    for (int i = 0; i < 8; i++) {
        DEBUG_SERIAL.printf("%02X ", _txBuffer[i]);
    }
    DEBUG_SERIAL.println();

    // Gonder ve yanit al
    sendRequest(8);
    int rxLen = receiveResponse(5 + count * 2);

    // Debug: alinan frame
    DEBUG_SERIAL.printf("[RX] (%d bytes) ", rxLen);
    for (int i = 0; i < rxLen; i++) {
        DEBUG_SERIAL.printf("%02X ", _rxBuffer[i]);
    }
    DEBUG_SERIAL.println();

    if (rxLen == 0) {
        return MB_ERR_TIMEOUT;
    }

    // Exception kontrolu (Sofar: FC=0x90 yerine FC|0x80)
    if (rxLen >= 5 && (_rxBuffer[1] & 0x80)) {
        _lastException = _rxBuffer[2];
        DEBUG_SERIAL.printf("[MODBUS] Exception: 0x%02X (%s)\n",
                            _lastException, exceptionToString(_lastException));
        return MB_ERR_EXCEPTION;
    }

    // Minimum uzunluk kontrolu
    uint8_t expectedLen = 3 + count * 2 + 2;  // ID + FC + ByteCount + Data + CRC
    if (rxLen < expectedLen) {
        DEBUG_SERIAL.printf("[MODBUS] Kisa yanit: %d/%d byte\n", rxLen, expectedLen);
        return MB_ERR_INVALID_RESPONSE;
    }

    // CRC dogrula
    uint16_t rxCrc = _rxBuffer[rxLen - 2] | (_rxBuffer[rxLen - 1] << 8);
    uint16_t calcCrc = calculateCRC(_rxBuffer, rxLen - 2);
    if (rxCrc != calcCrc) {
        DEBUG_SERIAL.printf("[MODBUS] CRC hatasi: rx=0x%04X calc=0x%04X\n", rxCrc, calcCrc);
        return MB_ERR_CRC;
    }

    // Slave ID ve FC dogrula
    if (_rxBuffer[0] != slaveId || _rxBuffer[1] != FC_READ_HOLDING_REGISTERS) {
        return MB_ERR_INVALID_RESPONSE;
    }

    // Byte count dogrula
    uint8_t byteCount = _rxBuffer[2];
    if (byteCount != count * 2) {
        return MB_ERR_INVALID_RESPONSE;
    }

    // Register degerlerini parse et (big-endian)
    for (uint16_t i = 0; i < count; i++) {
        buffer[i] = (_rxBuffer[3 + i * 2] << 8) | _rxBuffer[4 + i * 2];
    }

    return MB_OK;
}

// --- FC 0x06: Write Single Register ---

ModbusError ModbusRTU::writeSingleRegister(uint8_t slaveId, uint16_t addr,
                                            uint16_t value) {
    // Request: [SlaveID][0x06][AddrHi][AddrLo][ValueHi][ValueLo][CRCLo][CRCHi]
    _txBuffer[0] = slaveId;
    _txBuffer[1] = FC_WRITE_SINGLE_REGISTER;
    _txBuffer[2] = (addr >> 8) & 0xFF;
    _txBuffer[3] = addr & 0xFF;
    _txBuffer[4] = (value >> 8) & 0xFF;
    _txBuffer[5] = value & 0xFF;

    uint16_t crc = calculateCRC(_txBuffer, 6);
    _txBuffer[6] = crc & 0xFF;
    _txBuffer[7] = (crc >> 8) & 0xFF;

    DEBUG_SERIAL.printf("[TX] ");
    for (int i = 0; i < 8; i++) {
        DEBUG_SERIAL.printf("%02X ", _txBuffer[i]);
    }
    DEBUG_SERIAL.println();

    sendRequest(8);
    int rxLen = receiveResponse(8);

    DEBUG_SERIAL.printf("[RX] (%d bytes) ", rxLen);
    for (int i = 0; i < rxLen; i++) {
        DEBUG_SERIAL.printf("%02X ", _rxBuffer[i]);
    }
    DEBUG_SERIAL.println();

    if (rxLen == 0) return MB_ERR_TIMEOUT;

    if (rxLen >= 5 && (_rxBuffer[1] & 0x80)) {
        _lastException = _rxBuffer[2];
        return MB_ERR_EXCEPTION;
    }

    if (rxLen < 8) return MB_ERR_INVALID_RESPONSE;

    // CRC dogrula
    uint16_t rxCrc = _rxBuffer[rxLen - 2] | (_rxBuffer[rxLen - 1] << 8);
    uint16_t calcCrc = calculateCRC(_rxBuffer, rxLen - 2);
    if (rxCrc != calcCrc) return MB_ERR_CRC;

    // Echo kontrolu: yanit, istegin birebir aynisi olmali
    if (memcmp(_txBuffer, _rxBuffer, 6) != 0) {
        return MB_ERR_INVALID_RESPONSE;
    }

    return MB_OK;
}

// --- FC 0x10: Write Multiple Registers ---

ModbusError ModbusRTU::writeMultipleRegisters(uint8_t slaveId, uint16_t startAddr,
                                               uint16_t count, const uint16_t* values) {
    if (count == 0 || count > 125) return MB_ERR_BUFFER_OVERFLOW;

    uint8_t byteCount = count * 2;

    // Request: [SlaveID][0x10][AddrHi][AddrLo][CountHi][CountLo][ByteCount][Data...][CRC]
    _txBuffer[0] = slaveId;
    _txBuffer[1] = FC_WRITE_MULTIPLE_REGISTERS;
    _txBuffer[2] = (startAddr >> 8) & 0xFF;
    _txBuffer[3] = startAddr & 0xFF;
    _txBuffer[4] = (count >> 8) & 0xFF;
    _txBuffer[5] = count & 0xFF;
    _txBuffer[6] = byteCount;

    for (uint16_t i = 0; i < count; i++) {
        _txBuffer[7 + i * 2] = (values[i] >> 8) & 0xFF;
        _txBuffer[8 + i * 2] = values[i] & 0xFF;
    }

    uint8_t frameLen = 7 + byteCount;
    uint16_t crc = calculateCRC(_txBuffer, frameLen);
    _txBuffer[frameLen] = crc & 0xFF;
    _txBuffer[frameLen + 1] = (crc >> 8) & 0xFF;
    frameLen += 2;

    DEBUG_SERIAL.printf("[TX] ");
    for (int i = 0; i < frameLen; i++) {
        DEBUG_SERIAL.printf("%02X ", _txBuffer[i]);
    }
    DEBUG_SERIAL.println();

    sendRequest(frameLen);
    int rxLen = receiveResponse(8);

    DEBUG_SERIAL.printf("[RX] (%d bytes) ", rxLen);
    for (int i = 0; i < rxLen; i++) {
        DEBUG_SERIAL.printf("%02X ", _rxBuffer[i]);
    }
    DEBUG_SERIAL.println();

    if (rxLen == 0) return MB_ERR_TIMEOUT;

    if (rxLen >= 5 && (_rxBuffer[1] & 0x80)) {
        _lastException = _rxBuffer[2];
        return MB_ERR_EXCEPTION;
    }

    if (rxLen < 8) return MB_ERR_INVALID_RESPONSE;

    uint16_t rxCrc = _rxBuffer[rxLen - 2] | (_rxBuffer[rxLen - 1] << 8);
    uint16_t calcCrc = calculateCRC(_rxBuffer, rxLen - 2);
    if (rxCrc != calcCrc) return MB_ERR_CRC;

    // Yanit: [SlaveID][0x10][AddrHi][AddrLo][CountHi][CountLo][CRC]
    if (_rxBuffer[0] != slaveId || _rxBuffer[1] != FC_WRITE_MULTIPLE_REGISTERS) {
        return MB_ERR_INVALID_RESPONSE;
    }

    return MB_OK;
}

// --- Hata Aciklamalari ---

const char* ModbusRTU::errorToString(ModbusError err) {
    switch (err) {
        case MB_OK:                  return "Basarili";
        case MB_ERR_TIMEOUT:         return "Timeout - yanit yok";
        case MB_ERR_CRC:             return "CRC hatasi";
        case MB_ERR_EXCEPTION:       return "Modbus exception";
        case MB_ERR_INVALID_RESPONSE: return "Gecersiz yanit";
        case MB_ERR_BUFFER_OVERFLOW: return "Buffer tasma";
        default:                     return "Bilinmeyen hata";
    }
}

const char* ModbusRTU::exceptionToString(uint8_t code) {
    switch (code) {
        case MB_EX_ILLEGAL_FUNCTION: return "Gecersiz fonksiyon kodu";
        case MB_EX_ILLEGAL_ADDRESS:  return "Gecersiz adres";
        case MB_EX_ILLEGAL_DATA:     return "Gecersiz veri";
        case MB_EX_DEVICE_FAULT:     return "Cihaz hatasi";
        case MB_EX_DEVICE_BUSY:      return "Cihaz mesgul";
        default:                     return "Bilinmeyen exception";
    }
}
