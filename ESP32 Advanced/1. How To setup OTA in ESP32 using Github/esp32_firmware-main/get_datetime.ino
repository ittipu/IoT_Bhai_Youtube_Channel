#include <WiFi.h>
#include "time.h"

// Replace with your network credentials
const char* ssid = "IoT";
const char* password = "tipu1234@";
const char* ntpServer = "pool.ntp.org";


String datetime;

void setup() {
  Serial.begin(115200);
  initWiFi();
  configTime(0, 0, ntpServer);
}

void loop() {
  datetime = getFormattedTime();
  Serial.print("Date time: ");
  Serial.println(datetime);
  delay(1000);
}

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

String getFormattedTime() {
  time_t now;
  struct tm timeinfo;
  const int timezoneOffset = 6 * 3600;  // UTC+6 offset in seconds

  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "00-00-0000 00:00:00";
  }
  now = mktime(&timeinfo) + timezoneOffset;  // Adjust for timezone
  localtime_r(&now, &timeinfo);              // Convert adjusted time back to tm structure

  char buffer[20];
  strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", &timeinfo);

  return String(buffer);
}