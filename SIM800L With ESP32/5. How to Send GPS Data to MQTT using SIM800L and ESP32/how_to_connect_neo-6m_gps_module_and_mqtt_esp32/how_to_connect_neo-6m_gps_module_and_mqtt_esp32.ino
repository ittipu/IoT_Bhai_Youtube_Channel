#define TINY_GSM_MODEM_SIM800
#define SerialMon Serial
#define SerialGPS Serial1
#define SerialAT Serial2
#define TINY_GSM_DEBUG SerialMon
#define GSM_PIN ""

#include <PubSubClient.h>
#include <TinyGsmClient.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <TinyGPS++.h>

#define DEVICE_ID "gps001"

const char apn[] = "internet";
const char gprsUser[] = "";
const char gprsPass[] = "";

// SIM card PIN
const char simPIN[] = "";

// MQTT details
#define MAX_MQTT_RECONNECT_COUNTER 10
const char* broker = "broker.hivemq.com";
const char* mqttUsername = "";
const char* mqttPassword = "";

const char* publish_topic = "esp32/gps_data";

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

TinyGsmClient client(modem);
PubSubClient mqtt(client);
TinyGPSPlus gps;

// ESP32 and SIM800l pins
#define MODEM_TX 17
#define MODEM_RX 18

// ESP32 and GPS pins
#define GPS_TX_PIN 27
#define GPS_RX_PIN 26

uint32_t lastReconnectAttempt = 0;
uint8_t mqtt_reconnect_counter = 0;
long lastMsg = 0;
float lat = 0;
float lng = 0;
StaticJsonDocument<256> doc;
unsigned long timestamp;


boolean mqttConnect() {
  SerialMon.print("Connecting to ");
  SerialMon.print(broker);

  boolean status = mqtt.connect(DEVICE_ID, mqttUsername, mqttPassword);

  if (status == false) {
    SerialMon.println(" fail");
    return false;
  }
  SerialMon.println(" success");
  return mqtt.connected();
}

void setup() {
  SerialMon.begin(115200);
  delay(10);
  SerialMon.println("Wait ...");
  SerialGPS.begin(9600, SERIAL_8N1, GPS_TX_PIN, GPS_RX_PIN);
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
}

void loop() {
  if (!mqtt.connected()) {
    SerialMon.println("=== MQTT NOT CONNECTED ===");
    // Reconnect every 10 seconds
    uint32_t t = millis();
    if (t - lastReconnectAttempt > 3000L) {
      mqtt_reconnect_counter++;
      if (mqtt_reconnect_counter == MAX_MQTT_RECONNECT_COUNTER) {
        SerialMon.println("MQTT Reconnect Counter Reaches MAX! Going to restart");
        ESP.restart();
      }
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        mqtt_reconnect_counter = 0;
        lastReconnectAttempt = 0;
      }
    }
    delay(100);
    return;
  }
  long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;
    get_gps_data();
  }
  mqtt.loop();
}

void get_gps_data() {
  Serial.println("Getting data: ");
  float lat = 0, lng = 0;
  for (int i = 2; i; i--) {
    while (SerialGPS.available() > 0) {
      gps.encode(SerialGPS.read());
    }
    delay(1000);
  }
  if (gps.location.isValid()) {
    lat = gps.location.lat();
    lng = gps.location.lng();
    Serial.print("Latitude = ");
    Serial.println(lat, 6);
    Serial.print("Longitude= ");
    Serial.println(lng, 6);
  } else {
    Serial.println(F("Invalid"));
  }

  char buffer[256];

  timestamp = get_timestamp();
  doc["deviceID"] = DEVICE_ID;
  doc["lat"] = lat;
  doc["lng"] = lng;
  doc["timestamp"] = timestamp;

  SerialMon.println("Publish to broker: ");
  serializeJson(doc, SerialMon);
  SerialMon.println();
  serializeJson(doc, buffer);
  mqtt.publish(publish_topic, buffer);
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
