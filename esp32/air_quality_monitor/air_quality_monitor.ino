// air_quality_monitor.ino
// Luftqualitätsmessung mit ESP32: Sensoren, Display, SD-Logging und Webserver

#include "globals.h"

// --------------------------------------------------
// Kommunikation und Display
// --------------------------------------------------

#include <HardwareSerial.h>       // UART-Kommunikation (PMS5003-Sensor)
#include <Adafruit_GFX.h>         // Grafikbibliothek (Basis für TFT)
#include <Adafruit_ILI9341.h>     // TFT-Display Treiber (ILI9341)

// --------------------------------------------------
// Bus‑Protokolle und SD‑Karte
// --------------------------------------------------

#include <SPI.h>                  // SPI-Bus (TFT-Display, SD-Karte)
#include <SD.h>                   // SD-Karten-Zugriff
#include <Wire.h>                 // I2C-Bus (SCD41, BME680, DS3231)

// --------------------------------------------------
// Sensor‑Libs
// --------------------------------------------------

#include <SensirionI2cScd4x.h>    // SCD41 (CO2, Temperatur, Luftfeuchte)
#include <Adafruit_Sensor.h>      // Adafruit Sensor-Basisklasse
#include <bsec.h>                 // Bosch BSEC für BME680 (IAQ, VOC, CO2-Äq.)
#include "RTClib.h"               // DS3231 Echtzeituhr

// --------------------------------------------------
// WLAN / WebServer
// --------------------------------------------------

#include <WiFi.h>
#include <WebServer.h>
#include "secrets.h"

// ------------------------- Pins -----------------------

// SD-Pins
#define SD_CS    15               // Chip Select
#define SD_MISO  19               // Master In Slave Out (Daten lesen)

// Display-Pins 
#define TFT_CS   5                // Chip Select
#define TFT_DC   2                // Data/Command
#define TFT_RST  4                // Reset
#define TFT_MOSI 23               // Master Out Slave In (Daten schreiben)
#define TFT_SCLK 18               // Serial Clock

// I2C-Pins
#define I2C_SDA 21                // Serial Data
#define I2C_SCL 22                // Serial Clock

// -------------------- Objekte -------------------------

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
HardwareSerial   pmsSerial(2);   

SensirionI2cScd4x scd4x;
Bsec iaqSensor;
RTC_DS3231        rtc;

WebServer server(80);

// ----------------- Zeitsteuerung ----------------------

unsigned long       lastUpdate      = 0;
const unsigned long updateInterval  = 1000;           // 1 Sekunde

unsigned long       lastStatusPrint = 0;
const unsigned long statusInterval  = 10000;          // 10 Sekunden

unsigned long       lastSdLog       = 0;
unsigned long       sdInterval      = 300000;         // 5 Minuten             

static unsigned long lastI2CRead    = 0;
const unsigned long  i2cInterval    = 5000;           // 5 Sekunden   

// ----------------- Messwerte --------------------------

// PMS5003 Feinstaubsensor
// atm = reale Umgebungsluft (empfohlen für Anzeige und Bewertung)
uint16_t pm1_atm                    = 0;                // PM1.0 in µg/m³ (Partikel < 1 µm)
uint16_t pm25_atm                   = 0;                // PM2.5 in µg/m³ (Partikel < 2.5 µm)
uint16_t pm10_atm                   = 0;                // PM10 in µg/m³ (Partikel < 10 µm)
// CF1 = Laborkalibrierung 
uint16_t pm1_cf1                    = 0;               
uint16_t pm25_cf1                   = 0;
uint16_t pm10_cf1                   = 0;
// n   = Partikelanzahl pro 0.1L Luft (für erweiterte Auswertungen)
uint16_t pm_n03                     = 0;                // Anzahl Partikel >0.3 µm pro 0.1L Luft
uint16_t pm_n05                     = 0;                // Anzahl Partikel >0.5 µm pro 0.1L Luft
uint16_t pm_n10                     = 0;                // Anzahl Partikel >1   µm pro 0.1L Luft
uint16_t pm_n25                     = 0;                // Anzahl Partikel >2.5 µm pro 0.1L Luft
uint16_t pm_n50                     = 0;                // Anzahl Partikel >5   µm pro 0.1L Luft
uint16_t pm_n100                    = 0;                // Anzahl Partikel >10  µm pro 0.1L Luft

// SCD41
uint16_t co2_ppm                    = 0;                // CO2-Wert in ppm
float    scd_temp                   = 0;                // Temperatur in °C
float    scd_rh                     = 0;                // Relative Luftfeuchtigkeit in %

// BME680 Umweltsensor mit BSEC Kalibrierungsdaten
float    bme_iaq                    = 0;                // IAQ Index (0-500)
uint8_t  bme_iaq_accuracy           = 0;                // Kalibrierungsstatus (0-3)
float    bme_static_iaq             = 0;                // Static IAQ (für stationäre Geräte)
float    bme_co2_eq                 = 0;                // CO2-Äquivalent in ppm (geschätzt)
float    bme_voc_eq                 = 0;                // b-VOC-Äquivalent in ppm (geschätzt)
float    bme_temp_raw               = 0;                // Rohtemperatur in °C
float    bme_press_hPa              = 0;                // Luftdruck in hPa
float    bme_hum_raw                = 0;                // Rohluftfeuchtigkeit in %
float    bme_gas_resistance         = 0;                // Gaswiderstand in Ohm
float    bme_comp_temp              = 0;                // Kompensierte Temperatur in °C
float    bme_comp_hum               = 0;                // Kompensierte Luftfeuchtigkeit in %
float    bme_gas_percentage         = 0;                // Gasanteil relativ zur Sensorhistorie in %
float    bme_stabilization_status   = 0;                // Sensor-Einlaufphase abgeschlossen (1.0 = ja)
float    bme_run_in_status          = 0;                // Sensor-Stabilisierung abgeschlossen (1.0 = ja)

// --------- RTC-/Logging-Status -----------------------

bool     rtcIsSet        = false;                       // RTC wurde per Weboberfläche gesetzt und freigegeben
uint32_t lastLogUnix     = 0;                           // Unix-Timestamp des letzten Log-Eintrags
bool     lastLogUnixValid= false;                       // true sobald erster Log-Eintrag erfolgt ist
uint32_t logIndex        = 0;                           // fortlaufende Nummer der Log-Einträge

// --------------------------------------------------
// Hilfsfunktionen Zeit / Log
// --------------------------------------------------

// aktuelle Zeit als Unix-Timestamp holen
uint32_t getRtcUnix() {
  DateTime now = rtc.now();
  return now.unixtime();
}

// Lädt letzten Log-Eintrag von SD um logIndex und lastLogUnix fortzusetzen
// Verhindert doppelte Eintrags-Nummern und Zeitfehler nach Neustart
void loadLastLogUnix() {
  File f = SD.open("/logs/log_complete.csv", FILE_READ);
  if (!f) return;

  size_t fileSize = f.size();
  if (fileSize < 20) { f.close(); return; }

  size_t pos = fileSize - 1;
  f.seek(pos);
  if (f.read() == '\n' && pos > 0) pos--;

  String lastLine = "";
  while (pos > 0) {
    f.seek(pos);
    char c = f.read();
    if (c == '\n') break;
    pos--;
  }
  f.seek(pos == 0 ? 0 : pos + 1);
  lastLine = f.readStringUntil('\n');
  lastLine.trim();
  f.close();

  if (lastLine.length() < 20) return;
  if (lastLine[0] < '0' || lastLine[0] > '9') return;

  int sep1 = lastLine.indexOf(';');
  if (sep1 < 0) return;

  logIndex = lastLine.substring(0, sep1).toInt();
  String rest = lastLine.substring(sep1 + 1);
  if (rest.length() < 19) return;

  int day   = rest.substring(0, 2).toInt();
  int month = rest.substring(3, 5).toInt();
  int year  = rest.substring(6,10).toInt();
  int hour  = rest.substring(11,13).toInt();
  int min   = rest.substring(14,16).toInt();
  int sec   = rest.substring(17,19).toInt();

  DateTime dt(year, month, day, hour, min, sec);
  lastLogUnix      = dt.unixtime();
  lastLogUnixValid = (lastLogUnix > 0);
}

// Vorwärtsdeklarationen externer Funktionen
void handleRoot();
void handleInfo();
void handleLogTable();
void handleLogDownload();
void handleSetTimeForm();
void handleSetTimePost();
void handleSensorData();
void updateDisplay();
void logToSdIfDue();

// --------------------------------------------------
// setup()
// --------------------------------------------------

void setup() {
  // Serielle Schnittstellen und Busse initialisieren
  Serial.begin(115200);
  pmsSerial.begin(9600, SERIAL_8N1, 16, 17);
  Wire.begin(I2C_SDA, I2C_SCL);
  SPI.begin(TFT_SCLK, SD_MISO, TFT_MOSI, TFT_CS);

  // Display initialisieren
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLUE);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);

  // WLAN verbinden
  Serial.print("Verbinde mit WLAN : ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long wifiStart = millis();
  while (WiFi.status()!=WL_CONNECTED && millis()-wifiStart<15000) {
    delay(500); Serial.print(".");
  }
  Serial.println();
  if (WiFi.status()==WL_CONNECTED) {
    Serial.print("WLAN verbunden, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WLAN NICHT verbunden (Timeout)");
  }

  // SD-Karte initialisieren und letzten Log-Eintrag laden
  if (!SD.begin(SD_CS)) {
    Serial.println("SD init failed!");
  } else {
    Serial.println("SD init OK");  
    loadLastLogUnix();
  }

  // RTC initialisieren
  if (!rtc.begin()) {
    Serial.println("Konnte DS3231 nicht finden :(");
  } else {
    Serial.println("DS3231 RTC gefunden");
    DateTime rtcNow = rtc.now();
    Serial.printf("RTC Startzeit: %02d.%02d.%04d %02d:%02d:%02d\n",
                  rtcNow.day(),rtcNow.month(),rtcNow.year(),
                  rtcNow.hour(),rtcNow.minute(),rtcNow.second());
    rtcIsSet = false;
    Serial.println("RTC ist noch nicht freigegeben. Bitte Zeit per Weboberflaeche setzen.");
  }

// SCD41 CO2-Sensor initialisieren
  const uint8_t SCD41_ADDRESS = 0x62;

  scd4x.begin(Wire, SCD41_ADDRESS);
  uint16_t error = scd4x.stopPeriodicMeasurement();

  // Temperatur-Offset kalibrieren (kompensiert Eigenwaerme der Platine).
  // Berechnung: T_offset_neu = T_SCD41 - T_Referenz + T_offset_alt
  // Gemessen: 23.2 - 22.3 + 4.0 = 4.9 degC
  // Datenblatt SCD4x Kap. 3.7.1: muss im Idle-Zustand gesetzt werden.
  const float SCD41_TEMP_OFFSET = 4.9f;
  error = scd4x.setTemperatureOffset(SCD41_TEMP_OFFSET);
  if (error) {
    Serial.println("Fehler beim Setzen des SCD41 Temperatur-Offsets");
  } else {
    Serial.print("SCD41 Temperatur-Offset gesetzt: ");
    Serial.print(SCD41_TEMP_OFFSET);
    Serial.println(" degC");
  }
  // Offset wurde einmalig ins EEPROM geschrieben (persist_settings).
  // Datenblatt Kap. 3.10.1: max. 800 ms, ~2000 Schreibzyklen.
  // ( Nur bei Aenderung des Offsets erneut einkommentieren und ausfuehren: )
  // error = scd4x.persistSettings();

  error = scd4x.startPeriodicMeasurement();
  if (error) {
    Serial.println("Fehler beim Start von SCD41-Messung");
  } else {
    Serial.println("SCD41 gestartet");
  }

  // BME680 / BSEC initialisieren
  iaqSensor.begin(BME68X_I2C_ADDR_LOW, Wire);
  if (iaqSensor.bsecStatus != BSEC_OK) {
    Serial.println("BSEC init fehlgeschlagen, Status: "
                    + String(iaqSensor.bsecStatus));
  } else {
    Serial.println("BSEC init OK");

    // Gewuenschte Messwerte abonnieren
    bsec_virtual_sensor_t sensorList[] = {
        BSEC_OUTPUT_IAQ,
        BSEC_OUTPUT_STATIC_IAQ,
        BSEC_OUTPUT_CO2_EQUIVALENT,
        BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
        BSEC_OUTPUT_RAW_TEMPERATURE,
        BSEC_OUTPUT_RAW_PRESSURE,
        BSEC_OUTPUT_RAW_HUMIDITY,
        BSEC_OUTPUT_RAW_GAS,
        BSEC_OUTPUT_STABILIZATION_STATUS,
        BSEC_OUTPUT_RUN_IN_STATUS,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
        BSEC_OUTPUT_GAS_PERCENTAGE
    };

    // Anzahl der abonnierten Sensoren berechnen
    int sensorCount = sizeof(sensorList) / sizeof(sensorList[0]);

    iaqSensor.updateSubscription(
      sensorList,       // Liste der gewuenschten Messwerte
      sensorCount,      // Anzahl der Eintraege in der Liste
      BSEC_SAMPLE_RATE_LP  // Low Power: Messung alle 3 Sekunden
    );

  if (iaqSensor.bsecStatus != BSEC_OK) {
        Serial.println("BSEC subscription fehlgeschlagen: "
                      + String(iaqSensor.bsecStatus));
      } else {
        Serial.println("BSEC subscription OK");
      }
    }

  // Webserver-Routen registrieren
  server.on("/",                 handleRoot);           // Hauptseite: aktuelle Messwerte
  server.on("/log",              handleLogTable);       // Log als HTML-Tabelle
  server.on("/download",         handleLogDownload);    // Log als CSV-Download
  server.on("/info",             handleInfo);           // Sensor-Infos
  server.on("/settime",          handleSetTimeForm);    // Formular zum Setzen der RTC-Zeit
  server.on("/settime_post",     HTTP_POST, handleSetTimePost);  // Verarbeitet das Zeitformular (POST)
  server.on("/sensors",          handleSensorData);    // API: alle Sensordaten als JSON

  server.begin();
  Serial.println("HTTP-Server gestartet");
}

// --------------------------------------------------
// loop()
// --------------------------------------------------

void loop() {
  // Web-Client-Anfragen bearbeiten
  server.handleClient();

  // SCD41 alle 5 Sekunden auslesen
  if (millis() - lastI2CRead >= i2cInterval) {
    lastI2CRead = millis();

    bool dataReady = false;
    scd4x.getDataReadyStatus(dataReady);

    if (dataReady) {
      // SCD41 CO2-Sensor auslesen
      uint16_t co2;
      float temperature, humidity;
      uint16_t error = scd4x.readMeasurement(co2, temperature, humidity);

      // Messwerte mit Timestamp auf Serial ausgeben
      DateTime now = rtc.now();
      Serial.print(now.day());   Serial.print(".");
      Serial.print(now.month()); Serial.print(".");
      Serial.print(now.year());  Serial.print(" ");
      Serial.print(now.hour());  Serial.print(":");
      Serial.print(now.minute());Serial.print(":");
      Serial.print(now.second());
      Serial.print("  ");

      Serial.print("SCD41 read, error=");
      Serial.print(error);
      Serial.print(" co2=");
      Serial.print(co2);
      Serial.print(" temp=");
      Serial.print(temperature);
      Serial.print(" rh=");
      Serial.println(humidity);

      // SCD41 Werte speichern wenn gültig
      if (!error && co2 != 0) {
        co2_ppm  = co2;
        scd_temp = temperature;
        scd_rh   = humidity;
      } 
    }
  }
  
  // BME680 / BSEC auslesen (run() gibt true wenn neue Messwerte bereit sind)
  if (iaqSensor.run()) {
    bme_iaq            = iaqSensor.iaq;
    bme_iaq_accuracy   = iaqSensor.iaqAccuracy;
    bme_static_iaq     = iaqSensor.staticIaq;
    bme_co2_eq         = iaqSensor.co2Equivalent;
    bme_voc_eq         = iaqSensor.breathVocEquivalent;
    bme_temp_raw       = iaqSensor.rawTemperature;
    bme_press_hPa      = iaqSensor.pressure / 100.0;
    bme_hum_raw        = iaqSensor.rawHumidity;
    bme_gas_resistance = iaqSensor.gasResistance;
    bme_comp_temp      = iaqSensor.temperature;
    bme_comp_hum       = iaqSensor.humidity;
    bme_gas_percentage = iaqSensor.gasPercentage;
    bme_stabilization_status = iaqSensor.stabStatus;
    bme_run_in_status        = iaqSensor.runInStatus;
  }
    
  // PMS5003 Feinstaubsensor auslesen (über UART)
  // Der Sensor sendet jede Sekunde einen 32-Byte-Frame.
  // Aufbau: 2 Startbytes (0x42 0x4D), 2 Byte Framelänge (immer 28), dann Messwerte.
  if (pmsSerial.available() >= 32) {

    byte b1 = pmsSerial.read();
    byte b2 = pmsSerial.read();

    if (b1 == 0x42 && b2 == 0x4D) {

      // Framelänge lesen (2 Bytes zu einem 16-Bit-Wert kombinieren)
      uint16_t frameLen = (pmsSerial.read() << 8) | pmsSerial.read();

      // Framelänge prüfen — PMS5003 sendet immer 28
      // Andere Werte bedeuten: falscher Frame-Einstiegspunkt trotz passender Startbytes
      if (frameLen != 28) {
        while (pmsSerial.available() > 0) {
          pmsSerial.read();
        }
      } else {

        // CF1-Werte (Standard-Partikel, Laborbedingungen)
        pm1_cf1  = (pmsSerial.read() << 8) | pmsSerial.read();
        pm25_cf1 = (pmsSerial.read() << 8) | pmsSerial.read();
        pm10_cf1 = (pmsSerial.read() << 8) | pmsSerial.read();

        // ATM-Werte (atmospheric = für Außenluft kalibriert, empfohlen für Anzeige)
        pm1_atm  = (pmsSerial.read() << 8) | pmsSerial.read();
        pm25_atm = (pmsSerial.read() << 8) | pmsSerial.read();
        pm10_atm = (pmsSerial.read() << 8) | pmsSerial.read();

        // Partikelanzahl pro 0.1L Luft (für erweiterte Auswertungen)
        pm_n03  = (pmsSerial.read() << 8) | pmsSerial.read();
        pm_n05  = (pmsSerial.read() << 8) | pmsSerial.read();
        pm_n10  = (pmsSerial.read() << 8) | pmsSerial.read();
        pm_n25  = (pmsSerial.read() << 8) | pmsSerial.read();
        pm_n50  = (pmsSerial.read() << 8) | pmsSerial.read();
        pm_n100 = (pmsSerial.read() << 8) | pmsSerial.read();

        // Rest des Frames verwerfen (enthält Checksum, wird nicht ausgewertet)
        for (int i = 0; i < frameLen - 2 * 12 - 2; i++) {
          if (pmsSerial.available()) pmsSerial.read();
        }

        // PM-Werte auf Serial ausgeben
        Serial.print("PM 1.0: "); Serial.print(pm1_atm);  Serial.print(" ug/m3  ");
        Serial.print("PM 2.5: "); Serial.print(pm25_atm); Serial.print(" ug/m3  ");
        Serial.print("PM 10: ");  Serial.print(pm10_atm); Serial.println(" ug/m3");
      }

    } else {
      // Startbytes ungueltig (mitten im Frame) -> Puffer leeren und beim naechsten Frame neu starten
      while (pmsSerial.available() > 0) {
        pmsSerial.read();
      }
    }
  }

  // Display aktualisieren (max. alle 1 Sekunde)
  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();
    updateDisplay();
  }

  // SD-Logging prüfen (alle 5 Minuten)
  logToSdIfDue();

  // Status-Ausgabe alle 10 Sekunden
  if (millis() - lastStatusPrint >= statusInterval) {
    lastStatusPrint = millis();

    Serial.println("---- STATUS ----");

    wl_status_t st = WiFi.status();
    Serial.print("WiFi-Status: "); 
    Serial.print(st);
    if (st==WL_CONNECTED) { 
      Serial.print("  IP: "); 
      Serial.print(WiFi.localIP()); 
    }
    Serial.println();

    DateTime nowStatus = rtc.now();
    Serial.print("RTC: ");
    Serial.print(nowStatus.day());   Serial.print(".");
    Serial.print(nowStatus.month()); Serial.print(".");
    Serial.print(nowStatus.year());  Serial.print(" ");
    Serial.print(nowStatus.hour());  Serial.print(":");
    Serial.print(nowStatus.minute());Serial.print(":");
    Serial.println(nowStatus.second());

    Serial.print("RTC freigegeben: ");
    if (rtcIsSet) {
      Serial.println("ja");
    } else {
      Serial.println("nein");
    }

    Serial.print("SD present: ");
    if (SD.cardType() != CARD_NONE) {
      Serial.println("yes");
    } else {
      Serial.println("no");
    }

    Serial.println("--------------");
  }
}
