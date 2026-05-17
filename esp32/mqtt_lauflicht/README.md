Dieses Projekt ist ein Lernbeispiel aus dem Unterricht (Cyberphysische Systeme, Fachinformatiker Anwendungsentwicklung) 
und dient ausschließlich zu Bildungszwecken.

# ESP32 LED Lauflicht via MQTT

Steuerung eines LED-Lauflichts per MQTT Publish/Subscribe. 
Der ESP32 abonniert ein Topic und reagiert auf eingehende Nachrichten mit verschiedenen LED-Animationen.

---

## Hardware

- ESP32-S3 (oder kompatibler Controller mit WLAN)
- 5x LED (beliebige Farbe)
- 5x Vorwiderstand (je nach LED-Typ)
- Breadboard + Jumper-Kabel

### Verdrahtung

Jede LED parallel geschaltet mit eigenem Vorwiderstand und eigenem GPIO-Pin. 
Widerstand und LED sitzen in Reihe im jeweiligen Zweig zwischen GPIO-Pin und GND.

| LED | ESP32 Pin |
|-----|-----------|
| LED 1 | GPIO 4 |
| LED 2 | GPIO 5 |
| LED 3 | GPIO 6 |
| LED 4 | GPIO 7 |
| LED 5 | GPIO 15 |

---

## Architektur

```
MQTT Client (PC / Mobilgeräte)
        |
        | MQTT Publish
        v
MQTT Broker (Mosquitto)
        |
        | MQTT Subscribe
        v
ESP32 --> LED 1..5
```

---

## Installation

### 1. MQTT Broker (Mosquitto)

> ℹ️ Dieser Abschnitt beschreibt die lokale Installation von Mosquitto.
> Bei Verwendung eines öffentlichen Brokers (z.B. broker.hivemq.com) entfallen
> die Schritte 1.1 bis 1.3. In `secrets.h` dann den jeweiligen Broker eintragen
> und im Code die Authentifizierung deaktivieren.

Download und Installation: [mosquitto.org](https://mosquitto.org)

#### 1.1 Config anpassen

Nach der Installation startet Mosquitto im local-only Modus - externe Verbindungen
wie die vom Esp32 werden verweigert. In `mosquitto.conf` am Ende hinzufügen:

```
listener 1883
```

#### 1.2 Benutzerkonto anlegen (optional)

```
mosquitto_passwd -c "C:\Program Files\mosquitto\passwd" DeinBenutzername
```

Passwort eingeben und bestätigen (2x).

In `mosquitto.conf` am Ende hinzufügen:

```
allow_anonymous false
password_file C:\Program Files\mosquitto\passwd
```

(default: allow_anonymous true)

#### 1.3 Mosquitto starten

> ⚠️ Bekanntes Problem unter Windows: Der Windows-Dienst ignoriert die `mosquitto.conf`
> und startet trotzdem im local-only Modus. Daher manuell in CMD starten:

```
"C:\Program Files\mosquitto\mosquitto.exe" -v -c "C:\Program Files\mosquitto\mosquitto.conf"
```

CMD-Fenster offen lassen - Mosquitto läuft solange das Fenster aktiv ist.

Erfolgreich wenn im Log erscheint:
```
Config loaded from ...mosquitto.conf
mosquitto version x.x.x running
```

Kein "Starting in local only mode" darf erscheinen.

---

### 2. MQTT Clients einrichten

#### MQTT Explorer (Desktop)

Download: [mqtt-explorer.com](https://mqtt-explorer.com)

Verbindung einrichten:
- Host: IP des Brokers
- Port: `1883`
- Username / Password eintragen
- **TLS deaktivieren**
- Connect

#### MQTT Panel (Android)

Im Play Store verfügbar.

Verbindung einrichten:
- Broker: IP des Brokers
- Port: `1883`
- Username / Password eintragen
- Button-Widget anlegen mit Topic `lauflicht/steuerung` und gewünschtem Payload (hier: noloop/ loop/ aus)

---

### 3. Arduino IDE einrichten

- Boardverwalter-URL hinzufügen unter Datei -> Einstellungen:
  `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
- Boardverwalter: **esp32 von Espressif Systems** installieren
- Board: **ESP32S3 Dev Module**
- USB CDC on Boot: **Enabled**
- Bibliothek: **PubSubClient** von Nick O'Leary (v2.8.0)

---

### 4. Code anpassen und flashen

`secrets.example.h` kopieren, umbenennen in `secrets.h` und eigene Werte eintragen:

```cpp
// WLAN
const char* ssid        = "DeineSSID";
const char* password    = "DeinPasswort";

// Mosquitto Broker (Standard-Port: 1883)
const char* mqtt_server = "x.x.x.x";
const char* mqtt_user   = "DeinBenutzername";
const char* mqtt_pw     = "DeinPasswort";
```

> ⚠️ `secrets.h` ist in `.gitignore` eingetragen und wird nicht ins Repository hochgeladen.

Sketch in Arduino IDE öffnen, Board und Port auswählen, Serial Monitor schließen, hochladen.

Nach dem Flashen Serial Monitor öffnen (115200 Baud). Erfolgreich wenn erscheint:

```
Verbunden!
10.x.x.x
```

---

## Verwendung

ESP32 verbindet sich beim Start automatisch mit WLAN und MQTT-Broker und abonniert:

```
lauflicht/steuerung
```

### Verfügbare Nachrichten

| Nachricht | Funktion |
|-----------|----------|
| `noloop`  | Lauflicht einmal komplett durchlaufen |
| `loop`    | Alle LEDs nacheinander an, dann nacheinander aus |
| `aus`     | Alle LEDs sofort aus |

### Nachricht senden

**Per Kommandozeile:**
```
mosquitto_pub -h x.x.x.x -p 1883 -u Benutzername -P Passwort -t "lauflicht/steuerung" -m "noloop"
```

**Per MQTT Explorer:** Topic `lauflicht/steuerung` im Publish-Bereich eintragen, Nachricht eingeben, Publish klicken.

**Per MQTT Panel:** Button-Widget antippen.

---

## Bekannte Probleme

| Problem | Ursache | Lösung |
|---------|---------|--------|
| COM-Port belegt beim Flashen | Serial Monitor offen | Serial Monitor schließen |
| MQTT Fehler -2 | Broker läuft nicht oder falsche IP | Mosquitto starten, IP prüfen |
| MQTT Fehler -4 | Broker erreichbar aber Config fehlt | Manuell mit `-c` Config-Pfad starten |
| Mosquitto local only | Dienst ignoriert Config | Manuell in CMD starten (siehe 1.3) |
| Kein Serial Output | USB CDC on Boot deaktiviert | In Arduino IDE auf Enabled setzen |

---
