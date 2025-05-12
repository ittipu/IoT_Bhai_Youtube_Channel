#define TINY_GSM_MODEM_A7670
#define SerialMon Serial
#define SerialAT Serial1
#define TINY_GSM_DEBUG SerialMon
#define GSM_PIN ""

#include <TinyGsmClient.h>

#define ADMIN_NUMBER "+8801715497977"

// === SIM7600 Pin Definitions ===
#define MODEM_RESET_PIN 5
#define MODEM_PWKEY 4
#define MODEM_POWER_ON 12
#define MODEM_TX 26
#define MODEM_RX 27
#define MODEM_RESET_LEVEL HIGH

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

String phoneNumber = "";
String text = "";

void setup() {
  SerialMon.begin(115200);
  // Power ON the modem
  pinMode(MODEM_POWER_ON, OUTPUT);
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
  Serial.println("Wait SMS Done.");
  if (!modem.waitResponse(100000UL, "SMS DONE")) {
    Serial.println("Can't wait from sms register ....");
    return;
  }
  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);
  // Unlock SIM if needed
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
    SerialMon.println("Network connected");
  }

  String imei = modem.getIMEI();
  Serial.print("Init success, start to send message to  ");
  Serial.println(ADMIN_NUMBER);
  bool res = modem.sendSMS(ADMIN_NUMBER, String("Hello from ") + imei);
  Serial.print("Send sms message ");
  Serial.println(res ? "OK" : "fail");
  
  // Enable SMS text mode and notifications
  SerialAT.print("AT+CMGF=1\r");
  delay(1000);
  SerialAT.print("AT+CNMI=2,2,0,0,0\r");
  delay(1000);
}

void loop() {
  while (SerialAT.available()) {
    String response = SerialAT.readStringUntil('\n');
    Serial.println(response);
    // Incoming SMS
    if (response.startsWith("+CMT: ")) {
      int firstQuotePos = response.indexOf("+CMT: ") + 7;
      int secondQuotePos = response.indexOf("\"", firstQuotePos);
      phoneNumber = response.substring(firstQuotePos, secondQuotePos);
      Serial.print("Phone Number: ");
      Serial.println(phoneNumber);
    }
  }
}
