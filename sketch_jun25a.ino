#include <WiFiS3.h>
#include <PubSubClient.h>
#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT11

#define RELAY_PIN 7
#define BUZZER_PIN 8

DHT dht(DHTPIN, DHTTYPE);

// =======================
// WIFI
// =======================
char ssid[] = "HANS";
char pass[] = "hans2903";

// =======================
// MQTT (PUBLIC BROKER)
// =======================
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;

// =======================
// CLIENT
// =======================
WiFiClient wifiClient;
PubSubClient client(wifiClient);

// =======================
// WIFI CONNECT
// =======================
void connectWiFi() {
  Serial.println();
  Serial.print("Menghubungkan WiFi");

  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    Serial.print(".");
    delay(2000);
  }

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// =======================
// MQTT CONNECT
// =======================
void connectMQTT() {
  while (!client.connected()) {

    Serial.println();
    Serial.println("Mencoba koneksi MQTT...");

    // TANPA USERNAME / PASSWORD (public broker)
    if (client.connect("ArduinoInkubator")) {
      Serial.println("MQTT Connected");
    } else {
      Serial.print("Gagal, rc=");
      Serial.println(client.state());
      delay(3000);
    }
  }
}

// =======================
// SETUP
// =======================
void setup() {
  Serial.begin(9600);

  dht.begin();

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  connectWiFi();

  client.setServer(mqtt_server, mqtt_port);
}

// =======================
// LOOP
// =======================
void loop() {

  if (!client.connected()) {
    connectMQTT();
  }

  client.loop();

  float suhu = dht.readTemperature();

  if (isnan(suhu)) {
    Serial.println("Sensor gagal dibaca");
    delay(2000);
    return;
  }

  Serial.println("----------------");
  Serial.print("Suhu: ");
  Serial.print(suhu);
  Serial.println(" C");

  // =======================
// LOGIKA RELAY & BUZZER
// =======================

bool relayStatus = false;
bool buzzerStatus = false;

// Suhu normal atau masih kurang
if (suhu <= 38.5) {

  // Lampu menyala
  digitalWrite(RELAY_PIN, HIGH);

  // Buzzer mati
  digitalWrite(BUZZER_PIN, LOW);

  relayStatus = true;
  buzzerStatus = false;

  Serial.println("Lampu Pemanas ON");
  Serial.println("Buzzer OFF");
}

// Suhu terlalu tinggi
else {

  // Lampu mati
  digitalWrite(RELAY_PIN, LOW);

  relayStatus = false;

  Serial.println("Lampu Pemanas OFF");
  Serial.println("Buzzer ON 3 Detik");

  // Buzzer bunyi 3 detik
  digitalWrite(BUZZER_PIN, HIGH);
  buzzerStatus = true;

  delay(3000);

  digitalWrite(BUZZER_PIN, LOW);
}

// =======================
// JSON PAYLOAD MQTT
// =======================

String payload = "{";
payload += "\"suhu\":" + String(suhu, 1) + ",";
payload += "\"relay\":" + String(relayStatus ? 1 : 0) + ",";
payload += "\"buzzer\":" + String(buzzerStatus ? 1 : 0);
payload += "}";

client.publish("inkubator/suhu", payload.c_str());

Serial.println(payload);

  delay(5000);
}