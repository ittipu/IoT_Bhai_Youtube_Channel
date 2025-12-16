#define TINY_GSM_MODEM_SIM800
#define SerialMon Serial
#define SerialAT Serial1
#define TINY_GSM_DEBUG SerialMon
#define GSM_PIN ""

// Increase internal library buffer to prevent data loss
#define TINY_GSM_RX_BUFFER 1024

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <Update.h>
#include "config.h"

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
HttpClient http(client, server, port);

void performOTA();
void checkAndPerformUpdate();

void setup() {
  SerialMon.begin(115200);
  delay(100);
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);

  pinMode(MODEM_RST, OUTPUT);
  digitalWrite(MODEM_RST, HIGH);
  delay(100);  // Keep low for 100ms
  digitalWrite(MODEM_RST, LOW);
  delay(100);
  digitalWrite(MODEM_RST, HIGH);
  delay(1000);

  SerialMon.println("Wait ...");
  // SIM800L standard baud is usually 9600 for stability on ESP32
  SerialAT.begin(9600, SERIAL_8N1, MODEM_TX, MODEM_RX);
  delay(3000);

  SerialMon.println("Initializing modem...");
  modem.restart();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork(60000L)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  SerialMon.print("Connecting to APN: ");
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
    ESP.restart();
  }
  SerialMon.println(" OK");

  // Check for updates on boot
  checkAndPerformUpdate();
}

void loop() {
  delay(1800000);  // Check every 30 mins
  checkAndPerformUpdate();
}


void checkAndPerformUpdate() {
  if (!modem.isGprsConnected()) {
    SerialMon.println("GPRS disconnected, reconnecting...");
    modem.gprsConnect(apn, gprsUser, gprsPass);
  }

  SerialMon.println("Checking version...");

  // 1. Get Version Text
  http.stop();                         // Clean socket
  http.setHttpResponseTimeout(15000);  // 15s timeout

  int err = http.get(pathVersion);
  if (err != 0) {
    SerialMon.print("Connect failed: ");
    SerialMon.println(err);
    http.stop();
    return;
  }

  int statusCode = http.responseStatusCode();
  if (statusCode != 200) {
    SerialMon.print("Server error: ");
    SerialMon.println(statusCode);
    http.stop();
    return;
  }

  String serverVersion = http.responseBody();
  serverVersion.trim();
  http.stop();  // Close socket before next step

  SerialMon.printf("Server: %s | Device: %s\n", serverVersion.c_str(), currentVersion);

  if (serverVersion.equals(currentVersion)) {
    SerialMon.println("System up to date.");
  } else {
    SerialMon.println("New firmware detected. Starting SIM800L OTA...");
    delay(2000);  // Let modem settle
    performOTA();
  }
}

// ----------------------------------------------------------
// SIM800L OTA FUNCTION (Streaming Method)
// ----------------------------------------------------------
void performOTA() {
  SerialMon.println("Downloading Firmware...");

  // 1. Prepare HTTP Client
  http.stop();
  http.setHttpResponseTimeout(60000);  // 60s timeout for 2G

  // 2. Start Request
  int err = http.get(pathFirmware);
  if (err != 0) {
    SerialMon.print("Download start failed: ");
    SerialMon.println(err);
    http.stop();
    return;
  }

  int statusCode = http.responseStatusCode();
  long contentLength = http.contentLength();

  SerialMon.printf("Status: %d | Size: %ld bytes\n", statusCode, contentLength);

  if (statusCode != 200 || contentLength <= 0) {
    SerialMon.println("Invalid response.");
    http.stop();
    return;
  }

  // 3. Begin Update
  if (!Update.begin(contentLength)) {
    SerialMon.println("Not enough space for OTA");
    http.stop();
    return;
  }

  SerialMon.println("Writing to flash...");

  // 4. Stream Loop
  uint8_t buff[256];  // 256 bytes is safe for SIM800L
  size_t written = 0;
  size_t totalWritten = 0;
  size_t lastPrinted = 0;  // Track when we last printed to console
  unsigned long timeout = millis();

  while (http.connected() && (totalWritten < contentLength)) {
    if (http.available()) {
      // Read small chunk
      int len = http.read(buff, sizeof(buff));
      if (len > 0) {
        written = Update.write(buff, len);
        totalWritten += written;
        timeout = millis();  // Reset timeout on data arrival

        // --- NEW DISPLAY LOGIC ---
        // Print progress every 1024 bytes (1KB) or at the very end
        if ((totalWritten - lastPrinted >= 1024) || (totalWritten == contentLength)) {
          SerialMon.print(totalWritten);
          SerialMon.print(" | ");
          SerialMon.println(contentLength);
          lastPrinted = totalWritten;
        }
        // -------------------------
      }
    } else {
      delay(10);  // Wait for slow 2G data
    }

    // Watchdog: 60s timeout
    if (millis() - timeout > 60000) {
      SerialMon.println("\nTimeout: Data stopped flowing.");
      Update.abort();
      http.stop();
      return;
    }
  }

  // 5. Finalize
  if (totalWritten == contentLength && Update.end(true)) {
    SerialMon.println("\nOTA Success! Rebooting...");
    delay(1000);
    ESP.restart();
  } else {
    SerialMon.printf("\nError: Written %d / %d bytes\n", totalWritten, contentLength);
    Update.abort();
  }

  http.stop();
}