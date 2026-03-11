# Sofar Inverter - ESP32 Modbus RTU Reader/Writer

Sofar (SOFARSOLAR) güneş enerjisi invertörleri ile **Modbus RTU** protokolü üzerinden RS485 haberleşme sağlayan ESP32 tabanlı firmware projesi. Gerçek zamanlı veri okuma, register yazma ve invertör parametre yönetimi için interaktif seri konsol arayüzü sunar.

## Özellikler

- **Modbus RTU Haberleşme** — FC 0x03 (Read Holding Registers), FC 0x06 (Write Single Register), FC 0x10 (Write Multiple Registers)
- **Gerçek Zamanlı Veri Okuma** — Sistem durumu, şebeke çıkışı (3 faz R/S/T), PV giriş (4 kanal), batarya bilgileri, enerji istatistikleri
- **Register Yazma** — Shadow + Enable blok pattern desteği ile invertör parametre konfigürasyonu
- **Interaktif Seri Konsol** — Komut tabanlı arayüz: okuma, yazma, tarama, otomatik polling
- **Retry Mekanizması** — Otomatik yeniden deneme, hata yönetimi, Modbus exception raporlama
- **CRC-16 Doğrulama** — Gönderilen ve alınan tüm frame'lerde CRC kontrolü

## Donanım Gereksinimleri

| Bileşen | Açıklama |
|---------|----------|
| **MCU** | ESP32 DevKit |
| **RS485 Modül** | MAX485 / SP3485 (DE/RE kontrollü) |
| **İnvertör** | Sofar güneş enerjisi invertörü (Modbus G3 protokol desteği) |
| **Kablo** | RS485 A+/B- bağlantısı |

### Pin Bağlantıları

| ESP32 Pin | Fonksiyon | Açıklama |
|-----------|-----------|----------|
| GPIO17 | TX | RS485 modül DI |
| GPIO16 | RX | RS485 modül RO |
| GPIO5 | DE/RE | RS485 yön kontrolü (HIGH=TX, LOW=RX) |
| GPIO21 | LED | Modbus aktivite göstergesi (opsiyonel) |
| GPIO2 | LED | Dahili LED (opsiyonel) |

## Yazılım Gereksinimleri

- [PlatformIO](https://platformio.org/) (VS Code extension veya CLI)
- Arduino Framework (ESP32)

### Kütüphaneler

| Kütüphane | Versiyon | Kullanım |
|-----------|----------|----------|
| ArduinoJson | ^6.21.5 | JSON serileştirme (gelecek kullanım için) |

## Kurulum

```bash
# Projeyi klonla
git clone https://github.com/gucluceyhan/sofar-inverter-esp32.git
cd sofar-inverter-esp32

# PlatformIO ile derle
pio run

# ESP32'ye yükle
pio run --target upload

# Seri monitörü aç (9600 baud)
pio device monitor --baud 9600
```

## Kullanım

ESP32'ye bağlandıktan sonra seri konsol üzerinden komutlar girebilirsiniz:

```
============ SOFAR INVERTOR - KOMUT MENUSU ============
  all          - Tum verileri oku ve yazdir
  sys          - Sistem bilgisi
  grid         - Sebeke cikis verileri
  pv           - PV giris verileri
  bat          - Batarya bilgileri
  energy       - Enerji istatistikleri
  read XXXX    - Tek register oku (hex adres, orn: read 0404)
  readn XXXX N - N adet register oku (orn: readn 0484 10)
  write XXXX YYYY - Tek register yaz FC 0x06 (hex)
  writem XXXX V1 V2.. - Coklu register yaz FC 0x10 (hex)
  scan         - Slave ID tara (1-247)
  auto         - Otomatik okuma ac/kapat
  slave N      - Slave ID degistir (orn: slave 1)
  help         - Bu menuyu goster
=======================================================
```

### Örnek Komutlar

```bash
# Sistem durumunu oku
sys

# Tüm verileri bir kerede oku
all

# Belirli bir register'ı oku (hex adres)
read 0404

# 10 adet ardışık register oku
readn 0484 10

# İnvertör tarih/saat ayarla (2026-03-10 15:30:00)
writem 1004 001A 0003 000A 000F 001E 0000 0001

# Enerji depolama modunu değiştir (2 = Zamanli sarj/desarj)
writem 1110 0002

# Slave ID taraması yap (1-247)
scan

# 5 saniyelik otomatik polling aç/kapat
auto
```

## Proje Yapısı

```
sofar-esp32/
├── include/
│   ├── config.h              # Pin tanımları, Modbus ayarları, zamanlama
│   ├── modbus_rtu.h          # Modbus RTU protokol sınıfı (header)
│   ├── sofar_inverter.h      # İnvertör veri yapıları ve okuma sınıfı (header)
│   └── sofar_registers.h     # Sofar Modbus register haritası (V1.39)
├── src/
│   ├── main.cpp              # Ana program: komut işleyici, setup/loop
│   ├── modbus_rtu.cpp         # Modbus RTU implementasyonu (FC 0x03/0x06/0x10)
│   └── sofar_inverter.cpp     # İnvertör veri okuma/yazma/yazdırma
├── docs/
│   └── sofar_writable_registers.md  # Yazılabilir register haritası ve test sonuçları
├── platformio.ini             # PlatformIO yapılandırması
└── .gitignore
```

## Modbus Ayarları

| Parametre | Değer |
|-----------|-------|
| Baud Rate | 9600 |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |
| Slave ID | 1 (varsayılan) |
| Timeout | 1000 ms |
| Max Retry | 3 |
| Max Register/Okuma | 60 |

## Okunan Veri Kategorileri

### Sistem Bilgisi (0x0404 - 0x043E)
Çalışma durumu, hata kodları, sıcaklıklar (ortam, radyatör, modül), fan hızı, üretim/çalışma süreleri, izolasyon empedansı

### Şebeke Çıkışı (0x0484 - 0x04BF)
Şebeke frekansı, 3 fazlı (R/S/T) gerilim/akım/güç, toplam aktif/reaktif/görünen güç, PCC güç, güç faktörü, verim

### PV Girişi (0x0584 - 0x05C4)
4 kanallı PV gerilim/akım/güç, toplam PV gücü

### Batarya (0x0604 - 0x066B)
Gerilim, akım (şarj/deşarj yönü), güç, sıcaklık, SOC, SOH, döngü sayısı, paket bilgileri

### Enerji İstatistikleri (0x0684 - 0x069B)
Günlük/toplam üretim, tüketim, satın alınan, satılan, batarya şarj/deşarj enerji miktarları (kWh)

## Yazma Kuralları

Sofar invertörlerde yazma işlemi iki kategoriye ayrılır:

### 1. Enable Blok Yazma (Shadow + Enable Pattern)
Tüm shadow register'lar + enable register **tek FC 0x10 komutu** içinde gönderilmelidir. Enable register'ı ayrı yazmak **çalışmaz**.

Doğrulanmış enable blokları:
- Tarih/Saat (0x1004-0x100A)
- RS485 Ayarları (0x100B-0x100F)
- Batarya Parametreleri (0x1044-0x1053)
- Zamanlı Şarj/Deşarj (0x1111-0x111F)
- Time-Sharing (0x1120-0x112F)
- Off-Grid (0x1144-0x1146)

### 2. Bireysel Register Yazma
Enable register gerektirmeyen, tek tek yazılabilen register'lar:
- Remote kontrol (0x1104-0x110B)
- Enerji depolama modu (0x1110)
- PCC güç algılama (0x1060), kuru kontak (0x1064), PCC bias (0x1069) vb.

> **Önemli:** FC 0x06 (Write Single Register) bu invertörde **çalışmaz**. Her zaman FC 0x10 (Write Multiple Registers) kullanılmalıdır.

## Referans Dokümanlar

- Sofar Modbus-G3 Protocol V1.39 (Mart 2025)
- Detaylı yazılabilir register haritası ve test sonuçları: [`docs/sofar_writable_registers.md`](docs/sofar_writable_registers.md)

## Lisans

Bu proje özel kullanım amaçlıdır.
