# Innenluft-Monitor mit Display, SD-Logging und Webserver

> Privates Lern- und Übungsprojekt mit KI-Unterstützung,
> in Entwicklung seit Oktober 2025.

---

## Hardware

| Komponente | Typ | Schnittstelle |
|---|---|---|
| **MCU** | ESP32 Dev Module | — |
| **Display** | ILI9341 TFT 3.2" | SPI |
| **Feinstaubsensor** | PMS5003 | UART2 (GPIO16/17) |
| **CO₂-Sensor** | Sensirion SCD41 | I²C (0x62) |
| **Umweltsensor** | Bosch BME680 (BSEC) | I²C (0x76) |
| **Echtzeituhr** | DS3231 | I²C (0x68) |
| **Speicher** | µSD-Karte | SPI |

---

## Gemessene Größen

- **PMS5003:** PM1.0 / PM2.5 / PM10 in µg/m³ (atmospheric + CF1), Partikelanzahl in 6 Größenklassen.
- **SCD41:** CO₂ (ppm), Temperatur (°C), relative Luftfeuchte (%).
- **BME680 / BSEC:** IAQ, Static IAQ, CO₂-Äquivalent, VOC-Äquivalent, Temperatur, Luftdruck, Luftfeuchte, Gaswiderstand, Gasprozent, Stabilization- und Run-in-Status.
- **DS3231:** Datum und Uhrzeit.

---

## Funktionen

### Display (ILI9341)
* Aktualisierung jede Sekunde.
* Datum, Uhrzeit und alle Messwerte im übersichtlichen Spaltenlayout.
* AQI-Platzhalter für spätere Dashboard-Berechnung integriert.

### SD-Logging (Intervall: 5 Minuten)
* Tages-CSV unter `/logs/YYYY/MM/YYYY-MM-DD.csv`
* Gesamtlog unter `/logs/log_complete.csv`
* 31 Spalten, Semikolon-getrennt
* Plausibilitätsprüfung per RTC-Timestamp (Zeitfehler werden explizit geloggt)
* **Persistence:** Nach Neustart wird der letzte Index geladen und der Timestamp fortgeführt

### Webserver (Port 80)
| Route | Funktion |
|---|---|
| `/` | Hauptseite mit aktuellen Messwerten |
| `/log` | Log als sortierbare HTML-Tabelle |
| `/download` | `log_complete.csv` als Download |
| `/settime` | RTC-Zeit per Formular setzen |
| `/info` | Sensor-Infos und Bewertungsskalen |
| `/sensors` | Alle Sensordaten als JSON (CORS-enabled) |

*Die `/sensors`-API liefert alle Rohdaten als JSON für den Zugriff aus dem lokalen Netzwerk (z.B. für Home Assistant oder eigene Dashboards).*

---

## Projektstruktur

```text
air_quality_monitor/
├── air_quality_monitor.ino  — Hauptdatei: setup(), loop(), Sensoren
├── display.cpp               — TFT-Ausgabe & Layout
├── log.cpp                   — SD-Logging & Dateiverwaltung
├── web.cpp                   — HTTP-Handler & Web-Interface
├── globals.h                 — Gemeinsame Variablen & Objekte
└── secrets.h                 — WLAN-Konfiguration
```
---

## Abhängigkeiten (Arduino-Bibliotheken)

- `Adafruit GFX Library`
- `Adafruit ILI9341`
- `SensirionI2cScd4x`
- `BSEC Software Library` (Bosch)
- `RTClib`
- `SD` (ESP32-Core)
- `WiFi`, `WebServer` (ESP32-Core)

---

## Konfiguration & Einrichtung

### 1. Initialisierung
* **WLAN & Zugangsdaten:** Trage in die Datei `secrets.h` Deine Daten ein (die `secrets.h` wird durch die `.gitignore` ignoriert)

* **SCD41 Temperatur-Offset:** Aktuell auf **4.9°C** eingestellt (Eigenwärme-Kompensation). 

### 2. Wichtige Hardware-Hinweise 
* **DS3231 Echtzeituhr:** Das Modul besitzt eine Ladeschaltung für **LIR2032 Akkus**. Standard CR2032 Batterien können dadurch beschädigt werden/auslaufen! 
  **Ohne Batterie:** Zeit muss nach jedem Start manuell über `/settime` im Browser gesetzt werden. Ohne gültige Zeit erfolgt **kein Logging**.
* **SD-Karte:** * Formatierung ausschließlich mit dem [SD Card Formatter](https://www.sdcard.org/downloads/formatter/).
  **Stromspitzen:** Class-10 Karten können beim Schreiben kurzzeitig hohe Ströme ziehen. Falls es zu Schreibfehlern kommt: Ein **100µF Kondensator** zwischen 5V und GND am SD-Modul schafft Abhilfe.
* **BME680 / BSEC:** * Volle Kalibrierung (`Accuracy 3`) wird erst nach mehreren Stunden erreicht.
  Für das Display und Dashboard wird `bme_static_iaq` verwendet (optimiert für stationäre Geräte).

---

## Geplant / Roadmap
- [ ] **State Persistence:** BSEC-Kalibrierungsdaten im EEPROM sichern für schnelleren Kaltstart
- [ ] **Advanced AQI:** Kombinierter Index aus CO₂, IAQ, PM2.5 und PM10
- [ ] **Modern Dashboard:** React/Node.js Interface basierend auf der `/sensors`-API
- [ ] **Datenspeicherung 2.0:** Migration auf lokale SQLite-Datenbank oder MQTT-Anbindung


