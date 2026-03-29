// log.cpp

#include "globals.h"
#include "RTClib.h"
#include <SD.h>

uint32_t getRtcUnix();                  // Liest aktuelle RTC-Zeit und gibt sie als Unix-Timestamp zurück             
void     logToSdIfDue();                // Schreibt Messwerte auf SD-Karte wenn das Logging-Intervall abgelaufen ist

// Hilfsfunktion: macht aus 3 -> "03", lässt 10 -> "10"
String padZero(int number) {
    if (number < 10) return "0" + String(number);
    return String(number);
}

// Gibt den Pfad zur heutigen Log-Datei zurück und legt die Ordner an
String getDailyPath() {
    DateTime today = rtc.now();
    
    // Ordner anlegen (macht nichts wenn schon vorhanden)
    SD.mkdir("/logs");
    SD.mkdir("/logs/" + String(today.year()));
    SD.mkdir("/logs/" + String(today.year()) + "/" + padZero(today.month()));
    
    // Pfad zusammenbauen: /logs/2026/02/2026-02-16.csv
    return "/logs/" 
           + String(today.year()) 
           + "/" + padZero(today.month()) 
           + "/" + String(today.year()) 
           + "-" + padZero(today.month()) 
           + "-" + padZero(today.day()) 
           + ".csv";
}

// Prüft ob die Datei neu ist (noch nicht existiert)
bool isNewFile(String path) {
    return !SD.exists(path);
}

// Schreibt den CSV-Header in eine Datei
void writeHeader(File file) {
    file.print("#;Date;Time;");
    file.print("PM1;PM2.5;PM10;");
    file.print("PM1_CF1;PM2.5_CF1;PM10_CF1;");
    file.print("Particles_0.3;Particles_0.5;Particles_1.0;Particles_2.5;Particles_5.0;Particles_10;");
    file.print("CO2;Temp_SCD;RH_SCD;");
    file.print("IAQ;IAQ_Accuracy;Static_IAQ;CO2_Equivalent;VOC_Equivalent;");
    file.print("Temp_Raw;Pressure;RH_Raw;Gas_Resistance;");
    file.print("Temp_Comp;RH_Comp;Gas_Percent;");
    file.println("Stabilization;RunIn");
}

// Schreibt einen Log-Eintrag in die Datei
void writeEntry(File file, DateTime time, uint32_t index) {
    char date[11], clock[9];
    snprintf(date, sizeof(date), "%02d.%02d.%04d", time.day(), time.month(), time.year());
    snprintf(clock, sizeof(clock), "%02d:%02d:%02d", time.hour(), time.minute(), time.second());
    
    file.print(index); file.print(";");
    file.print(date); file.print(";");
    file.print(clock); file.print(";");
    
    // PMS5003
    file.print(pm1_atm); file.print(";");
    file.print(pm25_atm); file.print(";");
    file.print(pm10_atm); file.print(";");
    file.print(pm1_cf1); file.print(";");
    file.print(pm25_cf1); file.print(";");
    file.print(pm10_cf1); file.print(";");
    file.print(pm_n03); file.print(";");
    file.print(pm_n05); file.print(";");
    file.print(pm_n10); file.print(";");
    file.print(pm_n25); file.print(";");
    file.print(pm_n50); file.print(";");
    file.print(pm_n100); file.print(";");
    
    // SCD41
    file.print(co2_ppm); file.print(";");
    file.print(scd_temp, 1); file.print(";");
    file.print(scd_rh, 1); file.print(";");
    
    // BME680 / BSEC
    file.print(bme_iaq, 1); file.print(";"); 
    file.print(bme_iaq_accuracy); file.print(";");
    file.print(bme_static_iaq, 1); file.print(";");    
    file.print(bme_co2_eq, 0); file.print(";");
    file.print(bme_voc_eq, 2); file.print(";");
    file.print(bme_temp_raw, 1); file.print(";");
    file.print(bme_press_hPa, 1); file.print(";");
    file.print(bme_hum_raw, 1); file.print(";");
    file.print(bme_gas_resistance, 0); file.print(";");
    file.print(bme_comp_temp, 1); file.print(";");
    file.print(bme_comp_hum, 1); file.print(";");
    file.print(bme_gas_percentage, 1); file.print(";");
    file.print(bme_stabilization_status, 0); file.print(";");
    file.println(bme_run_in_status, 0);
}

// Hauptfunktion: loggt alle 5 Minuten in Tages-CSV und log_complete.csv
void logToSdIfDue() {
    if (millis() - lastSdLog < sdInterval) return;
    lastSdLog = millis();
    
    if (!rtcIsSet) {
        Serial.println("RTC not set -> no logging");
        return;
    }
    
    DateTime now = rtc.now();
    
    // RTC-Zeit plausibel prüfen
    uint32_t nowUnix = getRtcUnix();
    if (lastLogUnixValid && nowUnix < lastLogUnix) {
        Serial.println("WARNUNG: RTC-Zeit kleiner als letzter Log -> ZEITFEHLER");
        // Fehler in log_complete.csv schreiben
        File errorFile = SD.open("/logs/log_complete.csv", FILE_APPEND);
        if (errorFile) {
            char datum[11], uhrzeit[9];
            snprintf(datum, sizeof(datum), "%02d.%02d.%04d", now.day(), now.month(), now.year());
            snprintf(uhrzeit, sizeof(uhrzeit), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
            errorFile.print(datum); errorFile.print(";");
            errorFile.print(uhrzeit); errorFile.print(";");
            errorFile.println("ZEITFEHLER;RTC_UNPLAUSIBEL;;;;;;;;;;;;;;;;;;;;;;;;;;;;;");
            errorFile.close();
        }
        return;
    }

    // Timestamp für nächsten Durchlauf merken
    lastLogUnix = nowUnix;
    lastLogUnixValid = true;
    
    // Pfad zur heutigen Datei
    String dailyPath = getDailyPath();
    bool dailyFileNew = isNewFile(dailyPath);
    
    // Tages-Datei öffnen
    File dailyFile = SD.open(dailyPath, FILE_APPEND);
    if (!dailyFile) {
        Serial.println("Error: Daily CSV could not be opened");
        return;
    }
    
    // Wenn neu: Header schreiben
    if (dailyFileNew) {
        writeHeader(dailyFile);
    }
    
    // Eintrag in Tages-Datei schreiben
    writeEntry(dailyFile, now, ++logIndex);
    dailyFile.close();
    
    // log_complete.csv öffnen (oder anlegen wenn noch nicht da)
    bool allFileNew = isNewFile("/logs/log_complete.csv");
    File allFile = SD.open("/logs/log_complete.csv", FILE_APPEND);
    
    if (allFile) {
        if (allFileNew) {
            writeHeader(allFile);
        }
        writeEntry(allFile, now, logIndex);
        allFile.close();
    }
    
    Serial.println("-> logged: " + dailyPath);
}
