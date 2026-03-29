// web.cpp

#include "globals.h"        
#include <SD.h>

// --------------------------------------------------
// HTTP-Handler
// --------------------------------------------------

// Zeigt die Hauptseite mit aktuellen Messwerten und Ampelstatus im Browser
void handleRoot() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  server.sendContent("<!DOCTYPE html><html><head><meta charset='utf-8'><title>Luftqualitaet</title></head><body>");
  server.sendContent("<h1>Luftqualitaet</h1>");

  // Messwerte als HTML ausgeben
  server.sendContent("<p><b>CO2:</b> " + String(co2_ppm) + " ppm</p>");
  server.sendContent("<p><b>T SCD:</b> " + String(scd_temp, 1) + " &deg;C</p>");
  server.sendContent("<p><b>rF SCD:</b> " + String(scd_rh, 1) + " %</p>");
  server.sendContent("<p><b>T BME:</b> " + String(bme_comp_temp, 1) + " &deg;C</p>");
  server.sendContent("<p><b>rF BME:</b> " + String(bme_comp_hum, 1) + " %</p>");
  server.sendContent("<p><b>PM1/2.5/10:</b> " + String(pm1_atm) + " / "
                     + String(pm25_atm) + " / " + String(pm10_atm) + " &mu;g/m³</p>");

  // Warnung wenn RTC noch nicht gesetzt
  if (!rtcIsSet) {
    server.sendContent("<p style='color:red;'><b>Hinweis:</b> RTC-Zeit noch nicht gesetzt. "
                       "Bitte <a href='/settime'>hier klicken</a> und zuerst die Zeit einstellen. "
                       "Bis dahin wird nicht auf SD geloggt.</p>");
  }

  // AQI-Berechnung erfolgt extern im Dashboard
  server.sendContent("<p><b>AQI:</b> --- (Berechnung erfolgt in GUI)</p>");

  // Links zu anderen Seiten
  server.sendContent("<p><a href='/log'>Log ansehen</a></p>");
  server.sendContent("<p><a href='/download'>Vollständiges Log herunterladen</a></p>");
  server.sendContent("<p><a href='/settime'>Uhrzeit setzen</a></p>");
  server.sendContent("<p><a href='/info'>Info / Sensoren &amp; Skalen</a></p>");

  server.sendContent("</body></html>");
  server.client().stop();
}

// Zeigt Infoseite mit Sensor-Beschreibungen und Bewertungsskalen
void handleInfo() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  server.sendContent(
    "<!DOCTYPE html><html><head><meta charset='utf-8'>"
    "<title>Info</title>"
    "<style>"
    "body{font-family:sans-serif;font-size:14px;}"
    "h2{margin-top:1.2em;}"
    "table{border-collapse:collapse;font-size:13px;}"
    "th,td{border:1px solid #ccc;padding:2px 4px;}"
    "</style>"
    "</head><body><h1>Infos zu Sensoren &amp; Skalen</h1>"
  );

  // PMS5003 Sensor-Info
  server.sendContent(
    "<h2>PMS5003 Feinstaubsensor</h2>"
    "<p>Laserstreuungs-Sensor f&uuml;r Feinstaub (PM1.0, PM2.5, PM10) in &micro;g/m&sup3;.</p>"
    "<ul>"
    "<li>PM1.0: Partikel &lt; 1&nbsp;&micro;m (Rauch, Ru&szlig;).</li>"
    "<li>PM2.5: Partikel &lt;= 2,5&nbsp;&micro;m, dringen tief in die Lunge ein.</li>"
    "<li>PM10: Partikel &lt;= 10&nbsp;&micro;m (Staub, Pollen).</li>"
    "</ul>"
  );

  // SCD41 Sensor-Info
  server.sendContent(
  "<h2>SCD41 CO2-Sensor</h2>"
  "<p>NDIR-CO2-Sensor f&uuml;r Innenr&auml;ume, misst CO2 in ppm sowie Temperatur und rF.</p>"
  "<p>Der CO2-Wert wird optisch mit einem photoakustischen NDIR-Verfahren bestimmt: "
  "Infrarotlicht regt das CO2-Molekuel an, das dabei Schall erzeugt, der mit einem Mikrophon gemessen wird.</p>"
  "<p>Ein interner Temperatur- und Feuchtesensor wird zur Kompensation des CO2-Signals genutzt; "
  "die Werte stehen gleichzeitig als Raumtemperatur und rF zur Verfuegung.</p>"
  "<ul><li>Messbereich typ. ca. 400&ndash;5000&nbsp;ppm.</li>"
  "<li>CO2 &lt; 800&nbsp;ppm: sehr gute Luftqualit&auml;t.</li>"
  "<li>&gt; 1000&nbsp;ppm: stickig, L&uuml;ften empfohlen.</li></ul>"
  );

  // BME680 Sensor-Info
  server.sendContent(
  "<h2>BME680 Umweltsensor</h2>"
  "<p>Kombiniert Temperatur, Luftfeuchte, Luftdruck und Gas-Sensor f&uuml;r einen VOC-Index.</p>"
  "<p>Temperatur, rF und Druck werden mit integrierten Halbleiter-Sensoren gemessen; "
  "ein Metalloxid-Gassensor (MOX) wird kurz aufgeheizt und wertet die Widerstands&auml;nderung "
  "durch fluechtige organische Verbindungen (VOC) aus.</p>"
  "<p>Durch Eigenerwaermung auf der Platine kann die angezeigte Temperatur leicht zu hoch "
  "und die relative Feuchte etwas zu niedrig sein (Offset-Korrektur ggf. sinnvoll).</p>"
  "<ul>"
  "<li>Komfortbereich T: ca. 20&ndash;24&nbsp;&deg;C.</li>"
  "<li>Komfortbereich rF: ca. 40&ndash;60&nbsp;%.</li>"
  "<li>VOC-Index um 100: normale Hintergrundbelastung.</li>"
  "</ul>"
  );

  // Bewertungsskalen (alte Berechnung, wird später überarbeitet)
  server.sendContent(
    "<h2>Bewertungsskalen (1&ndash;5)</h2>"
    "<p>1 = sehr gut, 5 = sehr schlecht.</p>"

    "<h3>PM1</h3>"
    "<table><tr><th>Score</th><th>PM1 [&micro;g/m&sup3;]</th></tr>"
    "<tr><td>1</td><td>0&ndash;5</td></tr>"
    "<tr><td>2</td><td>&gt;5&ndash;10</td></tr>"
    "<tr><td>3</td><td>&gt;10&ndash;20</td></tr>"
    "<tr><td>4</td><td>&gt;20&ndash;35</td></tr>"
    "<tr><td>5</td><td>&gt;35</td></tr></table>"

    "<h3>PM2.5</h3>"
    "<table><tr><th>Score</th><th>PM2.5 [&micro;g/m&sup3;]</th></tr>"
    "<tr><td>1</td><td>0&ndash;10</td></tr>"
    "<tr><td>2</td><td>&gt;10&ndash;20</td></tr>"
    "<tr><td>3</td><td>&gt;20&ndash;35</td></tr>"
    "<tr><td>4</td><td>&gt;35&ndash;75</td></tr>"
    "<tr><td>5</td><td>&gt;75</td></tr></table>"

    "<h3>PM10</h3>"
    "<table><tr><th>Score</th><th>PM10 [&micro;g/m&sup3;]</th></tr>"
    "<tr><td>1</td><td>0&ndash;20</td></tr>"
    "<tr><td>2</td><td>&gt;20&ndash;40</td></tr>"
    "<tr><td>3</td><td>&gt;40&ndash;60</td></tr>"
    "<tr><td>4</td><td>&gt;60&ndash;100</td></tr>"
    "<tr><td>5</td><td>&gt;100</td></tr></table>"

    "<h3>CO2</h3>"
    "<table><tr><th>Score</th><th>CO2 [ppm]</th></tr>"
    "<tr><td>1</td><td>400&ndash;800</td></tr>"
    "<tr><td>2</td><td>&gt;800&ndash;1000</td></tr>"
    "<tr><td>3</td><td>&gt;1000&ndash;1400</td></tr>"
    "<tr><td>4</td><td>&gt;1400&ndash;2000</td></tr>"
    "<tr><td>5</td><td>&gt;2000</td></tr></table>"

    "<h3>IAQ-Index (Bosch BSEC)</h3>"
    "<table><tr><th>IAQ</th><th>Luftqualitaet</th></tr>"
    "<tr><td>0&ndash;50</td><td>Excellent</td></tr>"
    "<tr><td>51&ndash;100</td><td>Good</td></tr>"
    "<tr><td>101&ndash;150</td><td>Lightly polluted</td></tr>"
    "<tr><td>151&ndash;200</td><td>Moderately polluted</td></tr>"
    "<tr><td>201&ndash;250</td><td>Heavily polluted</td></tr>"
    "<tr><td>251&ndash;350</td><td>Severely polluted</td></tr>"
    "<tr><td>&gt;351</td><td>Extremely polluted</td></tr>"
    "</table>"

    "<p><a href='/'>Zur&uuml;ck</a></p>"
    "</body></html>"
  );

  server.client().stop();
}

// Zeigt CSV-Log als HTML-Tabelle (sortierbar per Klick auf Spaltenüberschrift)
void handleLogTable() {
  File f = SD.open("/logs/log_complete.csv", FILE_READ);
  if (!f) {
    server.send(500, "text/plain", "Konnte log_complete.csv nicht oeffnen");
    return;
  }

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  // HTML-Kopf mit Tabellen-Styling
  server.sendContent(
      "<!DOCTYPE html><html><head><meta charset='utf-8'>"
      "<title>Log</title>"
      "<style>"
      "table{border-collapse:collapse;font-family:monospace;font-size:11px;overflow-x:auto;display:block;}"
      "th,td{border:1px solid #ccc;padding:2px 4px;white-space:nowrap;}"
      "th{background:#eee;position:sticky;top:0;cursor:pointer;}"
      "tbody tr:nth-child(odd){background:#f9f9f9;}"
      "</style>"
      "</head><body><h1>Log</h1>"
      "<table id='logtable'><thead><tr>"
      "<th onclick='sortTable(0,true)'>#</th>"
      "<th onclick='sortTable(1,false)'>Date</th>"
      "<th onclick='sortTable(2,false)'>Time</th>"
      "<th onclick='sortTable(3,true)'>PM1</th>"
      "<th onclick='sortTable(4,true)'>PM2.5</th>"
      "<th onclick='sortTable(5,true)'>PM10</th>"
      "<th onclick='sortTable(6,true)'>PM1_CF1</th>"
      "<th onclick='sortTable(7,true)'>PM2.5_CF1</th>"
      "<th onclick='sortTable(8,true)'>PM10_CF1</th>"
      "<th onclick='sortTable(9,true)'>nPM_0.3</th>"
      "<th onclick='sortTable(10,true)'>nPM_0.5</th>"
      "<th onclick='sortTable(11,true)'>nPM_1.0</th>"
      "<th onclick='sortTable(12,true)'>nPM_2.5</th>"
      "<th onclick='sortTable(13,true)'>nPM_5.0</th>"
      "<th onclick='sortTable(14,true)'>nPM_10</th>"
      "<th onclick='sortTable(15,true)'>CO2</th>"
      "<th onclick='sortTable(16,true)'>T_SCD</th>"
      "<th onclick='sortTable(17,true)'>RH_SCD</th>"
      "<th onclick='sortTable(18,true)'>IAQ</th>"
      "<th onclick='sortTable(19,true)'>IAQ_Acc</th>"
      "<th onclick='sortTable(20,true)'>Static_IAQ</th>"
      "<th onclick='sortTable(21,true)'>CO2_Eq</th>"
      "<th onclick='sortTable(22,true)'>VOC_Eq</th>"
      "<th onclick='sortTable(23,true)'>T_Raw</th>"
      "<th onclick='sortTable(24,true)'>Press</th>"
      "<th onclick='sortTable(25,true)'>RH_Raw</th>"
      "<th onclick='sortTable(26,true)'>Gas_R</th>"
      "<th onclick='sortTable(27,true)'>T_Comp</th>"
      "<th onclick='sortTable(28,true)'>RH_Comp</th>"
      "<th onclick='sortTable(29,true)'>Gas_%</th>"
      "<th onclick='sortTable(30,true)'>Stab</th>"
      "<th onclick='sortTable(31,true)'>RunIn</th>"
      "</tr></thead><tbody>"
    );

  // CSV Zeile für Zeile lesen und als HTML-Tabelle ausgeben
  String line;
  bool firstLine = true;

  while (f.available()) {
    char c = f.read();
    if (c == '\n') {
      if (line.length() > 0) {
        if (firstLine) {
          firstLine = false;          // CSV-Header überspringen
        } else {
          // Semikolon durch Tabellenzellen ersetzen
          String row = "<tr><td>";
          for (uint16_t i = 0; i < line.length(); i++) {
            if (line[i] == ';') row += "</td><td>";
            else                row += line[i];
          }
          row += "</td></tr>";
          server.sendContent(row);
        }
      }
      line = "";
    } else if (c != '\r') {
      line += c;
    }
  }
  f.close();

  // JavaScript für Tabellen-Sortierung
  server.sendContent(
    "</tbody></table>"
    "<p><a href='/'>Zur&uuml;ck</a></p>"
    "<script>"
    "function sortTable(col, numeric){"
      "const table=document.getElementById('logtable');"
      "const tbody=table.tBodies[0];"
      "const rows=Array.from(tbody.rows);"
      "const sortCol=table.getAttribute('data-sort-col');"
      "const sortDir=table.getAttribute('data-sort-dir');"
      "let asc=true;"
      "if(sortCol==col){asc=(sortDir!=='asc');}"
      "rows.sort(function(a,b){"
        "let x=a.cells[col].textContent;"
        "let y=b.cells[col].textContent;"
        "if(numeric){x=parseFloat(x.replace(',','.'))||0;y=parseFloat(y.replace(',','.'))||0;return asc?x-y:y-x;}"
        "x=x.toLowerCase();y=y.toLowerCase();"
        "if(x<y)return asc?-1:1;"
        "if(x>y)return asc?1:-1;"
        "return 0;"
      "});"
      "rows.forEach(r=>tbody.appendChild(r));"
      "table.setAttribute('data-sort-col',col);"
      "table.setAttribute('data-sort-dir',asc?'asc':'desc');"
    "}"
    "</script>"
    "</body></html>"
  );
  server.client().stop();
}

// Bietet CSV-Datei als Download an
void handleLogDownload() {
  File f = SD.open("/logs/log_complete.csv", FILE_READ);
  if (!f) {
    server.send(500, "text/plain", "Konnte log_complete.csv nicht oeffnen");
    return;
  }
  
  // Komplette Datei in String einlesen
  String all = "";
  while (f.available()) all += char(f.read());
  f.close();

  // Als CSV-Download senden
  server.sendHeader("Content-Type", "text/csv; charset=utf-8");
  server.sendHeader("Content-Disposition","attachment; filename=\"log_complete.csv\"");
  server.send(200, "text/csv", all);
}

// Zeigt Formular zum Setzen der RTC-Zeit
void handleSetTimeForm() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  // Aktuelle RTC-Zeit als Vorauswahl im Formular
  DateTime now = rtc.now();
  char buf[32];
  snprintf(buf,sizeof(buf),"%04d-%02d-%02dT%02d:%02d",
           now.year(),now.month(),now.day(),now.hour(),now.minute());

  server.sendContent(
    "<!DOCTYPE html><html><head><meta charset='utf-8'>"
    "<title>Zeit setzen</title></head><body>"
    "<h1>Zeit der RTC setzen</h1>"
    "<form action='/settime_post' method='POST'>"
    "<label for='dt'>Datum und Uhrzeit (PC-Zeit):</label><br>"
    "<input type='datetime-local' id='dt' name='dt' value='"
  );
  server.sendContent(buf);
  server.sendContent(
    "' step='1'><br><br>"
    "<input type='submit' value='RTC setzen'>"
    "</form>"
    "<p><a href='/'>Zur&uuml;ck</a></p>"
    "</body></html>"
  );
  server.client().stop();
}

// Verarbeitet POST-Request zum Setzen der RTC-Zeit
void handleSetTimePost() {
  if (!server.hasArg("dt")) {
    server.send(400, "text/plain", "Parameter 'dt' fehlt");
    return;
  }

  // Zeit aus Formular parsen (Format: YYYY-MM-DDTHH:MM:SS)
  String dt = server.arg("dt");
  Serial.print("Empfangene PC-Zeit: ");
  Serial.println(dt);

  int year  = dt.substring(0, 4).toInt();
  int month = dt.substring(5, 7).toInt();
  int day   = dt.substring(8, 10).toInt();
  int hour  = dt.substring(11,13).toInt();
  int min   = dt.substring(14,16).toInt();
  int sec   = 0;
  if (dt.length() >= 19 && dt[16]==':')
    sec = dt.substring(17,19).toInt();

  // RTC mit neuer Zeit einstellen
  rtc.adjust(DateTime(year,month,day,hour,min,sec));
  rtcIsSet        = true;
  lastLogUnixValid= false;

  // Bestätigungsseite anzeigen mit automatischem Redirect
  server.sendHeader("Location", "/");
  server.send(303, "text/plain", "RTC gesetzt, leite weiter...");
}

// API für alle Sensordaten (für Dashboard + Kalibrierung/ Diagnose)
void handleSensorData() {

  DateTime now = rtc.now();
  char timestamp[20];
  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02d",
           now.year(), now.month(), now.day(), 
           now.hour(), now.minute(), now.second());
  
  String json = "{";
    
    json += "\"timestamp\":\"" + String(timestamp) + "\",";

    json += "\"bme680\":{";
      json += "\"timestamp_ms\":" + String((unsigned long)millis()) + ",";
      json += "\"iaq\":" + String(bme_iaq) + ",";
      json += "\"iaq_accuracy\":" + String(bme_iaq_accuracy) + ",";
      json += "\"static_iaq\":" + String(bme_static_iaq) + ",";
      json += "\"co2_eq\":" + String(bme_co2_eq) + ",";
      json += "\"voc_eq\":" + String(bme_voc_eq) + ",";
      json += "\"temp_raw\":" + String(bme_temp_raw, 1) + ",";
      json += "\"press_hPa\":" + String(bme_press_hPa, 1) + ",";
      json += "\"hum_raw\":" + String(bme_hum_raw, 1) + ",";
      json += "\"gas_resistance\":" + String(bme_gas_resistance) + ",";
      json += "\"stabilization_status\":" + String(bme_stabilization_status) + ",";
      json += "\"run_in_status\":" + String(bme_run_in_status) + ",";
      json += "\"comp_temp\":" + String(bme_comp_temp, 1) + ",";
      json += "\"comp_hum\":" + String(bme_comp_hum, 1) + ",";
      json += "\"gas_percentage\":" + String(bme_gas_percentage, 1);
    json += "}";  

    json += ",\"scd41\":{";
      json += "\"co2\":" + String(co2_ppm) + ",";
      json += "\"temp\":" + String(scd_temp, 1) + ",";
      json += "\"rh\":" + String(scd_rh, 1);
    json += "}";  

    json += ",\"pms5003\":{";
      json += "\"pm1_atm\":" + String(pm1_atm) + ",";
      json += "\"pm25_atm\":" + String(pm25_atm) + ",";
      json += "\"pm10_atm\":" + String(pm10_atm) + ",";
      json += "\"pm1_cf1\":" + String(pm1_cf1) + ",";
      json += "\"pm25_cf1\":" + String(pm25_cf1) + ",";
      json += "\"pm10_cf1\":" + String(pm10_cf1) + ",";
      json += "\"pm_n03\":" + String(pm_n03) + ",";
      json += "\"pm_n05\":" + String(pm_n05) + ",";
      json += "\"pm_n10\":" + String(pm_n10) + ",";
      json += "\"pm_n25\":" + String(pm_n25) + ",";
      json += "\"pm_n50\":" + String(pm_n50) + ",";
      json += "\"pm_n100\":" + String(pm_n100);
    json += "}";  

  json += "}";  
  
  // CORS-Header für Zugriff von externem Dashboard
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}
