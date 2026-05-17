#include <WiFi.h>             
#include <PubSubClient.h>   
#include "secrets.h"

int ledPins[] = {4,5,6,7,15};
int Leuchtdauer;
int i;

WiFiClient espClient;
PubSubClient client(espClient);

// Forward Declarations
void mqttCallback(char* topic, byte* message, unsigned int length);
void reconnect();
void alleAus();
void oneAfterAnotherNoLoop();
void oneAfterAnotherLoop();

void setup() {
  Serial.begin(115200);

  for (i = 0; i <= 4; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }

  // WLAN verbinden
  WiFi.begin(ssid, password);
  Serial.print("Verbinde mit WLAN");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" verbunden!");
  Serial.println(WiFi.localIP());

  // Broker verbinden
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);
}

void loop() {
  // MQTT Verbindung aufrechterhalten
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Verbinde mit MQTT...");
    if (client.connect("ESP32Lauflicht", mqtt_user, mqtt_pw)) {
      Serial.println("verbunden!");
      client.subscribe("lauflicht/steuerung");
    } else {
      Serial.print("Fehler: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void mqttCallback(char* topic, byte* message, unsigned int length) {
  String msg = "";
  for (int j = 0; j < length; j++) {
    msg += (char)message[j];
  }
  Serial.println("Nachricht empfangen: " + msg);

  if (msg == "noloop")  oneAfterAnotherNoLoop();
  if (msg == "loop")    oneAfterAnotherLoop();
  if (msg == "aus")     alleAus();
}

void alleAus() {
  for (i = 0; i <= 4; i++) {
    digitalWrite(ledPins[i], LOW);
  }
}

void oneAfterAnotherNoLoop() {
  Leuchtdauer = 500;
  for (i = 0; i <= 4; i++) {
    digitalWrite(ledPins[i], HIGH);
    delay(Leuchtdauer);
    digitalWrite(ledPins[i], LOW);
  }
}

void oneAfterAnotherLoop() {
  int delayTime = 500;
  for (i = 0; i <= 4; i++) {
    digitalWrite(ledPins[i], HIGH);
    delay(delayTime);
  }
  for (i = 4; i >= 0; i--) {
    digitalWrite(ledPins[i], LOW);
    delay(delayTime);
  }
}
