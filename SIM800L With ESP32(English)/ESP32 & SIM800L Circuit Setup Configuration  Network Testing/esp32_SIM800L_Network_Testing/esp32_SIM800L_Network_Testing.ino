#define TINY_GSM_MODEM_SIM800
#define SerialMon Serial
#define SerialAT Serial1
#define TINY_GSM_DEBUG SerialMon
#define GSM_PIN ""


#define NTP_SERVER "132.163.96.5"

#include <PubSubClient.h>
#include <TinyGsmClient.h>
#include <ArduinoJson.h>
#include <TimeLib.h>

const char apn[] = "internet";
const char gprsUser[] = "";
const char gprsPass[] = "";

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
#define MODEM_TX 26
#define MODEM_RX 27
#define MODEM_RST 14
#define MODEM_DTR 25
#define MODEM_RING 34

uint32_t lastReconnectAttempt = 0;
long lastMsg = 0;
float lat = 0;
float lng = 0;
StaticJsonDocument<256> doc;
unsigned long timestamp;


void setup() {
  SerialMon.begin(115200);
  delay(1000);
  pinMode(MODEM_RST, OUTPUT);
  digitalWrite(MODEM_RST, LOW);
  delay(100);                     // Keep low for 100ms
  digitalWrite(MODEM_RST, HIGH);  // Release reset pin
  delay(1000);                    // Wait for module to boot
  pinMode(MODEM_DTR, OUTPUT);
  digitalWrite(MODEM_DTR, HIGH);  // Keep modem awake
  pinMode(MODEM_RING, INPUT);     // Optional

  SerialMon.println("Wait ...");
  SerialAT.begin(115200, SERIAL_8N1, MODEM_TX, MODEM_RX);
  delay(3000);
  SerialMon.println("Initializing modem ...");
  modem.init();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);

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
  // IMEI
  String imei = modem.getIMEI();
  SerialMon.print("IMEI: ");
  SerialMon.println(imei);

  // Get operator name
  String operatorName = modem.getOperator();
  SerialMon.print("Operator: ");
  SerialMon.println(operatorName);

  // Signal quality (0–31, 99 = not known)
  int signalQuality = modem.getSignalQuality();
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
  // Local IP
  IPAddress  localIP = modem.localIP();
  SerialMon.print("Local IP: ");
  SerialMon.println(localIP);
}

void loop() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 5000) { // every 5 seconds
    int signal = modem.getSignalQuality();
    SerialMon.print("Signal Quality (0–31): ");
    SerialMon.println(signal);

    // Optional: Check if still connected
    if (!modem.isNetworkConnected()) {
      SerialMon.println("⚠️ Network disconnected!");
    } else {
      SerialMon.println("✅ Network OK");
    }

    lastPrint = millis();
  }

  delay(100);
}
