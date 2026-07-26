#define TINY_GSM_MODEM_A7670
#define SerialMon Serial
#define SerialAT Serial1
#define TINY_GSM_DEBUG SerialMon

#include <TinyGsmClient.h>
#include <Arduino.h>
#include "config.h"


// GSM Internet Settings
#define GSM_PIN ""
const char apn[] = "internet";
const char gprsUser[] = "";
const char gprsPass[] = "";

#define ADMIN_NUMBER ""
String phoneNumber = "";
String text = "";

// GPS print interval (milliseconds)
#define GPS_INTERVAL_MS 5000

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

void setup() {
  SerialMon.begin(115200);
  delay(100);
  pinMode(MODEM_RESET_PIN, OUTPUT);
  digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
  delay(100);
  digitalWrite(MODEM_RESET_PIN, MODEM_RESET_LEVEL);
  delay(2600);
  digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);

  pinMode(MODEM_DTR_PIN, OUTPUT);
  digitalWrite(MODEM_DTR_PIN, LOW);

  // Turn on modem
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

  Serial.println("Enabling GPS/GNSS/GLONASS");
  while (!modem.enableGPS(MODEM_GPS_ENABLE_GPIO, MODEM_GPS_ENABLE_LEVEL)) {
    Serial.print(".");
  }
  Serial.println();
  Serial.println("GPS Enabled");

  // Set GPS Baud to 115200
  modem.setGPSBaud(115200);
}

void loop() {
  float lat2 = 0;
  float lon2 = 0;
  float speed2 = 0;
  float alt2 = 0;
  int vsat2 = 0;
  int usat2 = 0;
  float accuracy2 = 0;
  int year2 = 0;
  int month2 = 0;
  int day2 = 0;
  int hour2 = 0;
  int min2 = 0;
  int sec2 = 0;
  uint8_t fixMode = 0;

  Serial.println("Requesting current GPS/GNSS/GLONASS location");
  if (modem.getGPS(&fixMode, &lat2, &lon2, &speed2, &alt2, &vsat2, &usat2, &accuracy2,
                   &year2, &month2, &day2, &hour2, &min2, &sec2)) {

    Serial.print("FixMode:");
    Serial.println(fixMode);
    Serial.print("Latitude:");
    Serial.print(lat2, 6);
    Serial.print("\tLongitude:");
    Serial.println(lon2, 6);
    Serial.print("Speed:");
    Serial.print(speed2);
    Serial.print("\tAltitude:");
    Serial.println(alt2);
    Serial.print("Visible Satellites:");
    Serial.print(vsat2);

    // GPS_BuiltIn cannot get the number of satellites in use, so it always returns 0
    Serial.print("\tUsed Satellites:");
    Serial.println(usat2);
    Serial.print("Accuracy:");
    Serial.println(accuracy2);

    Serial.print("Year:");
    Serial.print(year2);
    Serial.print("\tMonth:");
    Serial.print(month2);
    Serial.print("\tDay:");
    Serial.println(day2);

    Serial.print("Hour:");
    Serial.print(hour2);
    Serial.print("\tMinute:");
    Serial.print(min2);
    Serial.print("\tSecond:");
    Serial.println(sec2);
  } else {
    Serial.println("Couldn't get GPS/GNSS/GLONASS location yet.");
  }

  Serial.println("--------------------------------------------------");

  // Wait 5 seconds before requesting the next reading
  delay(GPS_INTERVAL_MS);
}
