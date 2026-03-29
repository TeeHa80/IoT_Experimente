// globals.h
#pragma   once
#include  <Arduino.h>
#include  <Adafruit_ILI9341.h>
#include  "RTClib.h"
#include  <WebServer.h>

// --------------------------------------------------
// Hardware-Objekte
// --------------------------------------------------
extern Adafruit_ILI9341 tft;              // TFT-Display (ILI9341)
extern WebServer        server;           // HTTP-Webserver auf Port 80

// --------------------------------------------------
// BME680 Umweltsensor
// --------------------------------------------------
extern float    bme_iaq;                  // BOSCH mobile IAQ Index (0-500)           
extern uint8_t  bme_iaq_accuracy;         // Kalibrierungsstatus (0-noch nicht;1-grob;2-mittel;3-vollständig)
extern float    bme_static_iaq;           // BOSCH static IAQ (0-500, einheitenlos, berechnet aus Temperatur, Luftfeuchtigkeit und Gaswiderstand)
extern float    bme_co2_eq;               // CO2-Äquivalent in ppm (geschätzt)
extern float    bme_voc_eq;               // b-VOC-Äquivalent in ppm (geschätzt)
extern float    bme_temp_raw;             // Rohtemperatur in °C
extern float    bme_press_hPa;            // Luftdruck in hPa
extern float    bme_hum_raw;              // Rohluftfeuchtigkeit in %
extern float    bme_gas_resistance;       // Gaswiderstand in Ohm
extern float    bme_comp_temp;            // Kompensierte Temperatur in °C
extern float    bme_comp_hum;             // Kompensierte Luftfeuchtigkeit in %
extern float    bme_gas_percentage;       // Gasanteil relativ zur Sensorhistorie in %
extern float    bme_stabilization_status; // Einlaufphase abgeschlossen (1.0 = ja)
extern float    bme_run_in_status;        // Sensor stabilisiert (1.0 = ja)

// --------------------------------------------------
// PMS5003 Feinstaub (atm = atmospheric)
// --------------------------------------------------
extern uint16_t pm1_atm;                  // PM1.0 in µg/m³ (Partikel < 1 µm)
extern uint16_t pm25_atm;                 // PM2.5 in µg/m³ (Partikel < 2.5 µm)
extern uint16_t pm10_atm;                 // PM10 in µg/m³ (Partikel < 10 µm)
extern uint16_t pm1_cf1;                  // CF=1 (Standard-Partikel, Laborbedingungen)
extern uint16_t pm25_cf1;
extern uint16_t pm10_cf1;
extern uint16_t pm_n03;                   // Anzahl Partikel >0.3 µm pro 0.1L Luft
extern uint16_t pm_n05;                   // Anzahl Partikel >0.5 µm pro 0.1L Luft
extern uint16_t pm_n10;                   // Anzahl Partikel >1   µm pro 0.1L Luft
extern uint16_t pm_n25;                   // Anzahl Partikel >2.5 µm pro 0.1L Luft
extern uint16_t pm_n50;                   // Anzahl Partikel >5   µm pro 0.1L Luft
extern uint16_t pm_n100;                  // Anzahl Partikel >10  µm pro 0.1L Luft

// --------------------------------------------------
// SCD41 CO2-Sensor
// --------------------------------------------------
extern uint16_t co2_ppm;                  // CO2-Wert in ppm
extern float    scd_temp;                 // Temperatur in °C
extern float    scd_rh;                   // Relative Luftfeuchtigkeit in %

// --------------------------------------------------
// RTC / Logging
// --------------------------------------------------
extern          RTC_DS3231 rtc;           // DS3231 Echtzeituhr (Real-Time Clock)
extern bool     rtcIsSet;                 // RTC wurde per Weboberfläche freigegeben
extern bool     lastLogUnixValid;         // Ist der letzte Log-Timestamp gültig?
extern uint32_t lastLogUnix;              // Unix-Timestamp des letzten Log-Eintrags
extern uint32_t logIndex;                 // Fortlaufende Nummer der Log-Einträge
extern unsigned long lastSdLog;           // Zeitstempel des letzten SD-Schreibvorgangs
extern unsigned long sdInterval;          // Logging-Intervall in Millisekunden
