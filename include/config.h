#pragma once

// ============================================================
// Sofar Inverter Modbus RTU - ESP32 Konfigurasyonu
// Zeus 2.0 pin uyumlu
// ============================================================

// --- RS485 Pin Tanimlari (Zeus 2.0 ile ayni) ---
#define RS485_TX_PIN        17
#define RS485_RX_PIN        16
#define RS485_ENABLE_PIN    5       // DE/RE kontrol: HIGH=TX, LOW=RX
#define RS485_SERIAL        Serial2

// --- Modbus Ayarlari ---
#define MODBUS_BAUD_RATE    9600
#define MODBUS_CONFIG       SERIAL_8N1
#define MODBUS_SLAVE_ID     1       // Invertor varsayilan slave adresi
#define MODBUS_TIMEOUT_MS   1000    // Yanit bekleme suresi
#define MODBUS_MAX_RETRIES  3       // Hata durumunda tekrar deneme
#define MODBUS_RETRY_DELAY  100     // Tekrar denemeler arasi bekleme (ms)
#define MODBUS_INTER_FRAME  50      // Frame arasi bekleme (ms)
#define MODBUS_MAX_REGS     60      // Sofar: tek seferde max 60 register

// --- Debug Serial ---
#define DEBUG_SERIAL        Serial
#define DEBUG_BAUD_RATE     9600

// --- LED Pinleri (opsiyonel, Zeus 2.0 uyumlu) ---
#define LED_MODBUS_PIN      21      // Kirmizi - Modbus aktivite
#define LED_BUILTIN_PIN     2       // Dahili LED

// --- Zamanlama ---
#define AUTO_POLL_INTERVAL  5000    // Otomatik okuma araligi (ms)
