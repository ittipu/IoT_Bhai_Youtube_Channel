/**************************************************************
 * Experiment 01: Arduino & SIM800L - Network & Signal Test
 * * WHAT THIS DOES:
 * 1. Connects Arduino Uno to SIM800L using SoftwareSerial.
 * 2. Initializes the modem to check if wiring is correct.
 * 3. Waits for the SIM card to connect to the cellular network.
 * 4. Prints Signal Quality (CSQ) every 5 seconds.
 * * HARDWARE WIRING:
 * - SIM800L VCC <--> External Battery (+) [5V 2A]
 * - SIM800L GND <--> External Battery (-) AND Arduino GND (Common Ground)
 * - SIM800L TX  <--> Arduino Pin 4
 * - SIM800L RX  <--> Arduino Pin 3 (VIA VOLTAGE DIVIDER 1k/2k)
 **************************************************************/

#define TINY_GSM_MODEM_SIM800
#define SerialMon Serial
#define TINY_GSM_DEBUG SerialMon
#define GSM_PIN ""

#include <SoftwareSerial.h>
#include <TinyGsmClient.h>

// Arduino Pin 4 -> SIM800L TX
// Arduino Pin 3 -> SIM800L RX (Use Voltage Divider!)
SoftwareSerial SerialAT(4, 3); 
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);

void setup() {
  SerialMon.begin(9600);
  delay(10);
  SerialAT.begin(9600);
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
  String imei = modem.getIMEI();
  SerialMon.print("IMEI: ");
  SerialMon.println(imei);

  String operatorName = modem.getOperator();
  SerialMon.print("Operator: ");
  SerialMon.println(operatorName);

  int signalQuality = modem.getSignalQuality();
  SerialMon.print("Signal Quality (0-31): ");
  SerialMon.println(signalQuality);
}

void loop() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 5000) { // every 5 seconds
    int signal = modem.getSignalQuality();
    SerialMon.print("Signal Quality (0-31): ");
    SerialMon.println(signal);
    if (!modem.isNetworkConnected()) {
      SerialMon.println("Network disconnected!");
    } else {
      SerialMon.println("Network OK");
    }

    lastPrint = millis();
  }

  delay(100);
}
