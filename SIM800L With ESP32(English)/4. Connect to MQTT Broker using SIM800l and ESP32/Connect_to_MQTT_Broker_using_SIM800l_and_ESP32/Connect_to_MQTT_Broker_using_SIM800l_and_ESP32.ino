#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <TimeLib.h>
#include "config.h"

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

TinyGsmClient client(modem);
PubSubClient mqtt(client);

uint32_t lastReconnectAttempt = 0;
long lastMsg = 0;
String mqtt_client_id = "";

void mqttCallback(char* topic, byte* message, unsigned int len) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.println(". Message: ");
  String incomming_message;

  for (int i = 0; i < len; i++) {
    incomming_message += (char)message[i];
  }
  incomming_message.trim();
  Serial.println(incomming_message);
  if (incomming_message == "ON") {
    Serial.println("Turing On Built-in LED");
    digitalWrite(BUILTIN_LED, HIGH);
  }
  if (incomming_message == "OFF") {
    Serial.println("Turing Off Built-in LED");
    digitalWrite(BUILTIN_LED, LOW);
  }
  Serial.println();
}

boolean mqttConnect() {
  SerialMon.print("Connecting to ");
  SerialMon.print(mqtt_broker);

  boolean status = mqtt.connect(mqtt_client_id.c_str(), mqtt_username, mqtt_password);

  if (status == false) {
    SerialMon.println(" fail");
    ESP.restart();
    return false;
  }
  SerialMon.println(" success");
  mqtt.subscribe(topic_sub);
  return mqtt.connected();
}


void setup() {
  SerialMon.begin(115200);
  delay(1000);
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);

  pinMode(MODEM_RST, OUTPUT);
  digitalWrite(MODEM_RST, LOW);
  delay(100);
  digitalWrite(MODEM_RST, HIGH);
  delay(1000);
  pinMode(MODEM_DTR, OUTPUT);
  digitalWrite(MODEM_DTR, HIGH);
  pinMode(MODEM_RING, INPUT);

  SerialMon.println("Wait ...");
  SerialAT.begin(115200, SERIAL_8N1, MODEM_TX, MODEM_RX);
  delay(3000);
  SerialMon.println("Initializing modem ...");
  modem.restart();
  delay(3000);
  modem.init();
  if (GSM_PIN && modem.getSimStatus() != 3) {
    modem.simUnlock(GSM_PIN);
  }
  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);
  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    delay(10000);  // wait 10s, for connected to network successfully
    return;
  }
  SerialMon.println(" success");
  if (modem.isNetworkConnected()) {
    DBG("Network connected");
  }
  String imei = modem.getIMEI();
  SerialMon.print("IMEI: ");
  SerialMon.println(imei);
  mqtt_client_id = "device-" + imei;

  String operatorName = modem.getOperator();
  SerialMon.print("Operator: ");
  SerialMon.println(operatorName);
  int signalQuality = modem.getSignalQuality();  // Signal quality (0–31, 99 = not known)
  SerialMon.print("Signal Quality (0-31): ");
  SerialMon.println(signalQuality);

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
  DBG("Asking modem to sync with NTP");
  modem.NTPServerSync("132.163.96.5", timezoneParam);
  // MQTT Broker setup
  mqtt.setServer(mqtt_broker, mqtt_port);
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
    publishDeviceData();
  }
  mqtt.loop();
}

void publishDeviceData() {
  SerialMon.println("Preparing to publish data...");
  time_t timestamp = get_timestamp();
  if (timestamp == 0) {
      SerialMon.println("Failed to get valid time. Skipping publish.");
      return;
  }
  char json_buffer[128];
  snprintf(json_buffer, sizeof(json_buffer), 
           "{\"deviceId\":\"%s\",\"timestamp\":%ld}", 
           mqtt_client_id.c_str(), 
           timestamp);
  SerialMon.print("Publishing message: ");
  SerialMon.println(json_buffer);
  mqtt.publish(topic_pub, json_buffer);
}

int get_timestamp() {
  int year3 = 0;
  int month3 = 0;
  int day3 = 0;
  int hour3 = 0;
  int min3 = 0;
  int sec3 = 0;
  float timezone = 0;
  for (int8_t i = 5; i; i--) {
    DBG("Requesting current network time");
    if (modem.getNetworkTime(&year3, &month3, &day3, &hour3, &min3, &sec3,
                             &timezone)) {
      break;
    } else {
      DBG("Couldn't get network time, retrying in 15s.");
      delay(15000L);
    }
  }

  setTime(hour3, min3, sec3, day3, month3, year3);
  int ct = now();
  return ct;
}