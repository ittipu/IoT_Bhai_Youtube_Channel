#define TINY_GSM_MODEM_A7670
#define SerialMon Serial
#define SerialAT Serial1
#define TINY_GSM_DEBUG SerialMon
#define GSM_PIN ""

#include <PubSubClient.h>
#include <TinyGsmClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "config.h"

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif


TinyGsmClient client(modem);
PubSubClient mqtt(client);
DHT dht(DHTPIN, DHTTYPE);

uint32_t lastReconnectAttempt = 0;
long lastMsg = 0;

void mqttCallback(char *topic, byte *message, unsigned int len) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < len; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
}

boolean mqttConnect() {
  SerialMon.print("Connecting to ");
  SerialMon.print(mqtt_broker);
  boolean status = mqtt.connect(DEVICE_ID, mqtt_username, mqtt_password);
  if (status == false) {
    SerialMon.println(" fail");
    ESP.restart();
    return false;
  }
  SerialMon.println(" success");
  mqtt.subscribe(subs_topic);
  return mqtt.connected();
}

void setup() {
  SerialMon.begin(115200);
  // Power ON the modem
  pinMode(MODEM_POWER_ON, OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(BUILTIN_LED, LOW);
  dht.begin();

  digitalWrite(MODEM_POWER_ON, HIGH);
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
  // MQTT Broker setup
  mqtt.setServer(mqtt_broker, port);
  mqtt.setCallback(mqttCallback);
}

void loop() {
  if (!mqtt.connected()) {
    SerialMon.println("=== MQTT NOT CONNECTED ===");
    // Reconnect every 10 seconds
    uint32_t t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
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
  mqtt.loop();
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
  mqtt.publish(publish_topic, jsonStr.c_str());
}
