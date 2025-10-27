#define TINY_GSM_MODEM_A7670
#define SerialMon Serial
#define SerialAT Serial1
#define TINY_GSM_DEBUG SerialMon

#include <TinyGsmClient.h>

// ESP32 and A7670 pins
#define UART_BAUD 115200
#define MODEM_RESET_PIN 5
#define MODEM_PWKEY 4
#define MODEM_POWER_ON 12
#define MODEM_TX 26
#define MODEM_RX 27
#define MODEM_RESET_LEVEL HIGH
#define MODEM_GPS_ENABLE_GPIO -1

// GSM Internet Settings
#define GSM_PIN ""
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


#include <Arduino.h>
#include <TinyGPS++.h>

TinyGPSPlus gps;


void setup() {
  SerialMon.begin(115200);
  delay(100);
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_POWER_ON, HIGH);
  pinMode(MODEM_RESET_PIN, OUTPUT);
  digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
  delay(100);
  digitalWrite(MODEM_RESET_PIN, MODEM_RESET_LEVEL);
  delay(1000);
  digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
  pinMode(MODEM_PWKEY, OUTPUT);
  digitalWrite(MODEM_PWKEY, LOW);
  delay(100);
  digitalWrite(MODEM_PWKEY, HIGH);
  delay(1000);
  digitalWrite(MODEM_PWKEY, LOW);
  SerialMon.println("Wait ...");

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);
  DBG("Initializing modem ...");

  String name = modem.getModemName();
  DBG("Modem Name:", name);
  delay(500);
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
  String ccid = modem.getSimCCID();
  DBG("CCID:", ccid);
  delay(500);
  String imei = modem.getIMEI();
  DBG("IMEI:", imei);
  delay(500);
  // Get operator name
  String operatorName = modem.getOperator();
  SerialMon.print("Operator: ");
  SerialMon.println(operatorName);
  delay(500);
  int csq = modem.getSignalQuality();
  DBG("Signal quality:", csq);
  delay(500);
  SerialMon.print("Connecting to APN: ");
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
    ESP.restart();
  }
  SerialMon.println(" OK");
  delay(500);
  if (modem.isGprsConnected()) {
    SerialMon.println("GPRS connected");
  }
  IPAddress local = modem.localIP();
  DBG("Local IP:", local);
  delay(500);
  DBG("Asking modem to sync with NTP");
  modem.NTPServerSync("132.163.96.5", 20);
}

void loop() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 5000) {  // every 5 seconds
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
