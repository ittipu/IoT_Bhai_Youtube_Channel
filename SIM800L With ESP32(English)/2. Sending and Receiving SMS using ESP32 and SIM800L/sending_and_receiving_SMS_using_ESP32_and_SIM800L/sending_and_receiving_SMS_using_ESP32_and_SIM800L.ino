#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>
#include "config.h"

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

String extractPhoneNumber(String response);
void handleSms(String number, String msg);

TinyGsmClient client(modem);

// Variables for handling incoming SMS
bool isReceivingMessage = false;
String received_message = "";
String sender_number = "";

void setup() {
  SerialMon.begin(115200);
  delay(1000);
  pinMode(MODEM_RST, OUTPUT);
  digitalWrite(MODEM_RST, LOW);
  delay(100);
  digitalWrite(MODEM_RST, HIGH);
  delay(1000);
  pinMode(MODEM_DTR, OUTPUT);
  digitalWrite(MODEM_DTR, HIGH);
  pinMode(MODEM_RING, INPUT);

  SerialMon.println("Wait ...");
  SerialAT.begin(115200, SERIAL_8N1, MODEM_TX, MODEM_RX);
  delay(3000);
  SerialMon.println("Initializing modem ...");
  modem.restart();
  delay(3000);
  modem.init();
  if (GSM_PIN && modem.getSimStatus() != 3) {
    modem.simUnlock(GSM_PIN);
  }
  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);
  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    delay(10000);  // wait 10s, for connected to network successfully
    return;
  }
  SerialMon.println(" success");
  if (modem.isNetworkConnected()) {
    DBG("Network connected");
  }
  String imei = modem.getIMEI();
  SerialMon.print("IMEI: ");
  SerialMon.println(imei);
  String operatorName = modem.getOperator();
  SerialMon.print("Operator: ");
  SerialMon.println(operatorName);
  int signalQuality = modem.getSignalQuality();  // Signal quality (0–31, 99 = not known)
  SerialMon.print("Signal Quality (0-31): ");
  SerialMon.println(signalQuality);
  SerialMon.println("Enabling network time synchronization...");
  modem.sendAT("+CLTS=1");
  delay(500);
  modem.sendAT("&W");
  delay(500);
  modem.restart();
  delay(3000);
  modem.waitForNetwork();
  modem.sendAT("+CMGF=1");
  delay(1000);
  SerialAT.print("AT+CNMI=2,2,0,0,0\r");
  delay(1000);

  String smsMessage = "GSM Device Started";
  SerialMon.print("Sending SMS to ");
  SerialMon.println(ADMIN_NUMBER);

  bool success = modem.sendSMS(ADMIN_NUMBER, smsMessage);
  if (success) {
    SerialMon.println("SMS sent successfully!");
  } else {
    SerialMon.println("SMS failed to send.");
  }
}

void loop() {
  while (SerialAT.available()) {
    String response = SerialAT.readStringUntil('\n');
    response.trim();
    if (response.startsWith("+CMT: ")) {
      sender_number = extractPhoneNumber(response);
      isReceivingMessage = true;
    } else if (isReceivingMessage) {
      received_message = response;
      isReceivingMessage = false;
      received_message.trim(); 
      received_message.toLowerCase();
      handleSms(sender_number, received_message);
    }
  }
}

void handleSms(String number, String msg) {
  SerialMon.println("New Incoming Message!");
  SerialMon.print("From: ");
  SerialMon.println(number);
  SerialMon.print("Message: ");
  SerialMon.println(msg);

  if (msg == "status") {
    SerialMon.println("'status' command received. Preparing reply...");

    int signalQuality = modem.getSignalQuality();
    String operatorName = modem.getOperator();
    
    int year, month, day, hour, min, sec;
    float timezone;
    String dateTimeStr = "Not available";
    if (modem.getNetworkTime(&year, &month, &day, &hour, &min, &sec, &timezone)) {
        char dateTimeBuffer[25];
        sprintf(dateTimeBuffer, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, min, sec);
        dateTimeStr = String(dateTimeBuffer);
    }

    String reply_message = "--- Device Status ---\n";
    reply_message += "Status: Online\n";
    reply_message += "Operator: " + operatorName + "\n";
    reply_message += "Signal: " + String(signalQuality) + "/31\n";
    reply_message += "Time: " + dateTimeStr;

    SerialMon.print("Sending reply: ");
    SerialMon.println(reply_message);
    bool success = modem.sendSMS(number, reply_message);
    if (success) {
      SerialMon.println("Reply sent successfully!");
    } else {
      SerialMon.println("Failed to send reply.");
    }
  } else {
    SerialMon.println("Unrecognized command.");
    String reply_message = "Unknown command. Try sending 'status'.";
    modem.sendSMS(number, reply_message);
  }

  SerialMon.println();
}

String extractPhoneNumber(String response) {
  int startIndex = response.indexOf("\"") + 1;
  int endIndex = response.indexOf("\",", startIndex);
  return response.substring(startIndex, endIndex);
}