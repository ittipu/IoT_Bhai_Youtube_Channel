#include <TinyGPS++.h>


// GPS setup
#define GPS_TX_PIN 27
#define GPS_RX_PIN 26

TinyGPSPlus gps;
unsigned long timestamp;
float lat = 0;
float lng = 0;

// Setup the Serial monitor and GPS
void setup() {
  Serial.begin(115200);
  Serial.println("Wait ...");

  // GPS Serial port
  Serial1.begin(9600, SERIAL_8N1, GPS_TX_PIN, GPS_RX_PIN);
}

// Main loop to get GPS data
void loop() {
  get_gps_data();
  delay(5000);  // Wait for 5 seconds before getting new data
}

// Function to get GPS data
void get_gps_data() {
  Serial.println("Getting GPS data: ");
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }
  // Check if GPS location is valid
  if (gps.location.isValid()) {
    lat = gps.location.lat();
    lng = gps.location.lng();
    Serial.print("Latitude = ");
    Serial.println(lat, 6);
    Serial.print("Longitude = ");
    Serial.println(lng, 6);
  } else {
    Serial.println(F("Invalid GPS data"));
  }
  Serial.println();
}
