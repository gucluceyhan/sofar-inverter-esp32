#pragma once

#include <Arduino.h>

// ============================================================
// Sofar Inverter Modbus Register Haritasi
// Kaynak: Sofar_Inverter_MODBUS_V1.39_EN.pdf
// ============================================================

// --- 5.0 Genel Veri Alani ---
#define REG_PROTOCOL_VERSION        0x0044  // BCD16, RO

// --- 5.1.1 Sistem Bilgisi 1 (Gercek Zamanli, RO) ---
#define REG_SYSTEM_STATE            0x0404  // U16 - Sistem calisma durumu
#define REG_FAULT_1                 0x0405  // U16
#define REG_FAULT_2                 0x0406  // U16
#define REG_FAULT_3                 0x0407  // U16
#define REG_FAULT_4                 0x0408  // U16
#define REG_FAULT_5                 0x0409  // U16
#define REG_GRID_WAIT_TIME          0x0417  // U16 - Sebeke baglanti bekleme suresi (1S)
#define REG_AMBIENT_TEMP_1          0x0418  // S16 - Ortam sicakligi 1 (1C)
#define REG_AMBIENT_TEMP_2          0x0419  // S16 - Ortam sicakligi 2 (1C)
#define REG_RADIATOR_TEMP_1         0x041A  // S16 - Radyator sicakligi 1 (1C)
#define REG_RADIATOR_TEMP_2         0x041B  // S16 - Radyator sicakligi 2 (1C)
#define REG_MODULE_TEMP_1           0x0420  // S16 - Modul sicakligi 1 (1C)
#define REG_MODULE_TEMP_2           0x0421  // S16 - Modul sicakligi 2 (1C)
#define REG_GEN_TIME_TODAY          0x0426  // U16 - Bugunun uretim suresi (1min)
#define REG_TOTAL_GEN_TIME_H        0x0427  // U32 - Toplam uretim suresi (1min)
#define REG_TOTAL_RUN_TIME_H        0x0429  // U32 - Toplam calisma suresi (1min)
#define REG_INSULATION_IMP          0x042B  // U16 - Izolasyon empedansi (1kOhm)
#define REG_SYS_TIME_YEAR           0x042C  // U16 - Sistem zamani (yil, +2000)
#define REG_SYS_TIME_MONTH          0x042D  // U16
#define REG_SYS_TIME_DAY            0x042E  // U16
#define REG_SYS_TIME_HOUR           0x042F  // U16
#define REG_SYS_TIME_MINUTE         0x0430  // U16
#define REG_SYS_TIME_SECOND         0x0431  // U16
#define REG_FAN_SPEED               0x043E  // U16 - Fan hizi (1r/min)

// --- 5.1.2 Sistem Bilgisi 2 (RO) ---
#define REG_SERIAL_NUM_1            0x0445  // ASCII (8 register = 16 karakter)
#define REG_HW_VERSION              0x044D  // ASCII
#define REG_STATUS_1                0x0477  // U16
#define REG_STATUS_2                0x0478  // U16

// --- 5.1.3 Sebekeye Bagli Cikis (Gercek Zamanli, RO) ---
#define REG_GRID_FREQ               0x0484  // U16 - Sebeke frekansi (0.01Hz)
#define REG_TOTAL_ACTIVE_POWER      0x0485  // S16 - Toplam aktif guc (0.01kW)
#define REG_TOTAL_REACTIVE_POWER    0x0486  // S16 - Toplam reaktif guc (0.01kVar)
#define REG_TOTAL_APPARENT_POWER    0x0487  // S16 - Toplam gorunen guc (0.01kVA)
#define REG_TOTAL_PCC_ACTIVE        0x0488  // S16 - Toplam PCC aktif guc (0.01kW)

// R fazı
#define REG_GRID_VOLTAGE_R          0x048D  // U16 - Sebeke gerilimi R (0.1V)
#define REG_OUTPUT_CURRENT_R        0x048E  // U16 - Cikis akimi R (0.01A)
#define REG_OUTPUT_ACTIVE_R         0x048F  // S16 - Cikis aktif gucu R (0.01kW)
#define REG_OUTPUT_REACTIVE_R       0x0490  // S16 - Cikis reaktif gucu R (0.01kVar)
#define REG_POWER_FACTOR_R          0x0491  // S16 - Guc faktoru R (0.001)

// S fazı
#define REG_GRID_VOLTAGE_S          0x0498  // U16 - Sebeke gerilimi S (0.1V)
#define REG_OUTPUT_CURRENT_S        0x0499  // U16 - Cikis akimi S (0.01A)
#define REG_OUTPUT_ACTIVE_S         0x049A  // S16 - Cikis aktif gucu S (0.01kW)

// T fazı
#define REG_GRID_VOLTAGE_T          0x04A3  // U16 - Sebeke gerilimi T (0.1V)
#define REG_OUTPUT_CURRENT_T        0x04A4  // U16 - Cikis akimi T (0.01A)
#define REG_OUTPUT_ACTIVE_T         0x04A5  // S16 - Cikis aktif gucu T (0.01kW)

// Hat gerilimleri
#define REG_LINE_VOLTAGE_L1         0x04BA  // U16 - Faz arasi gerilim R/S (0.1V)
#define REG_LINE_VOLTAGE_L2         0x04BB  // U16 - Faz arasi gerilim S/T (0.1V)
#define REG_LINE_VOLTAGE_L3         0x04BC  // U16 - Faz arasi gerilim T/R (0.1V)

// Toplam guc faktoru
#define REG_TOTAL_POWER_FACTOR      0x04BD  // S16 - (0.001)

// Guc uretim verimi
#define REG_GEN_EFFICIENCY          0x04BF  // U16 - (0.01%)

// --- 5.1.4 Sebekeden Bagimsiz Cikis (Yuk, RO) ---
#define REG_LOAD_ACTIVE_POWER       0x0504  // S16 - Yuk aktif gucu (0.01kW)
#define REG_LOAD_REACTIVE_POWER     0x0505  // S16 - Yuk reaktif gucu (0.01kVar)
#define REG_LOAD_APPARENT_POWER     0x0506  // S16 - Yuk gorunen gucu (0.01kVA)
#define REG_OUTPUT_FREQ             0x0507  // U16 - Cikis frekansi (0.01Hz)

// --- 5.1.5 PV Giris Kanallari (RO) ---
#define REG_PV1_VOLTAGE             0x0584  // U16 - PV1 gerilimi (0.1V)
#define REG_PV1_CURRENT             0x0585  // S16 - PV1 akimi (0.01A)
#define REG_PV1_POWER               0x0586  // U16 - PV1 gucu (0.01kW)
#define REG_PV2_VOLTAGE             0x0587  // U16
#define REG_PV2_CURRENT             0x0588  // S16
#define REG_PV2_POWER               0x0589  // U16
#define REG_PV3_VOLTAGE             0x058A  // U16
#define REG_PV3_CURRENT             0x058B  // S16
#define REG_PV3_POWER               0x058C  // U16
#define REG_PV4_VOLTAGE             0x058D  // U16
#define REG_PV4_CURRENT             0x058E  // S16
#define REG_PV4_POWER               0x058F  // U16

// --- 5.1.6 Toplam PV Gucu (RO) ---
#define REG_TOTAL_PV_POWER          0x05C4  // U16 - (0.1kW)

// --- 5.1.7 Batarya Temel Bilgi (RO) ---
#define REG_BAT1_VOLTAGE            0x0604  // U16 - Batarya 1 gerilimi (0.1V)
#define REG_BAT1_CURRENT            0x0605  // S16 - Batarya 1 akimi (0.01A, +sarj/-desarj)
#define REG_BAT1_POWER              0x0606  // S16 - Batarya 1 gucu (0.01kW)
#define REG_BAT1_TEMP               0x0607  // S16 - Batarya 1 sicakligi (1C)
#define REG_BAT1_SOC                0x0608  // U16 - Batarya 1 SOC (1%)
#define REG_BAT1_SOH                0x0609  // U16 - Batarya 1 SOH (1%)
#define REG_BAT1_CYCLES             0x060A  // U16 - Batarya 1 dongu sayisi

// Batarya genel bilgiler
#define REG_BAT_TOTAL_POWER         0x0667  // S16 - Toplam batarya gucu (0.1kW)
#define REG_BAT_AVG_SOC             0x0668  // U16 - Ortalama SOC (1%)
#define REG_BAT_AVG_SOH             0x0669  // U16 - Ortalama SOH (1%)
#define REG_BAT_PACK_COUNT          0x066A  // U16 - Aktif batarya paketi sayisi
#define REG_BAT_TOTAL_CAPACITY      0x066B  // U16 - Toplam kapasite (1Ah)

// --- 5.1.9 Elektrik Istatistikleri (RO, U32 = 2 register) ---
#define REG_DAILY_GEN               0x0684  // U32 - Gunluk uretim (0.01kWh)
#define REG_TOTAL_GEN               0x0686  // U32 - Toplam uretim (0.1kWh)
#define REG_DAILY_LOAD              0x0688  // U32 - Gunluk tuketim (0.01kWh)
#define REG_TOTAL_LOAD              0x068A  // U32 - Toplam tuketim (0.1kWh)
#define REG_DAILY_BOUGHT            0x068C  // U32 - Gunluk satin alinan (0.01kWh)
#define REG_TOTAL_BOUGHT            0x068E  // U32 - Toplam satin alinan (0.1kWh)
#define REG_DAILY_SOLD              0x0690  // U32 - Gunluk satilan (0.01kWh)
#define REG_TOTAL_SOLD              0x0692  // U32 - Toplam satilan (0.1kWh)
#define REG_DAILY_BAT_CHARGE        0x0694  // U32 - Gunluk batarya sarj (0.01kWh)
#define REG_TOTAL_BAT_CHARGE        0x0696  // U32 - Toplam batarya sarj (0.1kWh)
#define REG_DAILY_BAT_DISCHARGE     0x0698  // U32 - Gunluk batarya desarj (0.01kWh)
#define REG_TOTAL_BAT_DISCHARGE     0x069A  // U32 - Toplam batarya desarj (0.1kWh)

// --- Sistem Durumu Aciklamalari ---
static const char* SYSTEM_STATE_NAMES[] = {
    "Bekleme",              // 0
    "Algilama",             // 1
    "Sebekeye bagli",       // 2
    "Acil guc beslemesi",   // 3
    "Kurtarilabilir hata",  // 4
    "Kalici hata",          // 5
    "Guncelleme",           // 6
    "Kendi kendine sarj",   // 7
    "SVG durumu",           // 8
    "PID durumu",           // 9
    "Guc sinirlandirma",    // 10
    "Bekleme izleme"        // 11
};
#define SYSTEM_STATE_COUNT  12

// --- Batarya Durumu Aciklamalari ---
static const char* BATTERY_STATE_NAMES[] = {
    "Bilinmiyor",       // 0
    "Sarj oluyor",      // 1
    "Desarj oluyor",    // 2
    "Uyku",             // 3
    "Hata",             // 4
    "Kayip azaltma"     // 5
};
#define BATTERY_STATE_COUNT 6
