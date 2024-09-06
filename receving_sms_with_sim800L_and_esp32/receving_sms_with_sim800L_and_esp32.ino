#define TINY_GSM_MODEM_SIM800
#define SerialMon Serial
#define SerialAT Serial1
#define TINY_GSM_DEBUG SerialMon
#define GSM_PIN ""

#define ADMIN_NUMBER "+8801715497977"
#define TINY_GSM_DEBUG SerialMon

#include <TinyGsmClient.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

// ESP32 and SIM800l pins
#define MODEM_TX 17
#define MODEM_RX 18


String phoneNumber = "";
String message = "";
bool isReceivingMessage = false;

void setup() {
  SerialMon.begin(115200);
  delay(1000);

  SerialMon.println("Wait ...");
  SerialAT.begin(9600, SERIAL_8N1, MODEM_TX, MODEM_RX);
  delay(3000);
  SerialMon.println("Initializing modem ...");
  delay(1000);
  Serial.println("Modem will restart now. Please Wait");
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

  SerialAT.print("AT+CMGF=1\r");
  delay(1000);

  SerialAT.print("AT+CNMI=2,2,0,0,0\r");
  delay(1000);
  String smsMessage = "Device Ready";
  Serial.println("Sending a device started Message: ");
  Serial.println(smsMessage);
  modem.sendSMS(ADMIN_NUMBER, smsMessage);
}

void loop() {
  while (SerialAT.available()) {
    String response = SerialAT.readStringUntil('\n');
    response.trim();
    if (response.startsWith("+CMT: ")) {
      phoneNumber = extractPhoneNumber(response);
      isReceivingMessage = true;
    }
    else if (isReceivingMessage) {
      message = response;
      isReceivingMessage = false;
      Serial.println("New Incomming Message: ");
      Serial.println("Message: " + message);
      Serial.println("Phone Number: " + phoneNumber);
      Serial.println();
    }
  }
}

String extractPhoneNumber(String response) {
  int startIndex = response.indexOf("\"") + 1;
  int endIndex = response.indexOf("\",", startIndex);
  return response.substring(startIndex, endIndex);
}