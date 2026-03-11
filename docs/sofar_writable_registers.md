# Sofar Inverter - Yazilabilir (RW) Register Haritasi

> Kaynak: Sofar Modbus-G3 Protocol V1.39 (Mart 2025)
> Olusturma: 2026-03-10
> Olusturan: Guclu Ceyhan

---

## Genel Yazma Kurallari

### Desteklenen Fonksiyon Kodlari
| FC | Isim | Aciklama |
|----|------|----------|
| `0x03` | Read Holding Registers | Okuma (tum registerlar) |
| `0x06` | Write Single Register | Tek register yazma — **Sofar'da CALISMAYABILIR** |
| `0x10` | Write Multiple Registers | Coklu register yazma — **TERCIH EDILEN** |

### Yazma Patterni: Shadow Register + Enable
Sofar, parametre bloklarinda "shadow register" + "enable register" patterni kullanir:

1. **Shadow register'lara** degerleri yaz (0x1004-0x1009 gibi)
2. **Enable register'a** `0x0001` yaz (0x100A gibi)
3. Tum yazma islemi **tek bir FC 0x10 komutu** icinde yapilmali
4. Enable register'i **ayri yazmak CALISMAZ** — shadow + enable birlikte gonderilmeli

### Enable Register Okuma Sonuclari
| Deger | Anlam |
|-------|-------|
| `0x0000` | Islem basarili |
| `0x0001` | Islem devam ediyor |
| `0xFFFB` | Basarisiz — controller yanit vermiyor |
| `0xFFFC` | Basarisiz — controller yanit vermiyor |
| `0xFFFD` | Basarisiz — fonksiyon devre disi |
| `0xFFFE` | Basarisiz — parametre depolama hatasi |
| `0xFFFF` | Basarisiz — girdi parametreleri hatali |

### Modbus Exception Kodlari
| Kod | Anlam |
|-----|-------|
| `0x01` | Gecersiz fonksiyon kodu |
| `0x02` | Gecersiz veri adresi |
| `0x03` | Gecersiz veri icerigi |
| `0x04` | Slave cihaz hatasi |
| `0x07` | Slave cihaz mesgul |

### Veri Tipleri
| Tip | Boyut | Aciklama |
|-----|-------|----------|
| U16 | 1 register | Unsigned 16-bit [0, 65535] |
| S16 | 1 register | Signed 16-bit [-32768, 32767] |
| U32 | 2 register | Unsigned 32-bit, big-endian (high word first) |
| S32 | 2 register | Signed 32-bit, big-endian |
| BCD16 | 1 register | BCD kodlu (orn: 0x2359 = 23:59) |

### ESP32 Serial Monitor Komut Formati
```
read XXXX          — Tek register oku (hex adres)
readn XXXX N       — N adet register oku
write XXXX YYYY    — Tek register yaz FC 0x06 (CALISMAYABILIR)
writem XXXX V1 V2  — Coklu register yaz FC 0x10 (TERCIH EDILEN)
```

---

## 1. SISTEM PARAMETRE KONFIGURASYONU (0x1004 - 0x100F)

### 1.1 Tarih/Saat Ayari
| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x1004` | Sistem yili | U16 | [0, 99] | N/A | Gercek yil = deger + 2000 |
| `0x1005` | Sistem ayi | U16 | [1, 12] | N/A | |
| `0x1006` | Sistem gunu | U16 | [1, 31] | N/A | |
| `0x1007` | Sistem saati | U16 | [0, 23] | N/A | |
| `0x1008` | Sistem dakikasi | U16 | [0, 59] | N/A | |
| `0x1009` | Sistem saniyesi | U16 | [0, 59] | N/A | |
| `0x100A` | *Tarih enable | U16 | 1 yaz | N/A | Enable register |

**Yazma komutu ornegi (2026-03-10 15:30:00):**
```
writem 1004 001A 0003 000A 000F 001E 0000 0001
```
- `001A`=26(yil), `0003`=3(ay), `000A`=10(gun), `000F`=15(saat), `001E`=30(dk), `0000`=0(sn), `0001`=enable
- **DOGRULANDI** ✅ — Basariyla test edildi

### 1.2 RS485 Iletisim Ayarlari
| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x100B` | RS485 slave adresi | U16 | [1, 247] | N/A | Dikkat: degistirirsen iletisim kopar! |
| `0x100C` | RS485 baud rate | U16 | Tablo | N/A | 0=4800, 1=9600(varsayilan), 2=19200, 3=38400, 4=57600, 5=115200, 10=1200, 11=2400 |
| `0x100D` | RS485 stop bit | U16 | [0, 2] | N/A | 0=1bit(varsayilan), 1=1.5bit, 2=2bit |
| `0x100E` | RS485 parity | U16 | [0, 4] | N/A | 0=None(varsayilan), 1=Even, 2=Odd, 3=Mark, 4=Space |
| `0x100F` | *RS485 config enable | U16 | 1 yaz | N/A | Enable register |

**Yazma komutu ornegi (baud rate'i 19200'e degistir):**
```
writem 100B 0001 0002 0000 0000 0001
```
⚠️ **UYARI:** RS485 ayarlarini degistirmek iletisimi bozabilir! ESP32 tarafini da ayni ayarlara getiremezsen invertorle konusamazsin.

### 1.3 PV Giris Kanal Konfigurasyonu
| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x1010` | PV giris modu | U16 | [0, 1] | N/A | 0=Paralel, 1=Bagimsiz(varsayilan) |
| `0x1011` | Kanal 0 tip secimi | U16 | [0, 383] | N/A | 0=kullanilmiyor, 1-127=PV panel, 128-255=batarya, 256-383=fan girisi |
| `0x1012` | Kanal 1 tip secimi | U16 | [0, 383] | N/A | Ayni |
| ... | Kanal 2-15 | U16 | [0, 383] | N/A | 0x1013-0x1020 |
| `0x1021` | *PV config enable | U16 | 1 yaz | N/A | Invertor sebekeye bagli degilken degistir! |

⚠️ **UYARI:** PV kanal konfigurasyonu sadece invertor sebekeye bagli degilken veya kapali iken degistirilmeli.

---

## 2. BATARYA PARAMETRE KONFIGURASYONU (0x1044 - 0x105B)

| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x1044` | Batarya seri numarasi | U16 | [0, 7] | N/A | Fiziksel batarya arayuz numarasi |
| `0x1045` | Batarya adresi | U16 | [0, 99] | N/A | Coklu batarya sistemleri icin |
| `0x1046` | Iletisim protokolu | U16 | Tablo | N/A | 0=Sofar BMS, 1=PYLON, 2=Sofar/GENERAL, 3=AMASS, 4=LG, 5=Alpha.ESS, 6=CATL, 7=Weco, 8=Fronus, 9=EMS, 10=Nilar, 11=BTS5K, 12=Movefor, 13=STRONG C, 14=Reserve, 15=NEOOM |
| `0x1047` | Asiri gerilim korumasi | U16 | [0, 65535] | 0.1V | |
| `0x1048` | Sarj gerilimi | U16 | [0, 65535] | 0.1V | |
| `0x1049` | Dusuk gerilim korumasi | U16 | [0, 65535] | 0.1V | Kursun-asit bataryalarda gorunur |
| `0x104A` | Min desarj gerilimi | U16 | [0, 65535] | 0.1V | |
| `0x104B` | Max sarj akimi | U16 | [0, 65535] | 0.01A | |
| `0x104C` | Max desarj akimi | U16 | [0, 65535] | 0.01A | |
| `0x104D` | DOD (max desarj derinligi) | U16 | [1, 90] | 1% | Sebeke normal iken |
| `0x104E` | EOD (max off-grid desarj) | U16 | [1, 90] | 1% | |
| `0x104F` | Batarya kapasitesi | U16 | [1, 65535] | 1Ah | |
| `0x1050` | Nominal batarya gerilimi | U16 | [0, 65535] | 0.1V | |
| `0x1051` | Hucre tipi | U16 | [0, 6] | N/A | 0=Kursun-asit(ozel), 1=LFP, 2=Ternary, 3=Lityum titanat, 4=AGM, 5=Jel, 6=Su basmis |
| `0x1052` | Off-grid recovery desarj histerezis | U16 | [5, 100] | 1% | |
| `0x1053` | *Batarya param enable | U16 | [1, 2] | N/A | 1=shadow'u uygula, 2=varsayilana don |
| `0x1054` | Batarya adresi 2 | U16 | [0, 99] | N/A | Coklu batarya |
| `0x1055` | Batarya adresi 3 | U16 | [0, 99] | N/A | Coklu batarya |
| `0x1056` | Batarya adresi 4 | U16 | [0, 99] | N/A | Coklu batarya |
| `0x1057` | Kursun-asit sicaklik kompanzasyonu | S16 | [-60, 0] | 0.1mV/Cell | |
| `0x1058` | Kursun-asit recovery desarj artisi | U16 | [0, 65535] | 0.1V | |
| `0x1059` | Fonksiyon kontrol bitleri | U16 | Bit | N/A | Bit0: SOC kontrol devre disi |
| `0x105A` | Kursun-asit float gerilimi | U16 | | 0.1V | |
| `0x105B` | Max sarj SOC | U16 | [60, 100] | 1% | SOC >= bu deger ise sarj durur |

**Okuma komutu:**
```
readn 1044 16
```

---

## 3. FONKSIYON PARAMETRE KONFIGURASYONU 1 (0x1060 - 0x1076)

| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x1060` | PCC guc algilama cihazi | U16 | [1, 4] | N/A | 1=Elektrik sayaci, 2=CT, 3=ARPC, 4=Logger |
| `0x1061` | Rezonans algilama hassasiyeti | U16 | [1, 100] | N/A | |
| `0x1062` | CT orani | S16 | [-9999, 9999] | N/A | |
| `0x1063` | Hard anti-back-flow switch | U16 | [0, 1] | N/A | 0=Kapat, 1=Ac |
| `0x1064` | Kuru kontak kontrolu | U16 | [0, 4] | N/A | 0=Devre disi, 1=Jenerator, 2=Role modu 1, 3=Role modu 2, 4=Akilli yuk kontrolu |
| `0x1065` | Islak kontak kontrolu (rezerve) | U16 | [0, 1] | N/A | 0=Kapat, 1=Ac |
| `0x1066` | PV dal asiri akim korumasi | U16 | [0, 65535] | 0.1A | |
| `0x1067` | Grid algilama ve kontrol | U16 | Bit | N/A | Bit0=Faz kaybi algilama |
| `0x1068` | Faz kaybi algilama alani | U16 | [0, 1000] | 0.1% | |
| `0x1069` | PCC bias gucu | S16 | [-300, 300] | 1% | |
| `0x106A` | Satin alma guc limiti enable | U16 | [0, 1] | N/A | 0=Yasak, 1=Ac |
| `0x106B` | Toplam satin alma guc limiti | U16 | [0, 65535] | 0.1kW | |
| `0x106C` | Akilli yuk modu secimi | U16 | [0, 3] | N/A | 0=Kapat, 1=Ac, 2=Zamanli, 3=Akilli |
| `0x106D` | Akilli yuk baslangic saati | U16 | HH:MM packed | N/A | High byte=saat, Low byte=dakika |
| `0x106E` | Akilli yuk bitis saati | U16 | HH:MM packed | N/A | |
| `0x106F` | Akilli yuk aktivasyon gucu | U16 | [0, 65535] | 0.1kW | |
| `0x1070` | Paralel modul numarasi | U16 | [1, 65535] | N/A | |
| `0x1071` | DRMs kapatma kontrolu | U16 | Bit | N/A | Bit0=DRM0 kontrol |
| `0x1072` | Dosya transfer fonksiyonu | U16 | Bit | N/A | **RO** — sadece oku |
| `0x1073` | Batarya kullanim konfigurasyonu | U16 | [0, 3] | N/A | 0=Batarya kullanmaz, 1=Tek kanal(varsayilan), 2=Bagimsiz kanal 1+2, 3=Paralel kanal 1+2 |
| `0x1074` | *USB dosya export | U16 | Ozel | N/A | 0x0001=Ariza waveform, 0x0002=Lokal waveform, 0x0003=Gecmis olay |
| `0x1075` | Grid tarafi port tipi | U16 | [0, 2] | N/A | 0=Yasak, 1=Guc sebeke, 2=Jenerator |
| `0x1076` | Jenerator tarafi port tipi | U16 | [0, 2] | N/A | 0=Yasak, 1=Jenerator, 2=Akilli yuk |

---

## 4. FONKSIYON PARAMETRE KONFIGURASYONU 2 (0x1084 - 0x10EE)

### 4.1 Arc Algilama
| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x1084` | Arc algilama fonksiyonu | U16 | [0, 1] | N/A | 0x01=Aktif |
| `0x1085` | Peak-to-peak deger ayari | U16 | [0x0064, 0xFFFF] | N/A | |
| `0x1086` | Varyans ayari | U16 | [0x0095, 0xFFFF] | N/A | |
| `0x1087` | Harmonik enerji ayari | U16 | [0x0001, 0xFFFF] | N/A | |
| `0x1088` | Genlik varyans ayari | U16 | [0x0001, 0xFFFF] | N/A | |
| `0x1089` | Zaman alani agirlik ayari | U16 | [0x0001, 0xFFFF] | N/A | |
| `0x108A` | Frekans alani agirlik ayari | U16 | [0x0005, 0xFFFF] | N/A | |
| `0x108B` | Arc algilama hassasiyeti | U16 | [0x0014, 0xFFFF] | N/A | |
| `0x108C` | Arc alarm kapama esigi | U16 | [0x0005, 0xFFFF] | N/A | |
| `0x108D` | Self-check baslatma | U16 | 1 yaz | N/A | Tum kanallarda self-test baslatir |
| `0x108E` | Arc alarm temizleme | U16 | 0x1234 yaz | N/A | Tum kanallarin alarm temizligi |

### 4.2 PLC/BLE Iletisim Ayarlari
| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x10A0` | PLC ayar islem kontrolu | U16 | [0, 1] | N/A | 0=Izin yok, 1=Izin ver |
| `0x10A1` | PLC iletisim ozellikleri | U16 | Bit | N/A | Stop bit, parity, data bits, baud |
| `0x10A5` | BLE Bluetooth ayarlari | U16 | [0, 1] | N/A | 0=Izin yok, 1=Izin ver |
| `0x10A6` | BLE iletisim ozellikleri | U16 | Bit | N/A | Stop bit, parity, data bits, baud |
| `0x10AA` | Iletisim modulu durumu | U16 | Bit | N/A | Bit0=Ag, Bit1=Bulut, Bit2=Bluetooth |

### 4.3 PID (Potansiyel Kaynakli Bozulma) Kontrolu
| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x10AF` | Centralized PID kontrol | U16 | [0, 1] | N/A | 0=Devre disi, 1=Aktif |
| `0x10B0` | PID otomatik calistirma | U16 | [0, 1] | N/A | 0=Yasak, 1=Aktif |
| `0x10B1` | PID calisma baslangic saati | BCD16 | 0x0000-0x2359 | N/A | |
| `0x10B2` | PID calisma suresi | U16 | [0, 65535] | 1dk | |
| `0x10B3` | PID baslangic PV esigi | U16 | [0, 65535] | 0.1V | |
| `0x10B4` | PID ariza sonrasi bekleme | U16 | [0, 65535] | 1dk | |

### 4.4 Iletisim Kesinti Kontrolu
| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x10C0` | Iletisim kesinti kontrolu | U16 | [0, 5] | N/A | 0=Devre disi, 1=Kapat, 2=Onceki duruma gore, 3=Sabit aktif %, 4=Sabit reaktif %, 5=Sabit guc faktoru |
| `0x10C1` | Iletisim timeout suresi | U16 | [0, 999] | 1s | |
| `0x10C2` | Iletisim kesinti preset degeri | S16 | Degisken | N/A | Mode'a gore: aktif %= 0-1100, reaktif %= 0-1100, guc faktoru= +-1000+-800 |

### 4.5 Guc Cikis Kontrolu
| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x10CC` | Guc enable kontrolu | U16 | Bit | N/A | Bit0=Aktif guc enable, Bit1=Reaktif guc enable |
| `0x10CD-0x10CE` | Aktif guc cikis kontrolu | S32 | | 0.01kW | Anlik aktif guc cikisini kontrol et |
| `0x10CF-0x10D0` | Reaktif guc cikis kontrolu | S32 | | 0.01kVar | Anlik reaktif guc cikisini kontrol et |

### 4.6 Fan Kontrolu
| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x10E0` | Fan self-test kontrolu | U16 | [0, 1] | N/A | 0=Yasak, 1=Aktif |
| `0x10E1` | Fan gurultu modu | U16 | [0, 1] | N/A | 0=Standart(varsayilan), 1=Dusuk gurultu |
| `0x10E2` | Fan alarm algilama modu | U16 | [0, 1] | N/A | 0=Ariza durum algilama, 1=Ariza bakim algilama |
| `0x10E3` | Fan algilama hassasiyeti | U16 | | N/A | |
| `0x10E4` | Fan hiz kontrolu | U16 | [0, 100] | 1% | Max kontrol hiz yuzdesi |

### 4.7 Dengesiz Guc Kontrolu
| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x10EB` | Sabit dengesiz guc kontrolu | U16 | [0, 1] | N/A | 0=Yasak, 1=Aktif |
| `0x10EC` | R-faz dengesiz guc % | S16 | [-500, 500] | 0.1% | -50% ~ +50% temsil eder |
| `0x10ED` | S-faz dengesiz guc % | S16 | [-500, 500] | 0.1% | |
| `0x10EE` | T-faz dengesiz guc % | S16 | [-500, 500] | 0.1% | |

### 4.8 Diger Fonksiyon Parametreleri
| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x10F0` | Role konfigurasyonu | U16 | [0, 1] | N/A | 0=Baglantiyi kesme, 1=Baglanti kesildi |
| `0x10F1` | Standby izleme kontrolu | U16 | [0, 1] | N/A | 0=Yasak, 1=Aktif (gece sebekeden guc ceker) |
| `0x1093` | Max harmonik enerji sinirlamasi | U16 | | N/A | |
| `0x1094` | Ortalama harmonik enerji sinirlamasi | U16 | | N/A | |
| `0x1095` | Harmonik enerji yuzdesi | U16 | [0, 100] | 1% | |

---

## 5. REMOTE KONTROL PARAMETRELERI (0x1104 - 0x110D)

⚠️ **DIKKAT: Bu registerlar invertorun calismasini dogrudan etkiler!**

| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x1104` | Remote acma/kapama | U16 | [0, 1] | N/A | 0x0000=Uzaktan kapat, 0x0001=Uzaktan ac |
| `0x1105` | Guc kontrolu | U16 | Bit | N/A | Bit0=Aktif guc enable, Bit1=Reaktif guc enable, Bit2=Reaktif mod(0=reaktif guc,1=guc faktoru), Bit3=SVG enable, Bit4=SVG reaktif mod, Bit5=SVG guc faktoru kompanzasyonu, Bit8=Gece operasyonu |
| `0x1106` | Export max aktif guc % | U16 | [0, 1100] | 0.1% | |
| `0x1107` | Import max aktif guc % | U16 | [0, 1000] | 0.1% | |
| `0x1108` | Reaktif guc yuzdesi | S16 | [-1000, 1000] | 0.1% | |
| `0x1109` | Guc faktoru | S16 | [-100, 100] | 0.01p.u. | |
| `0x110A` | Aktif guc limit degisim hizi | U16 | [0, 65535] | 1%Pn/dk | 0=max hiz |
| `0x110B` | Reaktif guc ayar yanit suresi | U16 | [0, 65535] | 0.1S | |
| `0x110C` | Sabit reaktif guc buyuklugu | S16 | [-32768, 32767] | 0.1kVar | |
| `0x110D` | SVG guc faktoru kompanzasyonu | S16 | [-32768, 32767] | 0.1kVar | |

---

## 6. ENERJI DEPOLAMA MODU AYARLARI (0x1110)

| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x1110` | Enerji depolama calisma modu | U16 | [0, 6] | N/A | Asagidaki tablo |

**Calisma Modlari:**
| Deger | Mod | Aciklama |
|-------|-----|----------|
| 0 | Self-generation | Kendi kendine uretim |
| 1 | Time-sharing tariff | Zaman dilimli tarife |
| 2 | Timed charge/discharge | Zamanli sarj/desarj |
| 3 | Passive | Pasif mod |
| 4 | Peak-shaving | Tepe tiras |
| 5 | Off-grid | Sebekeden bagimsiz |
| 6 | Generator | Jenerator modu |

---

## 7. ZAMANLI SARJ/DESARJ PARAMETRELERI (0x1111 - 0x111F)

| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x1111` | Kural seri numarasi | U16 | [0, 3] | N/A | Dusuk numara = yuksek oncelik |
| `0x1112` | Enable kontrol | U16 | Bit | N/A | Bit0=Sarj enable, Bit1=Desarj enable |
| `0x1113` | Sarj baslangic saati | U16 | HH:MM packed | N/A | High byte=saat[0-23], Low byte=dakika[0-59] |
| `0x1114` | Sarj bitis saati | U16 | HH:MM packed | N/A | |
| `0x1115` | Desarj baslangic saati | U16 | HH:MM packed | N/A | |
| `0x1116` | Desarj bitis saati | U16 | HH:MM packed | N/A | |
| `0x1117-0x1118` | Sarj gucu | U32 | [1, 4294967296] | 1W | |
| `0x1119-0x111A` | Desarj gucu | U32 | [1, 4294967296] | 1W | |
| `0x111F` | *Zamanli sarj enable | U16 | 1 yaz | N/A | Shadow'u sisteme uygular |

**HH:MM Packed Format:**
- Saat 15:30 icin: High byte = 0x0F (15), Low byte = 0x1E (30) → Register degeri = `0x0F1E`

---

## 8. TIME-SHARING KONTROL PARAMETRELERI (0x1120 - 0x112F)

| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x1120` | Kural seri numarasi | U16 | [0, 255] | N/A | Dusuk = yuksek oncelik |
| `0x1121` | Enable kontrol | U16 | [0, 1] | N/A | 0=Yasak, 1=Aktif |
| `0x1122` | Baslangic saati | U16 | HH:MM packed | N/A | |
| `0x1123` | Bitis saati | U16 | HH:MM packed | N/A | |
| `0x1124` | Hedef SOC | U16 | [10, 100] | 1% | Moda gore farkli anlam tasir |
| `0x1125-0x1126` | Hedef guc | U32 | | 1W | Moda gore farkli anlam tasir |
| `0x1127` | Gecerlilik baslangic tarihi | U16 | MM:DD packed | N/A | High byte=ay, Low byte=gun |
| `0x1128` | Gecerlilik bitis tarihi | U16 | MM:DD packed | N/A | |
| `0x1129` | Gecerli haftanin gunleri | U16 | Bit | N/A | Bit0=Pzt, Bit1=Sal, ..., Bit6=Paz |
| `0x112A` | Kontrol modu konfigurasyonu | U16 | [0, 6] | N/A | 0=Zorla sarj, 1=Zorla desarj, 2=Peak-shaving, 3=Besleme onceligi, 4=Spontan kendi kullanim, 5=Sarj bakim, 6=Desarj bakim |
| `0x112B` | Ek ozellik konfigurasyonu | U16 | Bit | N/A | Moda gore degisir (sebekeden sarj enable vb.) |
| `0x112C` | Hedef SOC 2 | U16 | | 1% | |
| `0x112F` | *Write enable | U16 | 1 yaz | N/A | Shadow'u uygular |

---

## 9. PEAK-SHAVING PARAMETRELERI (0x1130 - 0x1135)

| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x1130-0x1131` | Satin alma guc ust siniri | U32 | [100, 65535] | 1W | Sebekeden alinabilecek max guc |
| `0x1132-0x1133` | Satis guc ust siniri | U32 | [100, 65535] | 1W | Sebekeye satilabilecek max guc |
| `0x1134` | Sebekeden sarj izni | U16 | [0, 1] | N/A | 0=Yasak, 1=Izin ver |
| `0x1135` | Batarya yedek SOC | U16 | [20, 100] | 1% | SOC bu degeri asinca spontan kullanima gecer |

---

## 10. OFF-GRID MOD PARAMETRELERI (0x1144 - 0x1146)

| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x1144` | Sarj kaynagi secimi | U16 | [0, 2] | N/A | 0=Grid, 1=Jenerator, 2=Rezerve |
| `0x1145` | Grid cekme gucu | U16 | [0, 65535] | 0.01kW | |
| `0x1146` | Max jenerator guc cekisi | U16 | [0, 65535] | 0.01kW | |

---

## 11. ENERJI DEPOLAMA MOD PARAMETRELERI (0x1184 - 0x11A9)

### 11.1 Passive Mod
| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x1184` | Passive mod timeout | U16 | [0, 65535] | 1s | Iletisim yoksa timeout aksiyonu |
| `0x1185` | Passive mod timeout aksiyonu | U16 | [0, 1] | N/A | 0=Zorla standby, 1=Enerji depolama moduna don |
| `0x1187-0x1188` | Manuel mod istenen grid gucu | S32 | | 1W | +grid→sistem, -sistem→grid |
| `0x1189-0x118A` | Manuel mod batarya min sarj/desarj | S32 | | 1W | +sarj, -desarj |
| `0x118B-0x118C` | Manuel mod batarya max sarj/desarj | S32 | | 1W | +sarj, -desarj |
| `0x118D-0x118E` | Manuel mod izin verilen satis | S32 | | 1W | +grid→sistem |
| `0x118F-0x1190` | Manuel mod izin verilen satin alma | S32 | | 1W | -sistem→grid |

### 11.2 Scheduler Modu
| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x1191-0x1192` | Scheduler beklenen grid gucu | S32 | | 1W | |
| `0x1193-0x1194` | Scheduler batarya min sarj/desarj | S32 | | 1W | |
| `0x1195-0x1196` | Scheduler batarya max sarj/desarj | S32 | | 1W | |
| `0x1197-0x1198` | Scheduler izin verilen satis | S32 | | 1W | |
| `0x1199-0x119A` | Scheduler izin verilen satin alma | S32 | | 1W | |
| `0x119B-0x119C` | Scheduler beklenen grid gucu 2 | S32 | | 1W | |
| `0x119D-0x119E` | Scheduler batarya min 2 | S32 | | 1W | |
| `0x119F-0x11A0` | Scheduler batarya max 2 | S32 | | 1W | |
| `0x11A1-0x11A2` | Scheduler satis 2 | S32 | | 1W | |
| `0x11A3-0x11A4` | Scheduler satin alma 2 | S32 | | 1W | |
| `0x11A5-0x11A6` | Scheduler baslangic zamani | U32 | | 1s | |
| `0x11A7-0x11A8` | Scheduler parametre suresi | U32 | | 1s | |
| `0x11A9` | Scheduler yonetim modu | U16 | [0, 2] | N/A | 0=Spontan, 1=Manuel, 2=Scheduler |

---

## 12. EMS ZAMAN PERIYODU KONFIGURASYONU (0x1204 - 0x1211)

| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x1204` | Zaman periyodu enable bit | U16 | Bit | N/A | Bit0-Bit5 = Periyot 1-6 enable |
| `0x1205` | Periyot 1 baslangic saati | U16 | HH:MM packed | N/A | |
| `0x1206` | Periyot 1 bitis saati | U16 | HH:MM packed | N/A | |
| `0x1207` | Periyot 1 calisma modu | U16 | [0, 6] | N/A | 0=PV, 1=Sabit guc, 2=Spontan, 3=Peak-shaving, 4=Talep kontrol, 5=Zorla sarj, 6=Zorla desarj |
| `0x1208` | Grid baglanti noktasi guc ust siniri | S16 | | 0.01kW | |
| `0x1209` | Grid baglanti noktasi guc alt siniri | S16 | | 0.01kW | |
| `0x120A` | Batarya sarj izni ust siniri | U16 | [0, 100] | 1% | |
| `0x120B` | Batarya desarj izni alt siniri | U16 | [0, 100] | 1% | |
| `0x120C` | Grid'den sarj izni | U16 | [0, 1] | N/A | 0=Izin yok, 1=Izin var |
| `0x120D` | Invertor gucu | S16 | | 0.01kW | |
| `0x120E` | Zorla sarj/desarj gucu | S16 | | 0.01kW | |
| `0x120F` | Spontan fazla guc grid baglantisi | U16 | [0, 1] | N/A | 0=Izin yok, 1=Izin var |

---

## 13. GUVENLIK PARAMETRELERI (0x0800 - 0x0A09) ⚠️ DIKKAT

Bu parametreler invertorun guvenlik korumalarini etkiler. **Yanlis ayar invertore zarar verebilir!**

### 13.1 Power-On Parametreleri (0x0800 - 0x080B)
| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x0800` | Grid baglanti oncesi bekleme | U16 | [1, 65535] | 1s | |
| `0x0801` | Grid baglanti rampa hizi | U16 | [1, 3000] | 1%Pn/dk | 100 = 1dk'da tam guc |
| `0x0802` | Grid ariza sonrasi yeniden baglanti bekleme | U16 | [1, 65535] | 1s | |
| `0x0803` | Grid ariza sonrasi yukselme hizi | U16 | [1, 3000] | 1%Pn/dk | |
| `0x0804` | Grid baslangic gerilimi ust siniri | U16 | [0, 65535] | 0.1V | |
| `0x0805` | Grid baslangic gerilimi alt siniri | U16 | [0, 65535] | 0.1V | |
| `0x0806` | Grid baslangic frekans ust siniri | U16 | [4000, 7000] | 0.01Hz | |
| `0x0807` | Grid baslangic frekans alt siniri | U16 | [4000, 7000] | 0.01Hz | |
| `0x0808` | Yeniden baglanti gerilim ust siniri | U16 | [0, 65535] | 0.1V | |
| `0x0809` | Yeniden baglanti gerilim alt siniri | U16 | [0, 65535] | 0.1V | |
| `0x080A` | Yeniden baglanti frekans ust siniri | U16 | [4000, 7000] | 0.01Hz | |
| `0x080B` | Yeniden baglanti frekans alt siniri | U16 | [4000, 7000] | 0.01Hz | |

### 13.2 Gerilim Koruma (0x0840 - 0x0851)
| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x0840` | Koruma enable kontrol | U16 | Bit | N/A | Bit bazli, 3 seviye asiri/dusuk gerilim |
| `0x0841` | Nominal grid gerilimi | U16 | [0, 65535] | 0.1V | |
| `0x0842` | Seviye 1 asiri gerilim degeri | U16 | [0, 65535] | 0.1V | |
| `0x0843` | Seviye 1 asiri gerilim suresi | U16 | [1, 65535] | 10ms | |
| `0x0844-0x0847` | Seviye 2-3 asiri gerilim deger/sure | U16 | | | Ayni format |
| `0x0848-0x084D` | Seviye 1-3 dusuk gerilim deger/sure | U16 | | | Ayni format |
| `0x084E` | 10dk asiri gerilim koruma | U16 | [0, 65535] | 0.1V | |

### 13.3 Frekans Koruma (0x0880 - 0x0895)
- Gerilim korumasina benzer yapida, 3 seviye asiri/dusuk frekans korumasi
- Adres araligi: 0x0880-0x0895

### 13.4 DCI Koruma (0x08C0 - 0x08CC)
- DC bileseni korumasi, 3 seviye
- Adres araligi: 0x08C0-0x08CC

### 13.5 Gerilim/Frekans Load Shedding (0x0900 - 0x0963)
- Asiri/dusuk gerilim ve frekans durumunda yuk atma
- Adres araligi: 0x0900-0x0963

### 13.6 Reaktif Guc Parametreleri (0x0980 - 0x09A1)
- Reaktif guc kontrol modlari (5 mod)
- Adres araligi: 0x0980-0x09A1

### 13.7 Gerilim Ride-Through (0x09C0 - 0x09E1)
- LVRT/OVRT gecis parametreleri
- Adres araligi: 0x09C0-0x09E1

### 13.8 Island/GFCI/ISO (0x0A00 - 0x0A09)
| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x0A00` | Ada modu enable | U16 | Bit | N/A | Bit0=Ada enable |
| `0x0A01` | GFCI enable | U16 | Bit | N/A | Bit0=GFCI enable |
| `0x0A02` | ISO enable | U16 | Bit | N/A | Bit0=Izolasyon, Bit1=Topraklama |
| `0x0A03` | Izolasyon empedans koruma | U16 | [30, 65535] | 1kOhm | |
| `0x0A04` | Bilesen kacak akim siniri | U16 | [1, 50] | 1mA | |
| `0x0A09` | Guc hesaplama kontrolu | U16 | Bit | N/A | Bit0=Batarya gucu bazli hesaplama |

---

## 14. SANTRAL ILETISIM PARAMETRELERI (0x11C4 - 0x11C8)

| Adres | Isim | Tip | Aralik | Birim | Not |
|-------|------|-----|--------|-------|-----|
| `0x11C4` | Iletisim kesinti koruma enable | U16 | [0, 1] | N/A | 0=Devre disi(varsayilan), 1=Aktif |
| `0x11C5` | Iletisim kesinti kurtarma yontemi | U16 | [0, 1] | N/A | 0=Otomatik(varsayilan), 1=Manuel |
| `0x11C6` | Iletisim kesinti esik suresi | U16 | [10, 120] | 1dk | |
| `0x11C7` | Iletisim kesinti veri izleme kaynagi | U16 | [0, 3] | N/A | 0=485(varsayilan), 1=WiFi, 3=PLC |
| `0x11C8` | Iletisim kesinti alarm temizleme | U16 | 1 yaz | N/A | **WO** — sadece yaz |

---

## PRATIK YAZMA ORNEKLERI

### Bireysel Register Yazma (Enable'siz Bloklar)
```bash
# Enerji depolama modunu degistir (0=Self-gen, 1=Time-sharing, 2=Timed, 3=Passive, 5=Off-grid)
writem 1110 0002

# PCC bias gucunu %5 olarak ayarla
writem 1069 0005

# Remote kontrol — invertoru kapat
writem 1104 0000

# Remote kontrol — export max aktif gucu %80'e dusur
writem 1106 0320

# Kuru kontak kontrolu — jenerator moduna al
writem 1064 0001
```

### Enable Blok Yazma (Tam Blok Gerekli)
```bash
# Tarih/Saat ayarla: 2026-03-10 15:30:00
writem 1004 001A 0003 000A 000F 001E 0000 0001

# Batarya DOD'u %90'a cikarmak icin (tum blogu yaz):
writem 1044 0000 0001 0001 1388 0000 0000 01A4 0DAC 0DAC 005A 0050 002E 1194 0000 0005 0001
#                                                              ^^^^ 0x005A = 90

# Off-grid mod — sarj kaynagini jenerator olarak ayarla:
writem 1144 0001 0000 0001
```

### Bu Modelde CALISMAYAN Yazmalar (Denemeyin)
```bash
# PV kanal config — Exception 0x02 (her turlu blok boyutunda basarisiz)
# writem 1010 ... → CALISMAZ

# Peak-shaving — Exception 0x03 (bu modelde desteklenmiyor)
# writem 1130 ... → CALISMAZ

# Safety params — Exception 0x01 (FC 0x10 yasak)
# writem 0800 ... → CALISMAZ

# Fan kontrolu — Exception 0x02 (register mevcut degil, V1.37+ gerekli)
# writem 10E1 ... → CALISMAZ
```

---

## TEST SONUCLARI VE YAZMA DAVRANISI ANALIZI

> Bu bolum gercek invertor uzerinde yapilan testlerin sonuclarini icerir.
> Invertor modeli: Sofar (Modbus G3), Slave ID: 1, RS485 9600 8N1
> Test tarihi: 2026-03-10

### FC 0x06 vs FC 0x10 Destegi

| FC | Sonuc | Aciklama |
|----|-------|----------|
| `0x03` (Read) | ✅ Calisiyor | Tum okunan registerlar basarili |
| `0x06` (Write Single) | ❌ CALISMAZ | Timeout — invertor yanit vermiyor |
| `0x10` (Write Multiple) | ✅ Calisiyor | Tek veya coklu register yazma basarili |

**KURAL: Her zaman FC 0x10 (writem) kullan. FC 0x06 (write) bu invertorde desteklenmiyor.**

### Register Blok Yazma Davranisi — KAPSAMLI TEST SONUCLARI

> Tum testler 2026-03-10 tarihinde gercek invertor uzerinde yapildi.
> Her blok icin: okuma, tek register yazma ve tam blok yazma test edildi.

#### Kategori 1: Enable Register'li Bloklar (Shadow + Enable Patterni) ✅
Bu bloklarda **tek register yazma CALISMAZ**. Tum shadow register'lar + enable register **tek FC 0x10 komutu** icinde gonderilmeli.

| Blok | Adres Araligi | Enable | Reg Sayisi | Tek Yazma | Blok Yazma | Test Sonucu |
|------|---------------|--------|------------|-----------|------------|-------------|
| Tarih/Saat | 0x1004-0x1009 | 0x100A | 7 | ❌ 0x03 | ✅ | **DOGRULANDI** |
| RS485 Ayarlari | 0x100B-0x100E | 0x100F | 5 | — | ✅ | **DOGRULANDI** |
| Batarya Param | 0x1044-0x1052 | 0x1053 | 16 | — | ✅ | **DOGRULANDI** |
| Zamanli Sarj | 0x1111-0x111E | 0x111F | 15 | — | ✅ | **DOGRULANDI** |
| Time-Sharing | 0x1120-0x112E | 0x112F | 16 | ❌ 0x02 | ✅ (test onceki) | **DOGRULANDI** |
| Off-Grid | 0x1144-0x1145 | 0x1146 | 3 | ❌ 0x03 | ✅ | **DOGRULANDI** |

**Enable blok yazma kurallari:**
- FC 0x10 ile tum shadow register'lar + enable = tek komut
- Enable register'a ayri yazma CALISMAZ
- Sadece shadow register'lara yazma (enable olmadan) CALISMAZ (Exception 0x03)
- Tek shadow register yazma (enable olmadan) CALISMAZ (Exception 0x02 veya 0x03)
- Enable deger: her zaman `0x0001` (batarya icin `0x0002` = varsayilana don)
- Enable okuma sonucu: `0x0000` = basarili, `0x0001` = islemde, `0xFFFx` = hata

**Enable blok yazma ornekleri:**
```bash
# Tarih/Saat (7 register): 2026-03-10 15:30:00
writem 1004 001A 0003 000A 000F 001E 0000 0001

# RS485 Config (5 register): slave=1, baud=9600, stop=1, parity=none
writem 100B 0001 0001 0000 0000 0001

# Batarya Param (16 register): mevcut degerlerle
writem 1044 0000 0001 0001 1388 0000 0000 01A4 0DAC 0DAC 0050 0050 002E 1194 0000 0005 0001

# Zamanli Sarj/Desarj (15 register)
writem 1111 0000 0000 1600 051E 0600 1500 0000 03E8 0000 03E8 0000 0000 0000 0000 0001

# Off-Grid (3 register)
writem 1144 0000 0000 0001
```

#### Kategori 2: Bireysel Yazilabilir Bloklar (Enable'siz) ✅
Bu bloklarda **tek register yazma CALISIYOR**. Enable register yok, FC 0x10 ile dogrudan yazilir.

| Blok | Adres | Register | Tek Yazma | Test Sonucu |
|------|-------|----------|-----------|-------------|
| Fonksiyon Param | 0x1060 | PCC guc algilama | ✅ | **DOGRULANDI** |
| Fonksiyon Param | 0x1064 | Kuru kontak kontrolu | ✅ | **DOGRULANDI** |
| Fonksiyon Param | 0x1069 | PCC bias gucu | ✅ | **DOGRULANDI** |
| Fonksiyon Param | 0x106C | Akilli yuk modu secimi | ✅ | **DOGRULANDI** |
| Fonksiyon Param | 0x1073 | Batarya kullanim config | ✅ | **DOGRULANDI** |
| BLE Ayarlari | 0x10A5 | BLE Bluetooth enable | ✅ | **DOGRULANDI** |
| Enerji Depolama | 0x1110 | Calisma modu | ✅ | **DOGRULANDI** |
| Remote Kontrol | 0x1104 | Remote on/off | ✅ | **DOGRULANDI** |
| Remote Kontrol | 0x1105 | Guc kontrolu | ✅ | **DOGRULANDI** |
| Remote Kontrol | 0x1106 | Export max aktif guc | ✅ | **DOGRULANDI** |
| Remote Kontrol | 0x1107 | Import max aktif guc | ✅ | **DOGRULANDI** |
| Remote Kontrol | 0x1108 | Reaktif guc % | ✅ | **DOGRULANDI** |
| Remote Kontrol | 0x1109 | Guc faktoru | ✅ | **DOGRULANDI** |
| Remote Kontrol | 0x110A | Aktif guc limit hizi | ✅ | **DOGRULANDI** |
| Remote Kontrol | 0x110B | Reaktif yanit suresi | ✅ | **DOGRULANDI** |

#### Kategori 3: Bu Modelde YAZILAMAYAN Bloklar ❌

| Blok | Adres | Tek Yazma | Blok Yazma | Exception | Analiz |
|------|-------|-----------|------------|-----------|--------|
| PV Kanal Config | 0x1010-0x1021 | ❌ 0x02 | ❌ 0x02 (18 reg) | 0x02 | 5, 13, 18 register denendi — hicbiri calismaz |
| Peak-Shaving | 0x1130-0x1135 | ❌ 0x03 | ❌ 0x03 (6 reg+enable) | 0x03 | Tek ve tam blok denendi — bu modelde desteklenmiyor |
| Energy Storage Params | 0x1100-0x1103 | ❌ 0x02 | ❌ 0x02 (4 reg+enable) | 0x02 | Okunabiliyor ama yazilamiyor |
| Safety Params | 0x0800+ | — | ❌ 0x01 (4 reg) | **0x01** | **Illegal Function** — FC 0x10 bu adreste desteklenmiyor |
| Arc Algilama | 0x1084-0x108E | ❌ 0x02 | — | 0x02 | Okunabiliyor ama yazilamiyor |
| PLC Ayarlari | 0x10A0 | ❌ 0x02 | — | 0x02 | Okunabiliyor ama yazilamiyor |
| PID Kontrolu | 0x10AF-0x10B4 | ❌ 0x02 | — | 0x02 | Okunabiliyor ama yazilamiyor |
| Passive Mod | 0x1184-0x1185 | ❌ 0x03/0x02 | — | 0x03/0x02 | Okunabiliyor ama yazilamiyor (enable blok olabilir) |
| Scheduler Mod | 0x11A9 | ❌ 0x02 | — | 0x02 | Okunabiliyor ama yazilamiyor |
| Santral Iletisim | 0x11C4-0x11C8 | ❌ 0x02 | — | 0x02 | Okunabiliyor ama yazilamiyor |
| Harmonik Enerji | 0x1093-0x1095 | Test edilmedi | — | — | Okunabiliyor, yazma test edilmedi |

#### Kategori 4: Bu Modelde MEVCUT OLMAYAN Register'lar ❌

| Adres | Isim | Eklendigi Versiyon | Okuma | Yazma |
|-------|------|--------------------|-------|-------|
| `0x10E0-0x10E4` | Fan kontrolu | V1.37 (2024/12) | ❌ 0x02 | ❌ |
| `0x10C0-0x10C2` | Iletisim kesinti kontrolu | V1.38 (2025/01) | ❌ 0x02 | ❌ |
| `0x10CC-0x10D0` | Guc cikis kontrolu | — | ❌ 0x02 | ❌ |
| `0x10EB-0x10EE` | Dengesiz guc kontrolu | — | ❌ 0x02 | ❌ |
| `0x10F0-0x10F1` | Role/standby izleme | — | ❌ 0x02 | ❌ |
| `0x1204-0x120F` | EMS zaman periyodlari | — | ❌ 0x02 | ❌ |

#### Kategori 5: Karisik Davranis — Register Bazinda Farklilik

**Fonksiyon Parametreleri 1 (0x1060-0x106A):**
Tam blok yazma CALISMAZ (Exception 0x03), ama bazi registerlar bireysel yazilabiir.

| Adres | Parametre | Bireysel Yazma | Exception |
|-------|-----------|----------------|-----------|
| 0x1060 | PCC guc algilama cihazi | ✅ | — |
| 0x1061 | Rezonans algilama | ❌ | 0x02 |
| 0x1062 | CT orani | ❌ | **0x04** (Slave Device Failure) |
| 0x1063 | Hard anti-back-flow | ❌ | 0x02 |
| 0x1064 | Kuru kontak kontrolu | ✅ | — |
| 0x1065 | Islak kontak (rezerve) | ❌ | 0x02 |
| 0x1066 | PV asiri akim koruma | ❌ | 0x02 |
| 0x1067 | Grid algilama ve kontrol | ❌ | 0x02 |
| 0x1068 | Faz kaybi algilama alani | ❌ | 0x02 |
| 0x1069 | PCC bias gucu | ✅ | — |
| 0x106A | Satin alma guc limiti enable | ❌ | **0x03** (enable register davranisi) |
| 0x106B | Toplam satin alma guc limiti | ❌ | 0x02 |
| 0x106C | Akilli yuk modu secimi | ✅ | — |
| 0x106D | Akilli yuk baslangic saati | ❌ | 0x02 |
| 0x106E | Akilli yuk bitis saati | ❌ | 0x02 |
| 0x106F | Akilli yuk aktivasyon gucu | ❌ | 0x02 |
| 0x1070 | Paralel modul numarasi | ❌ | 0x02 |
| 0x1071 | DRMs kapatma kontrolu | ❌ | 0x02 |
| 0x1073 | Batarya kullanim config | ✅ | — |
| 0x1074 | USB dosya export | ❌ | 0x02 |
| 0x1075 | Grid tarafi port tipi | ❌ | 0x02 |
| 0x1076 | Jenerator tarafi port tipi | ❌ | 0x02 |

> **Not:** 0x1062 (CT orani) Exception 0x04 donduruyor — adres gecerli, register mevcut, ama cihaz
> islemi yurutemiyor. Muhtemelen invertor calisma durumuna (on-grid/off-grid/standby) bagli.

**Remote Kontrol (0x1104-0x110D):**

| Adres | Parametre | Bireysel Yazma | Exception |
|-------|-----------|----------------|-----------|
| 0x1104 | Remote on/off | ✅ | — |
| 0x1105 | Guc kontrolu | ✅ | — |
| 0x1106 | Export max aktif guc | ✅ | — |
| 0x1107 | Import max aktif guc | ✅ | — |
| 0x1108 | Reaktif guc % | ✅ | — |
| 0x1109 | Guc faktoru | ✅ | — |
| 0x110A | Aktif guc limit hizi | ✅ | — |
| 0x110B | Reaktif yanit suresi | ✅ | — |
| 0x110C | Sabit reaktif guc | ❌ | 0x02 |
| 0x110D | SVG kompanzasyon | ❌ | 0x02 |

> **Not:** 0x110C-0x110D okunabiliyor (deger=0) ama yazilamiyor. Bu modelde
> SVG/sabit reaktif guc ozelligi desteklenmiyor olabilir.

### Modbus Exception Ozeti

Bu invertorde karsilasilan tum exception kodlari:

| Exception | Kod | Anlam | Karsilasilan Yer |
|-----------|-----|-------|-----------------|
| Illegal Function | 0x01 | FC desteklenmiyor | Safety params (0x0800+) — FC 0x10 bu adreste yasak |
| Invalid Address | 0x02 | Adres gecersiz/mevcut degil | Fan (0x10E0+), PV config (0x1010+), tek register enable bloklarda |
| Illegal Data Value | 0x03 | Veri degeri gecersiz | Enable bloklarda tek register yazma, peak-shaving |
| Slave Device Failure | 0x04 | Cihaz islemi yurutemedi | CT orani (0x1062) — muhtemelen calisma durumuna bagli |

Bu registerlar V1.37+ firmware gerektiriyor. Invertorün firmware versiyonunu kontrol etmek icin:
```
readn 0404 2
```

### Okunan Gercek Degerler (2026-03-10)

#### Fonksiyon Parametreleri (0x1060-0x1069)
| Adres | Parametre | Deger | Anlam |
|-------|-----------|-------|-------|
| `0x1060` | PCC guc algilama cihazi | 1 | Elektrik sayaci |
| `0x1061` | Rezonans algilama | 0 | Kapali |
| `0x1062` | CT orani | 40 | 40x |
| `0x1063` | Hard anti-back-flow | 0 | Kapali |
| `0x1064` | Kuru kontak | 0 | Devre disi |
| `0x1065` | Islak kontak | 0 | Kapali |
| `0x1066` | PV asiri akim koruma | 0 | Kapali |
| `0x1067` | Grid algilama | 0 | Kapali |
| `0x1068` | Faz kaybi alani | 0 | — |
| `0x1069` | PCC bias gucu | 0 | %0 |

#### Batarya Parametreleri (0x1044-0x1053)
| Adres | Parametre | Raw | Anlam |
|-------|-----------|-----|-------|
| `0x1044` | Batarya seri no | 0 | Batarya 0 |
| `0x1045` | Batarya adresi | 1 | Adres 1 |
| `0x1046` | Iletisim protokolu | 1 | **PYLON** |
| `0x1047` | Asiri gerilim koruma | 5000 | **500.0V** |
| `0x1048` | Sarj gerilimi | 0 | 0V (BMS kontrollu) |
| `0x1049` | Dusuk gerilim koruma | 0 | 0V |
| `0x104A` | Min desarj gerilimi | 420 | **42.0V** |
| `0x104B` | Max sarj akimi | 3500 | **35.00A** |
| `0x104C` | Max desarj akimi | 3500 | **35.00A** |
| `0x104D` | DOD | 80 | **%80** |
| `0x104E` | EOD | 80 | **%80** |
| `0x104F` | Batarya kapasitesi | 46 | **46Ah** |
| `0x1050` | Nominal gerilim | 4500 | **450.0V** |
| `0x1051` | Hucre tipi | 0 | Kursun-asit (ozel) |
| `0x1052` | Off-grid recovery histerezis | 5 | **%5** |
| `0x1053` | *Enable | 0 | Son islem basarili |

#### Remote Kontrol (0x1104-0x110D)
| Adres | Parametre | Raw | Anlam |
|-------|-----------|-----|-------|
| `0x1104` | Remote on/off | 1 | **Acik** |
| `0x1105` | Guc kontrolu | 0 | Tum kontroller kapali |
| `0x1106` | Export max aktif guc | 1000 | **%100.0** |
| `0x1107` | Import max aktif guc | 1000 | **%100.0** |
| `0x1108` | Reaktif guc % | 0 | %0 |
| `0x1109` | Guc faktoru | 100 | **1.00** (unity) |
| `0x110A` | Aktif guc limit hizi | 100 | %100/dk |
| `0x110B` | Reaktif yanit suresi | 1 | 0.1s |
| `0x110C` | Sabit reaktif guc | 0 | 0 kVar |
| `0x110D` | SVG kompanzasyon | 0 | 0 |

#### Enerji Depolama Modu (0x1110)
| Adres | Parametre | Raw | Anlam |
|-------|-----------|-----|-------|
| `0x1110` | Calisma modu | 1 | **Time-sharing tariff** |

#### Time-Sharing Parametreleri (0x1120-0x112F)
| Adres | Parametre | Raw | Anlam |
|-------|-----------|-----|-------|
| `0x1120` | Kural seri no | 0 | Kural 0 (en yuksek oncelik) |
| `0x1121` | Enable | 0 | **Devre disi** |
| `0x1122` | Baslangic saati | 0x0100 | **01:00** |
| `0x1123` | Bitis saati | 0x0500 | **05:00** |
| `0x1124` | Hedef SOC | 100 | **%100** |
| `0x1125-0x1126` | Hedef guc | 0x000009C4 | **2500W** |
| `0x1127` | Gecerlilik baslangic | 0x0101 | **1 Ocak** |
| `0x1128` | Gecerlilik bitis | 0x0C1F | **31 Aralik** |
| `0x1129` | Gecerli gunler | 0x007F | **Tum hafta** (Pzt-Paz) |
| `0x112A` | Kontrol modu | 0 | **Zorla sarj** |
| `0x112B` | Ek ozellik | 0 | Sebekeden sarj kapali |
| `0x112C` | Hedef SOC 2 | 0 | — |
| `0x112F` | *Write enable | 0 | Son islem basarili |

**Yorum:** Gece 01:00-05:00 arasi zorla sarj kurali tanimli ama DEVRE DISI.

---

## YAZMA KURALLARI OZETI (TAMAMLANDI)

### Temel Kurallar
1. **Her zaman FC 0x10 (writem) kullan** — FC 0x06 (write) bu invertorde CALISMAZ (timeout, yanit yok)
2. **Enable register'li bloklarda** tum shadow + enable'i tek FC 0x10 komutunda gonder
3. **Enable register'siz bloklarda** tek register bile writem ile yazilabilir
4. **Yazmadan once her zaman oku** — mevcut degeri bil, istenmeyen degisiklik yapma
5. **Yazdiktan sonra geri oku** — dogrulama yap, enable register `0x0000` donmeli
6. **writem max 20 register** destekler (firmware'de artirildi)

### Blok Tipleri Hizli Referans
| Tip | Nasil Yazilir | Ornek |
|-----|---------------|-------|
| Enable blok | Tum shadow + enable tek FC 0x10 | `writem 1004 ... 0001` |
| Bireysel blok | Tek register FC 0x10 | `writem 1110 0002` |
| Karisik blok | Register bazinda farkli (test et) | Fonksiyon param: 0x1060 ✅, 0x1061 ❌ |
| Yazilamaz | Deneme — exception alirsin | Safety (0x01), PV config (0x02) |

### Tehlikeli Islemler ⚠️
- **0x1104 = 0x0000** yazmak invertoru KAPATIR
- **0x100B-0x100F** degistirmek RS485 iletisimi bozabilir (baud, slave ID)
- **0x0800-0x0A09** safety params — FC 0x10 zaten reddediliyor (0x01), ama denemeyin
- **0x1044-0x1053** batarya param — yanlis deger batarya hasarina yol acabilir

### Exception Kodlari Anlamlari
| Exception | Ne Yapilmali |
|-----------|-------------|
| 0x01 (Illegal Function) | Bu adres araligi yazma desteklemiyor — DENEME |
| 0x02 (Invalid Address) | Register mevcut degil veya enable blogun parcasi — tam blok dene |
| 0x03 (Illegal Data Value) | Enable blogun parcasi ama enable olmadan yazilmis — tam blok + enable dene |
| 0x04 (Slave Device Failure) | Register mevcut ama cihaz kosullari uygun degil — invertor durumunu kontrol et |

### Dogrulanan Tum Yazilabilir Register Listesi

**Enable Bloklari (6 blok, toplam 62 register):**
- Tarih/Saat: 0x1004-0x100A (7 reg)
- RS485: 0x100B-0x100F (5 reg)
- Batarya: 0x1044-0x1053 (16 reg)
- Zamanli Sarj: 0x1111-0x111F (15 reg)
- Time-Sharing: 0x1120-0x112F (16 reg)
- Off-Grid: 0x1144-0x1146 (3 reg)

**Bireysel Yazilabilir (15 register):**
- 0x1060 (PCC guc algilama)
- 0x1064 (Kuru kontak kontrolu)
- 0x1069 (PCC bias gucu)
- 0x106C (Akilli yuk modu secimi)
- 0x1073 (Batarya kullanim konfigurasyonu)
- 0x10A5 (BLE Bluetooth enable)
- 0x1104 (Remote on/off)
- 0x1105 (Guc kontrolu bitleri)
- 0x1106 (Export max aktif guc)
- 0x1107 (Import max aktif guc)
- 0x1108 (Reaktif guc %)
- 0x1109 (Guc faktoru)
- 0x110A (Aktif guc limit hizi)
- 0x110B (Reaktif yanit suresi)
- 0x1110 (Enerji depolama modu)

**TOPLAM: 77 dogrulanan yazilabilir register**

### Test Edilmeyen / Okunabilen Ama Yazilamayan Bloklar

Asagidaki bloklar okunabiliyor ama yazma testi basarisiz oldu veya yapilmadi:

| Blok | Adres Araligi | Okuma | Yazma | Not |
|------|---------------|-------|-------|-----|
| Arc Algilama | 0x1084-0x108E | ✅ (tum 0) | ❌ 0x02 | |
| PLC Ayarlari | 0x10A0-0x10A4 | ✅ (tum 0) | ❌ 0x02 | |
| BLE Iletisim | 0x10A6-0x10AA | ✅ | Denenmedi | 0x10A5 yazilabilir |
| PID Kontrolu | 0x10AF-0x10B4 | ✅ (tum 0) | ❌ 0x02 | |
| Harmonik Enerji | 0x1093-0x1095 | ✅ (tum 0) | Denenmedi | |
| Passive Mod | 0x1184-0x1190 | ✅ (tum 0) | ❌ 0x03/0x02 | Enable blok olabilir |
| Scheduler Mod | 0x1191-0x11A9 | ✅ (tum 0) | ❌ 0x02 | |
| Santral Iletisim | 0x11C4-0x11C8 | ✅ (tum 0) | ❌ 0x02 | |
| Fonksiyon Param Ext | 0x106B-0x1076 | ✅ | Karisik | 0x106C, 0x1073 ✅, diger ❌ |
