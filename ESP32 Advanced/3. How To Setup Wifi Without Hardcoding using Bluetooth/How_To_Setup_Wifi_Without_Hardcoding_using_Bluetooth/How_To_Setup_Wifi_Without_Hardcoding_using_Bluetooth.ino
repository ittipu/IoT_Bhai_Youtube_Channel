#include <ArduinoJson.h>
#include <EEPROM.h>
#include <BluetoothSerial.h>
#include <PubSubClient.h>
#include <WiFi.h>


#define DEVICE_ID "Test001"
const char* ntpServer = "pool.ntp.org";
String ssid = "";
String password = "";

const int eepromSize = 100;
const int ssidAddr = 0;
const int passAddr = 50;


#define MQTT_BROKER "broker.hivemq.com"
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""
#define MQTT_PORT 1883
#define WIFI_TIMEOUT 60 
#define BLE_WAITING_TIMEOUT 30 // seconds

#define DHTPIN 5
#define DHTTYPE DHT22

#define LED 2

#define INTERVAL_MAIN_LOOP 5000
#define TOPIC_PUB "test/01"

WiFiClient espClient;
PubSubClient client(espClient);
BluetoothSerial SerialBT;
StaticJsonDocument<256> doc;

unsigned long lastReadingTime = 0;
const unsigned long readingInterval = INTERVAL_MAIN_LOOP;
unsigned long epochTime = 0;

void mqtt_reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(DEVICE_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
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
  EEPROM.begin(eepromSize);
  pinMode(LED, OUTPUT);

  show_saved_data();
  setup_ble();
  setup_wifi();
  setup_mqtt();
  configTime(0, 0, ntpServer);
}

void loop() {
  if (!client.connected()) {
    mqtt_reconnect();
  }
  unsigned long currentMillis = millis();
  if (currentMillis - lastReadingTime >= readingInterval) {
    lastReadingTime = currentMillis;
    char buffer[256];
    epochTime = getTime();
    doc["id"] = DEVICE_ID;
    doc["timestamp"] = epochTime;

    Serial.println("Getting Sensor Data - ");
    serializeJson(doc, Serial);
    Serial.println();
    Serial.println("Published Successfully");
    serializeJson(doc, buffer);
    client.publish(TOPIC_PUB, buffer);
    Serial.println();
    digitalWrite(LED, HIGH);
    delay(1000);
    digitalWrite(LED, LOW);
  }
}

void show_saved_data() {
  Serial.println("Getting Saved Data: ");
  ssid = EEPROM.readString(ssidAddr);
  password = EEPROM.readString(passAddr);
  Serial.println("SSID: " + ssid);
  Serial.println("Password: " + password);
}

void setup_ble() {
  SerialBT.begin(DEVICE_ID);
  Serial.println("Waiting for Bluetooth connection...");

  unsigned long startTime = millis();
  bool btConnected = false;

  while (millis() - startTime < BLE_WAITING_TIMEOUT*1000) {
    if (SerialBT.hasClient()) {
      btConnected = true;
      break;
    }
    if ((millis() / 500) % 2 == 0) {
      digitalWrite(LED, HIGH);
    } else {
      digitalWrite(LED, LOW);
    }
  }

  if (btConnected) {
    Serial.println("Bluetooth connected. Ready to receive credentials.");
    SerialBT.println("Enter SSID");
    while (!SerialBT.available());  // Wait for input
    String newSSID = SerialBT.readStringUntil('\n');
    newSSID.trim();
    Serial.println("Received SSID via Bluetooth: " + newSSID);
    if (newSSID.length() > 0) {
      ssid = newSSID;
      SerialBT.println("SSID saved: " + ssid);
    } else {
      SerialBT.println("SSID not changed.");
      Serial.println("No new SSID provided, using stored SSID: " + ssid);
    }
    delay(500);

    SerialBT.println("Enter password");
    while (!SerialBT.available());  // Wait for input
    String newPassword = SerialBT.readStringUntil('\n');
    newPassword.trim();
    Serial.println("Received Password via Bluetooth: " + newPassword);
    if (newPassword.length() > 0) {
      password = newPassword;
      SerialBT.println("Password saved: " + password);
    } else {
      SerialBT.println("Password not changed.");
      Serial.println("No new password provided, using stored password.");
    }
    delay(500);

    // Save credentials to EEPROM
    EEPROM.writeString(ssidAddr, ssid);
    EEPROM.writeString(passAddr, password);
    EEPROM.commit();

    // Print saved values to Serial for verification
    Serial.println("Saved SSID: " + ssid);
    Serial.println("Saved Password: " + password);

    Serial.println("Credentials saved to EEPROM.");
    SerialBT.end();
    digitalWrite(LED, LOW);
    ESP.restart();
  } else {
    Serial.println("No Bluetooth connection. Proceeding with stored credentials.");
    ssid = EEPROM.readString(ssidAddr);
    password = EEPROM.readString(passAddr);
    Serial.println("Using stored SSID: " + ssid);
    Serial.println("Using stored Password: " + password);
    digitalWrite(LED, LOW);
  }
}


void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid.c_str(), password.c_str());
  int connectTime = 0;
  while (WiFi.status() != WL_CONNECTED && connectTime < WIFI_TIMEOUT) {
    delay(500);
    Serial.print(".");
    connectTime++;
  }

  Serial.println("");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Connected");
    for (int i = 0; i < 30; i++) {
      digitalWrite(LED, HIGH);
      delay(100);
      digitalWrite(LED, LOW);
      delay(100);
    }
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Failed to connect to WiFi. Device will restart now");
    ESP.restart();
  }
}

void setup_mqtt() {
  delay(100);
  client.setServer(MQTT_BROKER, MQTT_PORT);
  while (!client.connected()) {
    Serial.println("Connecting to MQTT....");

    if (client.connect(DEVICE_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state ");
      Serial.println(client.state());
      delay(200);
    }
  }
}

unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return 0;
  }
  time(&now);
  return now;
}
