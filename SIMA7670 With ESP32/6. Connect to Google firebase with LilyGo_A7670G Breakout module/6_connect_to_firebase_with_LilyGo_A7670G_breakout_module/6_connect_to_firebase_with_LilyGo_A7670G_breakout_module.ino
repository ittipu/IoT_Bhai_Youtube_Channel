#define TINY_GSM_MODEM_SIM7600
#define SerialMon Serial
#define SerialAT Serial1
#define SerialGPS Serial2
#define TINY_GSM_DEBUG SerialMon
#define GSM_PIN ""

#define ENABLE_USER_AUTH
#define ENABLE_DATABASE
#define ENABLE_GSM_NETWORK
#define ENABLE_ESP_SSLCLIENT


// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[] = "internet";
const char gprsUser[] = "";
const char gprsPass[] = "";

#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP 600          // Time ESP32 will go to sleep (in seconds)

#define DEVICE_ID "W001"

// === DHT11 Pin Definitions ===
#define DHTPIN 23
#define DHTTYPE DHT11  // DHT 22  (AM2302), AM2321

// === SIM7600 Pin Definitions ===
#define MODEM_RESET_PIN 5
#define MODEM_PWKEY 4
#define MODEM_POWER_ON 12
#define MODEM_TX 26
#define MODEM_RX 27
#define MODEM_RESET_LEVEL HIGH
#define BUILTIN_LED 2

// Include TinyGsmClient.h first and followed by FirebaseClient.h
#include <Arduino.h>
#include <TinyGsmClient.h>
#include <FirebaseClient.h>
#include "ExampleFunctions.h"
#include <TinyGPS++.h>
#include <DHT.h>

// The API key can be obtained from Firebase console > Project Overview > Project settings.
#define API_KEY ""
// User Email and password that already registerd or added in your project.
#define USER_EMAIL "test@email.com"
#define USER_PASSWORD "123456"
#define DATABASE_URL "a7670g-test-device-default-rtdb.firebaseio.com/"

TinyGsm modem(SerialAT);
TinyGsmClient gsm_client(modem, 0), stream_gsm_client(modem, 1);
TinyGPSPlus gps;
void processData(AsyncResult &aResult);

ESP_SSLClient ssl_client, stream_ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client), streamClient(stream_ssl_client);
UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD, 3000);
FirebaseApp app;
RealtimeDatabase Database;
AsyncResult streamResult;

unsigned long ms = 0;
DHT dht(DHTPIN, DHTTYPE);

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

  // Unlock your sim card with a PIN if needed
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
  String ccid = modem.getSimCCID();
  DBG("CCID:", ccid);
  delay(100);
  String imei = modem.getIMEI();
  DBG("IMEI:", imei);
  delay(100);
  String imsi = modem.getIMSI();
  DBG("IMSI:", imsi);
  delay(100);
  String cop = modem.getOperator();
  DBG("Operator:", cop);
  delay(100);
  SerialMon.print("Connecting to APN: ");
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
    ESP.restart();
  }
  SerialMon.println(" OK");
  delay(100);
  if (modem.isGprsConnected()) {
    SerialMon.println("GPRS connected");
  }
  delay(100);
  IPAddress local = modem.localIP();
  DBG("Local IP:", local);
  delay(100);
  int csq = modem.getSignalQuality();
  DBG("Signal quality:", csq);
  delay(1000);

  Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);
  ssl_client.setInsecure();
  ssl_client.setDebugLevel(1);
  ssl_client.setBufferSizes(2048 /* rx */, 1024 /* tx */);
  ssl_client.setClient(&gsm_client);

  stream_ssl_client.setInsecure();
  stream_ssl_client.setDebugLevel(1);
  stream_ssl_client.setBufferSizes(2048 /* rx */, 1024 /* tx */);
  stream_ssl_client.setClient(&stream_gsm_client);

  Serial.println("Initializing app...");
  initializeApp(aClient, app, getAuth(user_auth), auth_debug_print, "🔐 authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
  streamClient.setSSEFilters("get,put,patch,keep-alive,cancel,auth_revoked");
  Database.get(streamClient, "/examples/Stream/data", processData, true /* SSE mode */, "streamTask");
}

void loop() {
  app.loop();
  if (millis() - ms > 10000 && app.ready()) {
    ms = millis();
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT11 sensor!");
      return;
    }

    JsonWriter writer;
    object_t json, obj1, obj2;
    writer.create(obj1, "temperature", t);
    writer.create(obj2, "humidity", h);
    writer.join(json, 2, obj1, obj2);
    Database.set<object_t>(aClient, "/room_condition/", json, processData, "setTask");
  }
  processData(streamResult);
}

void processData(AsyncResult &aResult) {
  if (!aResult.isResult())
    return;

  if (aResult.isEvent()) {
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());
  }

  if (aResult.isDebug()) {
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
  }

  if (aResult.isError()) {
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
  }

  if (aResult.available()) {
    RealtimeDatabaseResult &stream = aResult.to<RealtimeDatabaseResult>();
    if (stream.isStream()) {
      Serial.println("----------------------------");
      Firebase.printf("task: %s\n", aResult.uid().c_str());
      Firebase.printf("event: %s\n", stream.event().c_str());
      Firebase.printf("path: %s\n", stream.dataPath().c_str());
      Firebase.printf("data: %s\n", stream.to<const char *>());
      Firebase.printf("type: %d\n", stream.type());

      // The stream event from RealtimeDatabaseResult can be converted to the values as following.
      bool v1 = stream.to<bool>();
      int v2 = stream.to<int>();
      float v3 = stream.to<float>();
      double v4 = stream.to<double>();
      String v5 = stream.to<String>();
    } else {
      Serial.println("----------------------------");
      Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
    }
    Firebase.printf("Free Heap: %d\n", ESP.getFreeHeap());
  }
}
