#define TINY_GSM_MODEM_SIM800
#define SerialMon Serial
#define TINY_GSM_DEBUG SerialMon
#define SerialAT Serial1
#define GSM_PIN ""

#include <TinyGsmClient.h>
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
const int timezoneParam = UTC_OFFSET_HOURS * 4;


void setup() {
  SerialMon.begin(115200);
  delay(1000);
  pinMode(MODEM_RST, OUTPUT);
  digitalWrite(MODEM_RST, HIGH);
  delay(1000);
  digitalWrite(MODEM_RST, LOW);
  delay(1000);
  digitalWrite(MODEM_RST, HIGH);
  delay(1000);

  pinMode(MODEM_DTR, OUTPUT);
  digitalWrite(MODEM_DTR, HIGH);
  pinMode(MODEM_RING, INPUT);

  SerialMon.println("Wait ...");
  SerialAT.begin(115200, SERIAL_8N1, MODEM_TX, MODEM_RX);
  SerialMon.println("Initializing modem ...");
  modem.init();
  if (GSM_PIN && modem.getSimStatus() != 3) {
    modem.simUnlock(GSM_PIN);
  }
  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);

  SerialMon.print("Wait for network...");
  if (!modem.waitForNetwork(600000L, true)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isNetworkConnected()) {
    DBG("Network connected");
  }

  String imei = modem.getIMEI();
  SerialMon.print("IMEI: ");
  SerialMon.println(imei);
  delay(500);

  String operatorName = modem.getOperator();
  SerialMon.print("Operator: ");
  SerialMon.println(operatorName);
  delay(500);

  int signalQuality = modem.getSignalQuality();
  SerialMon.print("Signal Quality (0-31): ");
  SerialMon.println(signalQuality);
  delay(500);

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
  IPAddress localIP = modem.localIP();
  SerialMon.print("Local IP: ");
  SerialMon.println(localIP);
  DBG("Asking modem to sync with NTP");
  modem.NTPServerSync("132.163.96.5", timezoneParam);
}

void loop() {
  SerialMon.println("--- Current Time Details ---");
  int timestamp = get_timestamp();
  Serial.print("Currect timestamp: ");
  Serial.println(timestamp);
  // Print Date in YYYY-MM-DD format
  SerialMon.print("Date: ");
  SerialMon.print(year());
  SerialMon.print("-");
  if (month() < 10) { SerialMon.print("0"); }
  SerialMon.print(month());
  SerialMon.print("-");
  if (day() < 10) { SerialMon.print("0"); }
  SerialMon.print(day());

  // Print Time in HH:MM:SS format
  SerialMon.print("  Time: ");
  if (hour() < 10) { SerialMon.print("0"); }
  SerialMon.print(hour());
  SerialMon.print(":");
  if (minute() < 10) { SerialMon.print("0"); }
  SerialMon.print(minute());
  SerialMon.print(":");
  if (second() < 10) { SerialMon.print("0"); }
  SerialMon.print(second());

  // Print the day of the week
  SerialMon.print("  Day: ");
  SerialMon.println(daysOfTheWeek[weekday() - 1]);

  SerialMon.println();
  delay(10000);
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
