#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED config
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// WiFi Config
const char* ssid = "ajus";
const char* password = "makasihbang";

// MQTT Broker
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "sensor/mq7/CO";

WiFiClient espClient;
PubSubClient client(espClient);

const int mq7Pin = 34;

float Ro = 10.0;
unsigned long startTime;

float getADCValue() {
  int total = 0;
  for (int i = 0; i < 50; i++) {
    total += analogRead(mq7Pin);
    delay(10);
  }
  return total / 50.0;
}

float calibrate() {
  Serial.println("Kalibrasi... Pastikan udara bersih (tanpa asap/gas CO)");
  delay(5000);
  float adcValue = getADCValue();
  float Ro = adcValue / 9.83;
  Serial.print("Kalibrasi selesai. Ro = ");
  Serial.println(Ro);
  return Ro;
}

float readRatio(float Ro) {
  float adcValue = getADCValue();
  float Rs = adcValue;
  return Rs / Ro;
}

String getAirQuality(float ratio) {
  if (ratio >= 0 && ratio <= 10) {
    return "Sangat Bersih";
  } else if (ratio <= 20) {
    return "Aman";
  } else if (ratio <= 30) {
    return "Waspada";
  } else if (ratio <= 35) {
    return "Berbahaya";
  } else {
    return "Kritis";
  }
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32ClientMQ7")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(10);

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED gagal ditemukan"));
    while (true); // Hentikan program
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Inisialisasi...");
  display.display();

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  Ro = calibrate();
  startTime = millis();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float ratio = readRatio(Ro);
  String kualitas = getAirQuality(ratio);

  Serial.print("Rs/Ro = ");
  Serial.print(ratio, 2);
  Serial.print(" --> ");
  Serial.println(kualitas);

  // Tampilan OLED diperbesar dan dirapikan
  display.clearDisplay();

  // Rs/Ro besar
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("Rs/Ro:");

  display.setCursor(0, 20);
  display.println(ratio, 2);

  // Kualitas sedang
  display.setTextSize(1);
  display.setCursor(0, 45);
  display.print("Kualitas: ");
  display.print(kualitas);

  display.display();

  // Kirim data via MQTT
  String payload = "{";
  payload += "\"RsRo\": ";
  payload += String(ratio, 2);
  payload += ", \"kualitas\": \"" + kualitas + "\"}";
  client.publish(mqtt_topic, payload.c_str());

  delay(2000);
}
