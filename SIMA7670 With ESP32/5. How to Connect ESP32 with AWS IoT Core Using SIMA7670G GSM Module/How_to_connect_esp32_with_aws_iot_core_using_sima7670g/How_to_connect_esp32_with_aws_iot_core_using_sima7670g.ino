#define TINY_GSM_MODEM_A7670
#define SerialMon Serial
#define SerialAT Serial1
#define TINY_GSM_DEBUG SerialMon
#define GSM_PIN ""

#include <PubSubClient.h>
#include <TinyGsmClient.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <DHT.h>
#include "certificate.h"
#include "privatekey.h"
#include "root_ca.h"
#include "config.h"


#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

uint32_t lastReconnectAttempt = 0;
long lastMsg = 0;
StaticJsonDocument<256> doc;
char buffer[256];

unsigned long timestamp;
const uint8_t mqtt_client_id = 0;
DHT dht(DHTPIN, DHTTYPE);



void mqtt_callback(const char* topic, const uint8_t* payload, uint32_t len) {
  Serial.println();
  Serial.println("======mqtt_callback======");
  Serial.print("Topic:");
  Serial.println(topic);
  Serial.println("Payload:");
  String messageTemp;
  for (int i = 0; i < len; i++) {
    Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }
  Serial.println();
  Serial.println("=========================");
}

bool mqtt_connect() {
  Serial.print("Connecting to ");
  Serial.print(broker);

  bool ret = modem.mqtt_connect(mqtt_client_id, broker, broker_port, client_id);
  if (!ret) {
    Serial.println("Failed!");
    return false;
  }
  Serial.println("successfully.");

  if (modem.mqtt_connected()) {
    Serial.println("MQTT has connected!");
  } else {
    return false;
  }
  // Set MQTT processing callback
  modem.mqtt_set_callback(mqtt_callback);
  // Subscribe to topic
  modem.mqtt_subscribe(mqtt_client_id, subscribe_topic);

  return true;
}

void setup() {
  SerialMon.begin(115200);
  delay(100);
  dht.begin();
  // Power ON the modem
  pinMode(MODEM_POWER_ON, OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(BUILTIN_LED, LOW);

  digitalWrite(MODEM_POWER_ON, HIGH);

  // Hardware reset
  pinMode(MODEM_RESET_PIN, OUTPUT);
  digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
  delay(100);
  digitalWrite(MODEM_RESET_PIN, MODEM_RESET_LEVEL);
  delay(2600);
  digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);

  // Toggle PWRKEY to power up the modem
  pinMode(MODEM_PWKEY, OUTPUT);
  digitalWrite(MODEM_PWKEY, LOW);
  delay(100);
  digitalWrite(MODEM_PWKEY, HIGH);
  delay(1000);
  digitalWrite(MODEM_PWKEY, LOW);

  SerialMon.println("Wait ...");
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  SerialMon.println("Initializing modem...");
  if (!modem.init()) {
    SerialMon.println("Failed to restart modem, delaying 10s and retrying");
    delay(10000);
    return;
  }

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);
  // Unlock SIM if needed
  if (GSM_PIN && modem.getSimStatus() != 3) {
    modem.simUnlock(GSM_PIN);
  }
  String ccid = modem.getSimCCID();
  DBG("CCID:", ccid);
  String imei = modem.getIMEI();
  DBG("IMEI:", imei);
  String imsi = modem.getIMSI();
  DBG("IMSI:", imsi);
  String cop = modem.getOperator();
  DBG("Operator:", cop);

  int csq = modem.getSignalQuality();
  DBG("Signal quality:", csq);

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");
  if (modem.isNetworkConnected()) {
    SerialMon.println("Network connected");
  }
  SerialMon.print("Connecting to APN: ");
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
    ESP.restart();
  }
  SerialMon.println(" OK");
  if (modem.isGprsConnected()) {
    SerialMon.println("GPRS connected");
  }
  IPAddress local = modem.localIP();
  DBG("Local IP:", local);

  DBG("Asking modem to sync with NTP");
  modem.NTPServerSync("132.163.96.5", 20);
  modem.mqtt_begin(true);
  modem.mqtt_set_certificate(root_ca, certificate, privateKey);

  if (mqtt_connect()) {
    SerialMon.println("Publishing Device Ready Status");
    modem.mqtt_publish(0, publish_topic, "Device Ready");
  }
  SerialMon.println("Published Sucessfully");
  SerialMon.println("Waiting for Command -- ");
}


void loop() {
  if (!modem.mqtt_connected()) {
    SerialMon.println("=== MQTT NOT CONNECTED ===");
    uint32_t t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqtt_connect()) {
        lastReconnectAttempt = 0;
      }
    }
    delay(100);
    return;
  }
  long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;
    publish_data();
  }

  modem.mqtt_handle();
}

void publish_data() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT11 sensor!");
    return;
  }

  StaticJsonDocument<200> doc;
  doc["device_id"] = DEVICE_ID;
  doc["temperature"] = t;
  doc["humidity"] = h;

  String jsonStr;
  serializeJson(doc, jsonStr);
  Serial.println(jsonStr);
  Serial.println("Publishing to MQTT Broker");
  modem.mqtt_publish(0, publish_topic, jsonStr.c_str());
}
