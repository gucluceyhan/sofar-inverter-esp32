# Sofar SP2ES 10kW Hibrit Invertor — Tam Register Haritasi

> Cihaz: SP2ES110N95024 | HW: V0010V120005 | Slave ID: 1 | Baud: 9600 8N1
> Tarama Tarihi: 2026-03-27 | Yontem: ESP32 Modbus RTU fullscan + scanwrite
> Protokol Referans: Sofar Modbus-G3 V1.39, V1.29 Excel, HYD ES V1.04
> Guncelleme: 2026-03-27 — Excel ve HYD ES protokolunden elde edilen register aciklamalari eklendi

---

## Tarama Ozeti

| Metrik | Deger |
|--------|-------|
| Taranan aralik | 0x0000 - 0x1FFF (8192 register) |
| Okunabilir (OK) | 2402 |
| Exception 0x02 (gecersiz adres) | 4313 |
| Exception 0x01 (gecersiz FC) | 1472 |
| Exception 0x03 (gecersiz veri) | 5 |
| Timeout | 0 |
| Bireysel yazilabilir (RW) | 22 |
| Salt okunur (RO) | 2378 |
| Tarama suresi (fullscan) | 1016 sn |
| Tarama suresi (scanwrite) | 803 sn |

---

## Okunabilir Bloklar

| Aralik | Adet | Aciklama | RW | Kaynak |
|--------|------|----------|----|--------|
| 0x0040 - 0x007F | 64 | Genel veri alani (adres mask, protokol ver.) | RO | V1.39 |
| 0x0400 - 0x04BF | 192 | Sistem bilgisi + Sebeke cikisi (3-faz) | RO | V1.39 |
| 0x0500 - 0x053F | 64 | Yuk cikisi (off-grid V/I/P/F) | RO | V1.39 |
| 0x0580 - 0x077F | 512 | PV giris + Batarya + Enerji istatistik | RO | V1.39 |
| 0x0800 - 0x0A3F | 576 | **Guvenlik Parametreleri** (9 alt blok) | RO | V1.29 Excel |
| 0x1000 - 0x10BF | 192 | Konfigurasyon (zaman, RS485, batarya, fan) | RW (kismi) | V1.39 |
| 0x1100 - 0x11FF | 256 | Schedule (EMS mod, zamanli sarj, time-sharing) | RW (kismi) | V1.39 |
| 0x1300 - 0x133F | 64 | Italya Autotest Sonuclari (salt okunur) | RO | V1.29 Excel |
| 0x1480 - 0x1661 | 482 | Port Sicaklik + Batarya Cluster Verileri | RO | V1.33+ |

---

## 1. Genel Veri Alani (0x0040-0x007F) — RO

| Adres | Aciklama | Tip | Birim |
|-------|----------|-----|-------|
| 0x0040-0x0043 | AddressMask_General1 | U64 | — |
| 0x0044 | Protokol versiyonu | BCD16 | — |
| 0x0045-0x007F | Rezerve / Mask alanlari | — | — |

---

## 2. Sistem Bilgisi + Sebeke Cikisi (0x0400-0x04BF) — RO

### 2.1 Sistem Bilgisi (0x0400-0x047F)

| Adres | Aciklama | Tip | Birim | Olcek |
|-------|----------|-----|-------|-------|
| 0x0404 | Sistem calisma durumu | U16 | — | 0=Bekleme, 2=Sebekeye bagli, 3=EPS, 4=Hata |
| 0x0405-0x0409 | Hata kodlari 1-5 | U16 | — | Bit field |
| 0x0418 | Ortam sicakligi 1 | S16 | C | 1 |
| 0x0419 | Ortam sicakligi 2 | S16 | C | 1 |
| 0x041A | Radyator sicakligi 1 | S16 | C | 1 |
| 0x041B | Radyator sicakligi 2 | S16 | C | 1 |
| 0x0420 | Modul sicakligi 1 | S16 | C | 1 |
| 0x0421 | Modul sicakligi 2 | S16 | C | 1 |
| 0x0426 | Bugunun uretim suresi | U16 | dk | 1 |
| 0x0427-0x0428 | Toplam uretim suresi | U32 | dk | 1 |
| 0x0429-0x042A | Toplam calisma suresi | U32 | dk | 1 |
| 0x042B | Izolasyon empedansi | U16 | kOhm | 1 |
| 0x042C-0x0431 | Sistem zamani (yil/ay/gun/saat/dk/sn) | U16 | — | yil+2000 |
| 0x043E | Fan hizi | U16 | r/min | 1 |
| 0x0445-0x044C | Seri numarasi | ASCII | — | 8 reg = 16 char |
| 0x044D-0x0452 | HW versiyonu | ASCII | — | — |

### 2.2 Sebeke Cikisi (0x0484-0x04BF)

| Adres | Aciklama | Tip | Birim | Olcek |
|-------|----------|-----|-------|-------|
| 0x0484 | Sebeke frekansi | U16 | Hz | 0.01 |
| 0x0485 | Toplam aktif guc | S16 | kW | 0.01 |
| 0x0486 | Toplam reaktif guc | S16 | kVar | 0.01 |
| 0x0487 | Toplam gorunen guc | S16 | kVA | 0.01 |
| 0x0488 | Toplam PCC aktif guc | S16 | kW | 0.01 |
| 0x048D | R-faz gerilim | U16 | V | 0.1 |
| 0x048E | R-faz akim | U16 | A | 0.01 |
| 0x048F | R-faz aktif guc | S16 | kW | 0.01 |
| 0x0498 | S-faz gerilim | U16 | V | 0.1 |
| 0x0499 | S-faz akim | U16 | A | 0.01 |
| 0x049A | S-faz aktif guc | S16 | kW | 0.01 |
| 0x04A3 | T-faz gerilim | U16 | V | 0.1 |
| 0x04A4 | T-faz akim | U16 | A | 0.01 |
| 0x04A5 | T-faz aktif guc | S16 | kW | 0.01 |
| 0x04BD | Toplam guc faktoru | S16 | — | 0.001 |
| 0x04BF | Uretim verimi | U16 | % | 0.01 |

---

## 3. Yuk Cikisi / Off-Grid (0x0500-0x053F) — RO

| Adres | Aciklama | Tip | Birim | Olcek |
|-------|----------|-----|-------|-------|
| 0x0504 | Yuk aktif gucu | S16 | kW | 0.01 |
| 0x0505 | Yuk reaktif gucu | S16 | kVar | 0.01 |
| 0x0506 | Yuk gorunen gucu | S16 | kVA | 0.01 |
| 0x0507 | Cikis frekansi | U16 | Hz | 0.01 |
| 0x050A-0x0520 | Faz bazli yuk V/I/P | U16/S16 | V/A/kW | 0.1/0.01 |

---

## 4. PV + Batarya + Enerji (0x0580-0x077F) — RO

### 4.1 PV Giris (0x0584-0x05C4)

| Adres | Aciklama | Tip | Birim | Olcek |
|-------|----------|-----|-------|-------|
| 0x0584 | PV1 gerilim | U16 | V | 0.1 |
| 0x0585 | PV1 akim | S16 | A | 0.01 |
| 0x0586 | PV1 guc | U16 | kW | 0.01 |
| 0x0587-0x058F | PV2-PV4 (ayni format) | — | — | — |
| 0x05C4 | Toplam PV gucu | U16 | kW | 0.1 |

### 4.2 Batarya (0x0604-0x066B)

| Adres | Aciklama | Tip | Birim | Olcek |
|-------|----------|-----|-------|-------|
| 0x0604 | Batarya 1 gerilim | U16 | V | 0.1 |
| 0x0605 | Batarya 1 akim | S16 | A | 0.01 (+sarj/-desarj) |
| 0x0606 | Batarya 1 guc | S16 | kW | 0.01 |
| 0x0607 | Batarya 1 sicaklik | S16 | C | 1 |
| 0x0608 | Batarya 1 SOC | U16 | % | 1 |
| 0x0609 | Batarya 1 SOH | U16 | % | 1 |
| 0x060A | Batarya 1 dongu sayisi | U16 | — | 1 |
| 0x0667 | Toplam batarya gucu | S16 | kW | 0.1 |
| 0x0668 | Ortalama SOC | U16 | % | 1 |
| 0x0669 | Ortalama SOH | U16 | % | 1 |
| 0x066A | Aktif batarya paketi sayisi | U16 | — | 1 |
| 0x066B | Toplam kapasite | U16 | Ah | 1 |

### 4.3 Enerji Istatistikleri (0x0684-0x069B)

| Adres | Aciklama | Tip | Birim | Olcek |
|-------|----------|-----|-------|-------|
| 0x0684-0x0685 | Gunluk uretim | U32 | kWh | 0.01 |
| 0x0686-0x0687 | Toplam uretim | U32 | kWh | 0.1 |
| 0x0688-0x0689 | Gunluk tuketim | U32 | kWh | 0.01 |
| 0x068A-0x068B | Toplam tuketim | U32 | kWh | 0.1 |
| 0x068C-0x068D | Gunluk alinan (grid) | U32 | kWh | 0.01 |
| 0x068E-0x068F | Toplam alinan | U32 | kWh | 0.1 |
| 0x0690-0x0691 | Gunluk satilan (grid) | U32 | kWh | 0.01 |
| 0x0692-0x0693 | Toplam satilan | U32 | kWh | 0.1 |
| 0x0694-0x0695 | Gunluk batarya sarj | U32 | kWh | 0.01 |
| 0x0696-0x0697 | Toplam batarya sarj | U32 | kWh | 0.1 |
| 0x0698-0x0699 | Gunluk batarya desarj | U32 | kWh | 0.01 |
| 0x069A-0x069B | Toplam batarya desarj | U32 | kWh | 0.1 |

---

## 5. Guvenlik Parametre Alani (0x0800-0x0A3F) — RO

> **Kaynak:** V1.29 Excel "Address Description" sayfasi. V1.39 PDF'de dokumante edilmemis.
> Bu bolge ulke bazli grid kod gereksinimlerini icerir (voltaj/frekans koruma, ride-through, reaktif guc).

### 5.1 Baslangic Parametreleri (0x0800-0x083F)

| Adres | Aciklama | Tip | Birim | Olcek |
|-------|----------|-----|-------|-------|
| 0x0800 | ConnectWaitTime — Baglanti bekleme suresi | U16 | sn | 1 |
| 0x0801 | PowerUpSpeed — Guc yukselme hizi | U16 | %Pn/min | 1 |
| 0x0802 | ReconnectWaitTime — Yeniden baglanti bekleme | U16 | sn | 1 |
| 0x0803 | ReconnectPowerUpSpeed | U16 | %Pn/min | 1 |
| 0x0804 | VoltHighLimit — Ust voltaj limiti | U16 | V | 0.1 |
| 0x0805 | VoltLowLimit — Alt voltaj limiti | U16 | V | 0.1 |
| 0x0806 | FreqHighLimit — Ust frekans limiti | U16 | Hz | 0.01 |
| 0x0807 | FreqLowLimit — Alt frekans limiti | U16 | Hz | 0.01 |
| 0x0808 | ReconnectVoltHighLimit | U16 | V | 0.1 |
| 0x0809 | ReconnectVoltLowLimit | U16 | V | 0.1 |
| 0x080A | ReconnectFreqHighLimit | U16 | Hz | 0.01 |
| 0x080B | ReconnectFreqLowLimit | U16 | Hz | 0.01 |

### 5.2 Voltaj Koruma (0x0840-0x087F) — 3 Kademeli OVP/UVP

| Adres | Aciklama | Tip | Birim | Olcek |
|-------|----------|-----|-------|-------|
| 0x0840 | VoltageConfig — Voltaj koruma konfigurasyonu | U16 | — | — |
| 0x0841 | RatedVoltage — Nominal voltaj | U16 | V | 0.1 |
| 0x0842 | 1. Kademe OVP degeri | U16 | V | 0.1 |
| 0x0843 | 1. Kademe OVP suresi | U16 | ms | 10 |
| 0x0844 | 2. Kademe OVP degeri | U16 | V | 0.1 |
| 0x0845 | 2. Kademe OVP suresi | U16 | ms | 10 |
| 0x0846 | 3. Kademe OVP degeri | U16 | V | 0.1 |
| 0x0847 | 3. Kademe OVP suresi | U16 | ms | 10 |
| 0x0848 | 1. Kademe UVP degeri | U16 | V | 0.1 |
| 0x0849 | 1. Kademe UVP suresi | U16 | ms | 10 |
| 0x084A | 2. Kademe UVP degeri | U16 | V | 0.1 |
| 0x084B | 2. Kademe UVP suresi | U16 | ms | 10 |
| 0x084C | 3. Kademe UVP degeri | U16 | V | 0.1 |
| 0x084D | 3. Kademe UVP suresi | U16 | ms | 10 |
| 0x084E | 10dk OVP degeri | U16 | V | 0.1 |

### 5.3 Frekans Koruma (0x0880-0x08BF) — 3 Kademeli OFP/UFP + RoCoF

| Adres | Aciklama | Tip | Birim | Olcek |
|-------|----------|-----|-------|-------|
| 0x0880 | FrequencyConfig | U16 | — | — |
| 0x0881 | RatedFrequency — Nominal frekans | U16 | Hz | 0.01 |
| 0x0882 | 1. Kademe OFP degeri | U16 | Hz | 0.01 |
| 0x0883 | 1. Kademe OFP suresi | U16 | ms | 10 |
| 0x0884 | 2. Kademe OFP degeri | U16 | Hz | 0.01 |
| 0x0885 | 2. Kademe OFP suresi | U16 | ms | 10 |
| 0x0886 | 3. Kademe OFP degeri | U16 | Hz | 0.01 |
| 0x0887 | 3. Kademe OFP suresi | U16 | ms | 10 |
| 0x0888 | 1. Kademe UFP degeri | U16 | Hz | 0.01 |
| 0x0889 | 1. Kademe UFP suresi | U16 | ms | 10 |
| 0x088A | 2. Kademe UFP degeri | U16 | Hz | 0.01 |
| 0x088B | 2. Kademe UFP suresi | U16 | ms | 10 |
| 0x088C | 3. Kademe UFP degeri | U16 | Hz | 0.01 |
| 0x088D | 3. Kademe UFP suresi | U16 | ms | 10 |
| 0x0894 | RoCoF max limit | U16 | Hz/s | 0.01 |
| 0x0895 | RoCoF koruma suresi | U16 | ms | 10 |

### 5.4 DCI Koruma (0x08C0-0x08FF) — DC Kacak Akim

| Adres | Aciklama | Tip | Birim | Olcek |
|-------|----------|-----|-------|-------|
| 0x08C0 | DCI Config | U16 | — | — |
| 0x08C1 | 1. Kademe DCI koruma degeri | U16 | mA | 1 |
| 0x08C2 | 1. Kademe DCI koruma suresi | U16 | ms | 10 |
| 0x08C3 | 2. Kademe DCI koruma degeri | U16 | mA | 1 |
| 0x08C4 | 2. Kademe DCI koruma suresi | U16 | ms | 10 |
| 0x08C6 | 3. Kademe DCI koruma suresi | U16 | ms | 10 |
| 0x08CA | 1. Kademe DCI oran | U16 | % | 0.01 |
| 0x08CB | 2. Kademe DCI oran | U16 | % | 0.01 |
| 0x08CC | 3. Kademe DCI oran | U16 | % | 0.01 |

### 5.5 Aktif Guc Derating (0x0900-0x093F)

| Adres | Aciklama | Tip | Birim | Olcek |
|-------|----------|-----|-------|-------|
| 0x0900 | RemoteConfig | U16 | — | — |
| 0x0901 | ActiveOutputLimit — Aktif guc cikis limiti | U16 | % | 0.1 |
| 0x0902 | ActiveOutputDownSpeed | U16 | %Pn/min | 1 |
| 0x0903 | GridVoltageDropStart | U16 | V | 0.1 |
| 0x0904 | GridVoltageDropStop | U16 | V | 0.1 |
| 0x0905 | GridVoltageDropMinPower | I16 | % | 1 |
| 0x0906 | OvervoltageDownSpeed | U16 | %Pn/min | 1 |
| 0x0907 | ChgDerateVoltStart — Sarj derating bas | U16 | V | 0.1 |
| 0x0908 | ChgDerateVoltEnd — Sarj derating son | U16 | V | 0.1 |
| 0x0909 | ChgDerateMinPower | I16 | % | 1 |
| 0x0914 | LogicDerateSpeed | U16 | %Pn/min | 1 |
| 0x0915 | LogicReloadSpeed | U16 | %Pn/min | 1 |

### 5.6 Frekans Derating / Load Shedding (0x0940-0x097F)

| Adres | Aciklama | Tip | Birim | Olcek |
|-------|----------|-----|-------|-------|
| 0x0940 | FrequencyDerateConfig | U16 | — | — |
| 0x0941 | OverfrequencyStart | U16 | Hz | 0.01 |
| 0x0942 | OverfrequencyEnd | U16 | Hz | 0.01 |
| 0x0943 | OverfrequencySlope | U16 | %Pn/Hz | 1 |
| 0x0948 | UnderfrequencyStart | U16 | Hz | 0.01 |
| 0x0949 | UnderfrequencyEnd | U16 | Hz | 0.01 |
| 0x094A | UnderfrequencySlope | U16 | %Pn/Hz | 1 |
| 0x0951-0x0962 | ESS Over/Under Frequency (ayni format) | U16 | Hz/% | — |

### 5.7 Reaktif Guc Parametreleri (0x0980-0x09BF) — PF, Q(U), Q(P) Egrileri

| Adres | Aciklama | Tip | Birim | Olcek |
|-------|----------|-----|-------|-------|
| 0x0980 | ReactiveConfig | U16 | — | — |
| 0x0981 | PowerFactor — Sabit guc faktoru | I16 | — | 0.0001 |
| 0x0982 | FixedReactivePercentage | I16 | % | 0.01 |
| 0x0983-0x0990 | Reactive P(U)/Q(U) egri noktalari (4 cift) | I16/U16 | — | — |
| 0x0993 | MaxLeadingReactivePower | U16 | %Pn | 0.01 |
| 0x099F | PhaseType — Faz tipi | U16 | — | — |
| 0x09A0 | ReactiveResponsePeriod | U16 | derece | 1 |
| 0x09A1 | MaxLaggingReactivePower | U16 | %Pn | 0.01 |

### 5.8 Voltaj Ride-Through / LVRT-OVRT (0x09C0-0x09FF) — 4 Noktali Egri

| Adres | Aciklama | Tip | Birim | Olcek |
|-------|----------|-----|-------|-------|
| 0x09C0 | VRTConfig | U16 | — | — |
| 0x09C1 | LVRT giris voltaji | U16 | % | 1 |
| 0x09C2 | LVRT 1. nokta voltaj | U16 | % | 1 |
| 0x09C3 | LVRT 1. nokta sure | U16 | ms | 1 |
| 0x09C4 | LVRT 2. nokta voltaj | U16 | % | 1 |
| 0x09C5 | LVRT 2. nokta sure | U16 | ms | 1 |
| 0x09C6 | LVRT 3. nokta voltaj | U16 | % | 1 |
| 0x09C7 | LVRT 3. nokta sure | U16 | ms | 1 |
| 0x09C8 | LVRT 4. nokta voltaj | U16 | % | 1 |
| 0x09C9 | LVRT 4. nokta sure | U16 | ms | 1 |
| 0x09CA | LVRT reaktif akim katsayisi K | U16 | p.u. | 0.1 |
| 0x09CB | LVRT voltaj kurtarma sonrasi bekleme | U16 | ms | 1 |
| 0x09CC | LVRT guc geri donme hizi | U16 | %Pn/min | 1 |
| 0x09CD | OVRT giris voltaji | U16 | % | 1 |
| 0x09CE-0x09D5 | OVRT 4 nokta (ayni format) | U16 | %/ms | — |
| 0x09D8 | OVRT guc geri donme hizi | U16 | %Pn/min | 1 |

### 5.9 Ada Koruma, GFCI, ISO (0x0A00-0x0A3F)

| Adres | Aciklama | Tip | Birim | Olcek |
|-------|----------|-----|-------|-------|
| 0x0A00 | IslandConfig — Ada koruma konfig | U16 | — | — |
| 0x0A01 | GFCIConfig — Toprak kacak konfig | U16 | — | — |
| 0x0A02 | ISOConfig — Izolasyon konfig | U16 | — | — |
| 0x0A03 | Izolasyon koruma degeri | U16 | kOhm | 1 |
| 0x0A04 | ISO kacak akim limiti | U16 | mA | 1 |
| 0x0A05 | GFCI limiti | U16 | mA/kVA | 1 |
| 0x0A06 | PE_N_Config | U16 | — | — |
| 0x0A07 | Ada algilama hassasiyeti | U16 | — | — |
| 0x0A08 | PE koruma degeri | U16 | kOhm | 1 |
| 0x0A09 | Ozel sertifikasyon | U16 | — | — |

---

## 6. Konfigurasyon (0x1000-0x10BF) — RW (Kismi)

> V1.39 protokolunde detayli. Shadow+Enable pattern ile blok yazma gerektirir.

### Bireysel Yazilabilir Registerlar (Tip 1)

| Adres | Mevcut | Aciklama | Aralik |
|-------|--------|----------|--------|
| 0x102F | 0x0000 | Seri port alani | ? |
| 0x1034 | 0x0001 | Anti-PID aktif | 0=Devre disi, 1=Aktif |
| 0x103E | 0x0000 | Bilinmeyen konfig | ? |
| 0x1060 | 0x0001 | PCC guc algilama modu | 0=CT, 1=Metre |
| 0x1064 | 0x0001 | Kuru kontak enable | 0=Devre disi, 1=Aktif |
| 0x1069 | 0x0000 | PCC bias | S16, birim: 1W |
| 0x106C | 0x0000 | Bilinmeyen konfig | ? |
| 0x1073 | 0x0001 | Batarya kullanim konfigurasyonu | V1.33+ |
| 0x10A5 | 0x0001 | Bilinmeyen konfig | ? |

### Shadow + Enable Blok Yazma (Tip 2)

| Blok | Shadow Aralik | Enable | Adet | Aciklama |
|------|---------------|--------|------|----------|
| Tarih/Saat | 0x1004 - 0x1009 | 0x100A | 7 | Yil/Ay/Gun/Saat/Dk/Sn |
| RS485 Ayar | 0x100B - 0x100E | 0x100F | 5 | Baud, slave ID, parity |
| Batarya Param | 0x1044 - 0x1052 | 0x1053 | 16 | Batarya tipi, kapasite, voltaj limitleri |

---

## 7. Schedule (0x1100-0x11FF) — RW (Kismi)

### Bireysel Yazilabilir

| Adres | Mevcut | Aciklama | Aralik |
|-------|--------|----------|--------|
| 0x1104 | 0x0001 | Remote on/off | 0=Off, 1=On |
| 0x1105 | 0x0000 | Aktif guc sinirlandirma | 0=Devre disi, 1=Aktif |
| 0x1106 | 0x03E8 | Aktif guc limiti | U16, 0.1% (1000=100%) |
| 0x1107 | 0x03E8 | Reaktif guc limiti | U16, 0.1% (1000=100%) |
| 0x1108 | 0x0000 | Guc faktoru ayar | S16, -1000...+1000 |
| 0x1109 | 0x0064 | Aktif guc rampa yukselis | U16, 1% Pn/s |
| 0x110A | 0x0064 | Aktif guc rampa dusus | U16, 1% Pn/s |
| 0x110B | 0x0001 | Q(U) reaktif kontrol | 0=Devre disi, 1=Aktif |
| 0x1110 | 0x0004 | Enerji depolama modu | 0-6 (asagidaki tabloya bak) |

### Shadow + Enable Blok Yazma

| Blok | Shadow Aralik | Enable | Adet | Aciklama |
|------|---------------|--------|------|----------|
| Zamanli Sarj | 0x1111 - 0x111E | 0x111F | 15 | Kural index, enable, zaman, guc |
| Time-Sharing | 0x1120 - 0x112E | 0x112F | 16 | Kural index, enable, mod, zaman, SOC, guc |
| Off-Grid | 0x1144 - 0x1145 | 0x1146 | 3 | Off-grid gerilim, frekans |

### Enerji Depolama Modlari (0x1110)

| Deger | Mod | Time-Sharing Alt Modlari |
|-------|-----|--------------------------|
| 0 | Self-generation (kendi uretim) | — |
| 1 | Time-sharing tariff | mode 0=Sarj, 1=Desarj, 2=Peak-shaving, 3=Besleme, 4=Spontan |
| 2 | Timed charge/discharge | — |
| 3 | Passive | — |
| 4 | Peak-shaving | — |
| 5 | Off-grid | — |
| 6 | Generator | — |

> **NOT:** Peak-shaving (0x1130-0x1135) bu modelde okunabilir ama YAZILAMIYOR (Exception 0x03).
> Peak-shaving islevselligine time-sharing mode=2 ile ulasiliyor.

### Enable Register Donus Degerleri

| Deger | Anlam |
|-------|-------|
| 0x0000 | Basarili |
| 0x0001 | Isleniyor |
| 0xFFFB | Basarisiz — controller yanit yok |
| 0xFFFD | Basarisiz — fonksiyon devre disi |
| 0xFFFE | Basarisiz — depolama hatasi |
| 0xFFFF | Basarisiz — parametre hatali |

---

## 8. Italya Autotest Sonuclari (0x1300-0x133F) — RO

> Kaynak: V1.29 Excel. Italya CEI sertifikasyonu icin otomatik test sonuclari.

| Adres | Aciklama | Tip | Birim | Olcek |
|-------|----------|-----|-------|-------|
| 0x1304-0x1313 | Voltaj test sonuclari (8 cift: deger+sure) | U16 | V / ms | 0.1 / 1 |
| 0x1314-0x1323 | Frekans test sonuclari (8 cift: deger+sure) | U16 | Hz / ms | 0.01 / 1 |

---

## 9. Port Sicaklik + Batarya Cluster (0x1480-0x1661) — RO

> Kaynak: V1.39 changelog (V1.33: "Port Temperature Detection Data Area", V1.34: "256 registers", V1.36: "Battery cluster 1 address")
> Bu bolge V1.29 Excel'de dokumante edilmemis (V1.33+ ile eklenmis).

| Alt Bolge (tahmini) | Aralik | Aciklama |
|---------------------|--------|----------|
| Port Sicaklik | 0x1480-0x14FF | Baglanti noktasi sicakliklari (NTC, 128 register) |
| Port Sicaklik 2 | 0x1500-0x157F | Ek sicaklik verileri |
| Batarya Cluster | 0x1580-0x1661 | Batarya cluster 1 adres alani (hucre voltajlari, BMS) |

> **NOT:** Kesin register tanimlari icin V1.39 PDF'in ilgili bolumlerine bakilmali veya deger degisimi izleme ile teyit edilmeli.

---

## 10. Mevcut Degil (Bu Modelde)

| Aralik | Beklenen | Gercek |
|--------|----------|--------|
| 0x1200 - 0x12FF | EMS Time Period | Exception 0x02 (tamamen yok) |
| 0x1130 - 0x1135 | Peak-shaving parametreleri | Okunabilir ama yazma Exception 0x03 |

---

## 11. Exception 0x01 Bolgesi — FC 0x04 Gerektirebilir

| Aralik | Adet | Aciklama |
|--------|------|----------|
| 0x0A40 - 0x0A7F | 64 | **Pull Arc** (ark koruma) — V1.29 Excel'de "3.10 Pull Arc" olarak tanimli |
| 0x0A80 - 0x0FFF | 1408 | Bilinmeyen — FC 0x03 ile "gecersiz fonksiyon kodu" |

> HYD ES protokolune (2018) gore FC 0x04 (Read Input Registers) farkli adres bolgelerini acabilir.
> Ayrica FC 0x07 (Calibration, 0x3000), FC 0x08 (Maintenance, 0x4000), FC 0x21 (Extended Code) gibi ozel FC'ler de mevcut olabilir.
> Scanner firmware'ina FC 0x04 destegi eklenerek test edilmeli.

---

## 12. Exception 0x03 Adresleri

| Adres | Aciklama |
|-------|----------|
| 0x1340 | Italya Autotest bolgesi siniri — gecersiz veri |
| 0x1380 | Italya Autotest bolgesi siniri |
| 0x13C0 | Gecis bolgesi |
| 0x1400 | Gecis bolgesi |
| 0x1440 | Port Sicaklik baslamadan onceki sinir |

---

## 13. HYD ES Legacy Protokol Bilgisi

> Kaynak: SofarHYD ES ME3000SP Modbus protocol.pdf (2018, V1.04)
> SP2ES, ES ailesi oldugu icin bu eski FC kodlari hala desteklenebilir.

### Eski FC Kodlari (Test Edilmedi)

| FC | Aralik | Aciklama |
|----|--------|----------|
| 0x03 | 0x0000-0x02FF | Gercek zamanli veri (eski adres alani) |
| 0x04 | 0x1000-0x11FF | Invertor/Combiner parametreleri |
| 0x13 | 0x1000-0x11FF | Parametre yazma (FC 0x10 yerine) |
| 0x01 | 0x0142 / 0x0141 / 0x0161 / 0x1062 | Remote on/off, guc limiti, PF, reaktif |
| 0x07 | 0x3000 | Kalibrasyon (sifre korumali) |
| 0x08 | 0x4000 | Bakim bilgileri |
| 0x21 | 0x2000-0x21FF | Extended Code |
| 0x50 | — | EEPROM okuma |
| 0x51 | — | EEPROM yazma |
| 0x30 | — | Fabrika sifirlama |
| 0x31 | — | Bugunun enerjisini sifirla |
| 0x33 | — | Toplam enerjiyi sifirla |

### Eski Adres Alani (0x0200-0x0255) — HYD ES Protokolu

| Adres | Aciklama | Tip | Birim |
|-------|----------|-----|-------|
| 0x0200 | Operation status | U16 | — |
| 0x0201-0x0205 | Fault list 1-5 | U16 | Bit field |
| 0x0206 | Grid R voltage | U16 | V (0.1) |
| 0x0207 | Grid A current | U16 | A (0.01) |
| 0x020C | Grid frequency | U16 | Hz (0.01) |
| 0x020D | Battery charge/discharge power | S16 | kW (0.01) |
| 0x020E | Battery cell voltage | U16 | V (0.1) |
| 0x020F | Battery current | S16 | A (0.01) |
| 0x0210 | Battery SOC | U16 | % | 1 |
| 0x0211 | Battery temperature | S16 | C | 1 |
| 0x0212 | Grid point power | S16 | kW (0.01) |
| 0x0213 | Load power | U16 | kW (0.01) |
| 0x0214 | Hybrid inverter power | S16 | kW (0.01) |
| 0x0215 | PV generation power | U16 | kW (0.01) |
| 0x0218 | Today generation | U16 | kWh (0.01) |
| 0x021C-0x021D | Total generation | U32 | kWh (1) |
| 0x0237 | Battery SOH | U16 | % | 1 |
| 0x0250-0x0255 | PV1/PV2 V/I/P | U16 | V/A/kW |

---

## 14. Referans Dokumanlar

| Dosya | Aciklama |
|-------|----------|
| `Sofar_Inverter_MODBUS_V1.39_EN.pdf` | Ana G3 protokol dokumani (Mart 2025) |
| `SOFAR Modbus Protocol G3_2024-06-06_1.29_en-INT.xlsx` | **V1.29 Excel — en detayli register tablosu** |
| `SOFAR_Modbus_Protocol_English_G3_V1.17.xlsx` | V1.17 Excel (eski versiyon) |
| `SofarHYD ES ME3000SP Modbus protocol.pdf` | **HYD ES legacy protokolu — ozel FC kodlari** |
| `SOFAR 1-70KTL G1-G2 Modbus Protocol EN.pdf` | G1/G2 protokol (string invertor) |
| `HYD 5-20KTL-3PH User Manual.pdf` | HYD kullanici kilavuzu |

## 15. Tarama Kaynak Dosyalari

| Dosya | Aciklama |
|-------|----------|
| `sofar_fullscan_results.txt` | Tam tarama ham logu (8192 register) |
| `sofar_ok_registers.csv` | Okunabilir registerlar CSV (2402 satir) |
| `sofar_rw_registers.csv` | Yazilabilir registerlar CSV (22 satir) |
| `sofar_scanwrite_results.txt` | Konfig bolgesi (0x1000-0x1661) scanwrite logu |
| `sofar_scanwrite_data_results.txt` | Veri bolgesi (0x0040-0x0A3F) scanwrite logu |

## 16. Anlik Veri Snapshot (Tarama Aninda)

### Cihaz Bilgisi
| Register | Deger | Aciklama |
|----------|-------|----------|
| 0x0404 | 2 | Sistem durumu: Sebekeye bagli |
| 0x0445-044C | SP2ES110N95024 | Seri numarasi (ASCII) |
| 0x044D-0452 | V0010V120005 | HW versiyonu (ASCII) |

### Sicaklik
| Register | Deger | Aciklama |
|----------|-------|----------|
| 0x0418 | 48 | Ortam sicakligi (C) |
| 0x041A | 37 | Radyator sicakligi (C) |
| 0x0420 | 41 | Modul sicakligi (C) |

### Batarya
| Register | Deger | Aciklama |
|----------|-------|----------|
| 0x0604 | 0x0852 | Gerilim: 213.0V |
| 0x0608 | 81 | SOC: 81% |
| 0x0609 | 100 | SOH: 100% |
| 0x066A | 1 | Paket sayisi: 1 |
| 0x066B | 46 | Toplam kapasite: 46Ah |

### Sebeke (3-Faz)
| Register | Deger | Aciklama |
|----------|-------|----------|
| 0x0484 | 4998 | Frekans: 49.98 Hz |
| 0x048D | 2292 | R-faz gerilim: 229.2V |
| 0x0498 | 2280 | S-faz gerilim: 228.0V |
| 0x04A3 | 2282 | T-faz gerilim: 228.2V |

### Guvenlik Parametreleri (Ornek Degerler)
| Register | Deger | Aciklama |
|----------|-------|----------|
| 0x0804 | 2530 | Voltaj ust limit: 253.0V |
| 0x0805 | 1955 | Voltaj alt limit: 195.5V |
| 0x0806 | 5010 | Frekans ust limit: 50.10Hz |
| 0x0807 | 4750 | Frekans alt limit: 47.50Hz |
| 0x0841 | 2300 | Nominal voltaj: 230.0V |
| 0x0881 | 5000 | Nominal frekans: 50.00Hz |
