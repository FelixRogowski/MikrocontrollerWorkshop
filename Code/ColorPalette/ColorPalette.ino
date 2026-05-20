#include <WiFi.h>
#include <ArduinoMqttClient.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 5
#define LED_COUNT 200

// WLAN-Zugangsdaten
const char ssid[] = "rise-iot";      // Netzwerkname
const char pass[] = "risegericht"; // Passwort



WiFiClient wifiClient;
MqttClient mqtt(wifiClient);

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);

void onMessage(int messageSize);

void setup() {
  Serial.begin(115200);
  strip.begin();

  // LEDs zu Beginn weiß setzen
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(255, 255, 255));
  }
  strip.show();

  // Netzwerkscan
  Serial.println("Scanning for WiFi networks...");
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("No networks found!");
  } else {
    bool found = false;
    for (int i = 0; i < n; i++) {
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.println(" dBm)");

      if (WiFi.SSID(i) == ssid) {
        found = true;
      }
    }

    if (!found) {
      Serial.println("rise-iot network not found! Cannot connect.");
      return;
    }
  }

  // Statische IP konfigurieren und verbinden
  WiFi.config(localIP, gateway, subnet);
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempts++;
    if (attempts > 40) {  // 20 Sekunden max
      Serial.println("\nFailed to connect to WiFi");
      return;
    }
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // MQTT verbinden
  mqtt.setId("Arduino1");
  while (!mqtt.connect("192.168.100.132", 1883)) { // IP deines Pi
    delay(1000);
    Serial.println("MQTT connecting...");
  }
  Serial.println("MQTT connected!");

  mqtt.onMessage(onMessage);
  mqtt.subscribe("led/strip/color");
}

void loop() {
  mqtt.poll();
}

void onMessage(int messageSize) {
  String msg = mqtt.readString();
  Serial.print("Received: ");
  Serial.println(msg);

  int r, g, b;
  if (sscanf(msg.c_str(), "%d,%d,%d", &r, &g, &b) == 3) {
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(r, g, b));
    }
    strip.show();
  } else {
    Serial.println("Invalid message format");
  }
}
