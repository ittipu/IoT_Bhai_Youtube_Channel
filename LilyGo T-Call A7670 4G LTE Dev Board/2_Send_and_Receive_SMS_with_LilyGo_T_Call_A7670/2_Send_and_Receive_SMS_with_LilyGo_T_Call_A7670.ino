#define TINY_GSM_MODEM_A7670
#define SerialMon Serial
#define SerialAT Serial1
#define TINY_GSM_DEBUG SerialMon

#include <TinyGsmClient.h>
#include <Arduino.h>
#include "config.h"

// GSM Internet Settings
#define GSM_PIN ""
const char apn[]      = "internet";
const char gprsUser[] = "";
const char gprsPass[] = "";

#define ADMIN_NUMBER "+8801750127169"

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

// SMS parsing state
bool   waitingForBody = false;
String sender         = "";
bool   ledState       = false;

void setup() {
  SerialMon.begin(115200);
  delay(100);

  // On-board LED
  pinMode(BOARD_LED_PIN, OUTPUT);
  digitalWrite(BOARD_LED_PIN, LOW);

  // Modem reset
  pinMode(MODEM_RESET_PIN, OUTPUT);
  digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
  delay(100);
  digitalWrite(MODEM_RESET_PIN, MODEM_RESET_LEVEL);
  delay(2600);
  digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);

  pinMode(MODEM_DTR_PIN, OUTPUT);
  digitalWrite(MODEM_DTR_PIN, LOW);

  // Power on modem
  pinMode(BOARD_PWRKEY_PIN, OUTPUT);
  digitalWrite(BOARD_PWRKEY_PIN, LOW);
  delay(100);
  digitalWrite(BOARD_PWRKEY_PIN, HIGH);
  delay(MODEM_POWERON_PULSE_WIDTH_MS);
  digitalWrite(BOARD_PWRKEY_PIN, LOW);

  SerialMon.println("Wait ...");

  SerialAT.begin(MODEM_BAUDRATE, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
  delay(3000);
  DBG("Initializing modem ...");

  String name = modem.getModemName();
  DBG("Modem Name:", name);
  delay(500);

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);

  if (strlen(GSM_PIN) && modem.getSimStatus() != 3) {
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

  // Startup SMS to admin
  SerialMon.print("Init success, sending boot message to ");
  SerialMon.println(ADMIN_NUMBER);
  bool res = modem.sendSMS(
    ADMIN_NUMBER,
    String("Boot OK. IMEI: ") + imei + ". Send LED ON / LED OFF / STATUS."
  );
  SerialMon.print("Boot SMS: ");
  SerialMon.println(res ? "OK" : "fail");

  // SMS text mode + push new SMS straight to UART as +CMT
  SerialAT.print("AT+CMGF=1\r");
  delay(1000);
  SerialAT.print("AT+CNMI=2,2,0,0,0\r");
  delay(1000);
}

void handleCommand(const String& rawCmd) {
  String cmd = rawCmd;
  cmd.trim();
  cmd.toUpperCase();

  Serial.print("Command from ");
  Serial.print(sender);
  Serial.print(": ");
  Serial.println(cmd);

  // Whitelist — only ADMIN_NUMBER can control the board
  if (sender != ADMIN_NUMBER) {
    modem.sendSMS(sender, "Not authorized.");
    return;
  }

  if (cmd == "LED ON") {
    digitalWrite(BOARD_LED_PIN, HIGH);
    ledState = true;
    modem.sendSMS(sender, "LED is ON");
  }
  else if (cmd == "LED OFF") {
    digitalWrite(BOARD_LED_PIN, LOW);
    ledState = false;
    modem.sendSMS(sender, "LED is OFF");
  }
  else if (cmd == "STATUS") {
    String reply = "LED is ";
    reply += ledState ? "ON" : "OFF";
    reply += ". Signal: ";
    reply += modem.getSignalQuality();
    modem.sendSMS(sender, reply);
  }
  else {
    modem.sendSMS(sender, "Unknown. Try: LED ON / LED OFF / STATUS");
  }
}

void loop() {
  while (SerialAT.available()) {
    String line = SerialAT.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    Serial.println(line);

    // Header: +CMT: "+8801...","","25/03/15,12:34:56+24"
    if (line.startsWith("+CMT:")) {
      int firstQuote  = line.indexOf('"');
      int secondQuote = line.indexOf('"', firstQuote + 1);
      if (firstQuote >= 0 && secondQuote > firstQuote) {
        sender         = line.substring(firstQuote + 1, secondQuote);
        waitingForBody = true;
        Serial.print("SMS from: ");
        Serial.println(sender);
      }
      continue;
    }

    // Next non-empty line after +CMT: is the message body
    if (waitingForBody) {
      waitingForBody = false;
      handleCommand(line);
    }
  }
}