#define TINY_GSM_MODEM_SIM800
#define SerialMon Serial
#define SerialAT Serial1
#define TINY_GSM_DEBUG SerialMon
#define GSM_PIN ""

#include <PubSubClient.h>
#include <TinyGsmClient.h>
#include <ArduinoJson.h>
#include <TimeLib.h>

#define DEVICE_ID "Test001"
#define BUILTIN_LED 2

const char apn[] = "internet";
const char gprsUser[] = "";
const char gprsPass[] = "";

// SIM card PIN
const char simPIN[] = "";

// MQTT details
const char* broker = "broker.hivemq.com";
const char* mqttUsername = "";
const char* mqttPassword = "";
const char* topicPubData = "test/data";
const char* topicSubLed = "test/led";



#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

TinyGsmClient client(modem);
PubSubClient mqtt(client);


// ESP32 and SIM800l pins
#define MODEM_TX 17
#define MODEM_RX 18

uint32_t lastReconnectAttempt = 0;
long lastMsg = 0;
float lat = 0;
float lng = 0;
StaticJsonDocument<256> doc;
unsigned long timestamp;


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
  SerialMon.print(broker);

  boolean status = mqtt.connect(DEVICE_ID, mqttUsername, mqttPassword);

  if (status == false) {
    SerialMon.println(" fail");
    ESP.restart();
    return false;
  }
  SerialMon.println(" success");
  mqtt.subscribe(topicSubLed);
  return mqtt.connected();
}

void setup() {
  SerialMon.begin(115200);
  delay(10);
  pinMode(BUILTIN_LED, OUTPUT);
  SerialMon.println("Wait ...");
  SerialAT.begin(115200, SERIAL_8N1, MODEM_TX, MODEM_RX);
  delay(3000);
  SerialMon.println("Initializing modem ...");
  modem.restart();

  if (GSM_PIN && modem.getSimStatus() != 3) {
    modem.simUnlock(GSM_PIN);
  }

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isNetworkConnected()) {
    DBG("Network connected");
  }

  String cop = modem.getOperator();
  DBG("Operator:", cop);

  int csq = modem.getSignalQuality();
  DBG("Signal quality:", csq);

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
  modem.NTPServerSync("132.163.96.5", 20);

  // MQTT Broker setup
  mqtt.setServer(broker, 1883);
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
    get_data();
  }
  mqtt.loop();
}

void get_data() {
  Serial.println("Getting Data: ");
  char buffer[256];
  timestamp = get_timestamp();
  doc["deviceID"] = DEVICE_ID;
  doc["timestamp"] = timestamp;

  SerialMon.print("Publish to broker: ");
  serializeJson(doc, SerialMon);
  SerialMon.println();
  serializeJson(doc, buffer);
  mqtt.publish(topicPubData, buffer);
  Serial.println();
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
  SerialMon.print("Timestamp: ");
  int ct = now();
  SerialMon.println(ct);
  return ct;
}
