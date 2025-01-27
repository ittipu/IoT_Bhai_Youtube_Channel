#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL6Gdgl6gPW"
#define BLYNK_TEMPLATE_NAME "MyGPS"
#define BLYNK_AUTH_TOKEN "Cj_7JmB6-VsxQz0o1GSME3wGUr_hFdCx"

#define TINY_GSM_MODEM_SIM800
#define SerialMon Serial
#define SerialAT Serial1
#define SerialGPS Serial2
#define TINY_GSM_DEBUG SerialMon
#define GSM_PIN ""


#include <TinyGsmClient.h>
#include <BlynkSimpleTinyGSM.h>
#include <TinyGPS++.h>

// ESP32 and SIM800l pins
#define MODEM_TX 17
#define MODEM_RX 18

// ESP32 and GPS pins
#define GPS_TX_PIN 15
#define GPS_RX_PIN 0

#define UPDATE_INTERVAL 5000L

char apn[] = "Internet";
char user[] = "";
char pass[] = "";

TinyGsm modem(SerialAT);
TinyGPSPlus gps;
BlynkTimer timer;


void setup() {
  SerialMon.begin(115200);
  delay(1000);
  SerialMon.println("Wait ...");
  SerialAT.begin(115200, SERIAL_8N1, MODEM_TX, MODEM_RX);
  SerialGPS.begin(9600, SERIAL_8N1, GPS_TX_PIN, GPS_RX_PIN);
  delay(3000);
  SerialMon.println("Initializing modem ...");
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

  Blynk.begin(BLYNK_AUTH_TOKEN, modem, apn, user, pass);
  timer.setInterval(UPDATE_INTERVAL, myTimerEvent);
}


BLYNK_WRITE(V2) {
  int pinValue = param.asInt();  // assigning incoming value from pin V1 to a variable
  Serial.print("Incomming Message: ");
  Serial.println(pinValue);

  // process received value
}

void myTimerEvent() {
  Serial.println("Getting data: ");
  float lat = 0, lng = 0;
  char lat_str[12];
  char lng_str[12];
  for (int i = 2; i; i--) {
    while (SerialGPS.available() > 0) {
      gps.encode(SerialGPS.read());
    }
    delay(1000);
  }
  if (gps.location.isValid()) {
    lat = gps.location.lat();
    lng = gps.location.lng();
  } else {
    Serial.println(F("Invalid"));
    lat = 0;
    lng = 0;
  }
  dtostrf(lat, 8, 6, lat_str);
  dtostrf(lng, 8, 6, lng_str);
  
  Serial.print("Latitude = ");
  Serial.println(lat_str);
  Serial.print("Longitude= ");
  Serial.println(lng_str);
  Blynk.virtualWrite(V0, lat_str);
  Blynk.virtualWrite(V1, lng_str);
}

void loop() {
  Blynk.run();
  timer.run();
}