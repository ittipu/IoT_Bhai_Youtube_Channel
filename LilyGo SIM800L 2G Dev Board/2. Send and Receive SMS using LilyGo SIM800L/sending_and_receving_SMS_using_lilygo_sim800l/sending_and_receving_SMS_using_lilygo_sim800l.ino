#define TINY_GSM_MODEM_SIM800
#define SerialMon Serial
#define SerialAT Serial1
#define TINY_GSM_DEBUG SerialMon
#define GSM_PIN ""
#include <TinyGsmClient.h>

#define ADMIN_NUMBER "+8801715497977"

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

// ESP32 and SIM800l pins
#define MODEM_RST 5
#define MODEM_PWKEY 4
#define MODEM_POWER_ON 23
#define MODEM_TX 27
#define MODEM_RX 26
#define I2C_SDA 21
#define I2C_SCL 22

TinyGsmClient client(modem);

bool isReceivingMessage = false;
String received_message = "";
String sender_number = "";

void setup() {
  SerialMon.begin(115200);
  delay(1000);
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);

  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);

  SerialMon.println("Wait ...");
  SerialAT.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);
  SerialMon.println("Initializing modem ...");
  modem.restart();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);

  // Unlock your sim card with a PIN if needed
  if (GSM_PIN && modem.getSimStatus() != 3) {
    modem.simUnlock(GSM_PIN);
  }
  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork(240000L)) {
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

  String imei = modem.getIMEI();
  DBG("IMEI:", imei);

  String imsi = modem.getIMSI();
  DBG("IMSI:", imsi);

  String cop = modem.getOperator();
  DBG("Operator:", cop);

  // Signal quality (0–31, 99 = not known)
  int signalQuality = modem.getSignalQuality();
  SerialMon.print("Signal Quality (0-31): ");
  SerialMon.println(signalQuality);
  SerialMon.println("Enabling network time synchronization...");
  modem.sendAT("+CLTS=1");
  delay(500);
  modem.sendAT("&W");
  delay(500);

  modem.sendAT("+CMGF=1");
  delay(1000);
  SerialAT.print("AT+CNMI=2,2,0,0,0\r");
  delay(1000);
  String smsMessage = "LilyGo SIM800l Device Started";
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
