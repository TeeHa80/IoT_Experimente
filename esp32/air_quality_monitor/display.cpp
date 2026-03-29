// display.cpp

#include <Adafruit_GFX.h>
#include "globals.h"

// Spalten-Positionen auf dem Display
const int COL_LABEL     = 5;                    // X-Position des Beschriftungstextes (z.B. "CO2")
const int COL_COLON     = 90;                   // X-Position des Doppelpunkts
const int COL_VALUE     = 110;                  // X-Position des Messwerts
const int LINE_HEIGHT   = 15;                   // Zeilenabstand in Pixeln

// Löscht eine Display-Zeile durch Überschreiben mit blauem Hintergrund
void clearLine(int y) {
  tft.fillRect(0, y, tft.width(), LINE_HEIGHT, ILI9341_BLUE);
}

// Gibt eine Zeile im Format "Label : Wert Einheit" auf dem Display aus
void printLine(int& y, const char* label, float value, const char* unit, int decimals) {
  clearLine(y);
  tft.setCursor(COL_LABEL, y); tft.print(label);
  tft.setCursor(COL_COLON, y); tft.print(":");
  tft.setCursor(COL_VALUE, y); tft.print(value, decimals);
  if (unit[0] != '\0') {
    tft.print(" ");
    tft.print(unit);
  }
  y += LINE_HEIGHT;
}

// --------------------------------------------------
// Display aktualisieren
// --------------------------------------------------
void updateDisplay() {
  int y = 10;  // Startposition Y-Achse

  // Datum anzeigen (Format: DD.MM.YYYY)
  clearLine(y);
  tft.setCursor(COL_LABEL, y); tft.print("Datum");
  tft.setCursor(COL_COLON, y); tft.print(":");
  tft.setCursor(COL_VALUE, y);
  DateTime nowDisp = rtc.now();
  if (nowDisp.day()   < 10) tft.print("0"); tft.print(nowDisp.day());
  tft.print(".");
  if (nowDisp.month() < 10) tft.print("0"); tft.print(nowDisp.month());
  tft.print("."); tft.print(nowDisp.year());
  y += LINE_HEIGHT;

  // Zeit anzeigen (Format: HH:MM:SS)
  clearLine(y);
  tft.setCursor(COL_LABEL, y); tft.print("Zeit");
  tft.setCursor(COL_COLON, y); tft.print(":");
  tft.setCursor(COL_VALUE, y);
  if (nowDisp.hour()   < 10) tft.print("0"); tft.print(nowDisp.hour());
  tft.print(":");
  if (nowDisp.minute() < 10) tft.print("0"); tft.print(nowDisp.minute());
  tft.print(":");
  if (nowDisp.second() < 10) tft.print("0"); tft.print(nowDisp.second());
  y += LINE_HEIGHT;

  // Messwerte anzeigen
  printLine(y, "PM 1.0",  pm1_atm,          "ug/m3",  0);  // Feinstaub PM1.0
  printLine(y, "PM 2.5",  pm25_atm,         "ug/m3",  0);  // Feinstaub PM2.5
  printLine(y, "PM 10 ",  pm10_atm,         "ug/m3",  0);  // Feinstaub PM10
  printLine(y, "CO2   ",  co2_ppm,          "ppm",    0);  // CO2-Konzentration
  printLine(y, "T SCD",   scd_temp,         "C",      1);  // Temperatur SCD41
  printLine(y, "rF SCD",  scd_rh,           "%",      1);  // Luftfeuchtigkeit SCD41
  printLine(y, "T BME",   bme_comp_temp,    "C",      1);  // Temperatur BME680
  printLine(y, "rF BME",  bme_comp_hum,     "%",      1);  // Luftfeuchtigkeit BME680
  printLine(y, "p",       bme_press_hPa,    "hPa",    1);  // Luftdruck
  printLine(y, "VOC",     bme_voc_eq,       "ppm",    2);  // VOC-Äquivalent (BSEC-Schätzwert, Range 0.5-15)
  printLine(y, "stIAQ",   bme_static_iaq,   "",       1);  // BOSCH static IAQ
  printLine(y, "Kalib",   bme_iaq_accuracy, "",       0);  // Kalibrierungsstatus (0-3) 
   
  // AQI-Platzhalter (Berechnung erfolgt extern im Dashboard)
  clearLine(y);
  tft.setCursor(COL_LABEL, y); tft.print("AQI");
  tft.setCursor(COL_COLON, y); tft.print(":");
  tft.setCursor(COL_VALUE, y); tft.print("---");
  y += LINE_HEIGHT;
}
