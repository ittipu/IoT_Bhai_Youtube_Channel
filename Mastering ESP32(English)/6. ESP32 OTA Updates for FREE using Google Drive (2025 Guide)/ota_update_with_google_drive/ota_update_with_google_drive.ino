#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>

// --- YOUR CREDENTIALS ---
const char* ssid = "IoT";
const char* password = "tipu1234@";

const char* currentFirmwareVersion = "1.0.1";

const char* firmwareUrl = "https://drive.google.com/uc?export=download&id=1B_R1mwUwoJGR5JxRzpSR75u7bUwrTzuH";
const char* versionUrl = "https://docs.google.com/document/d/1h3gwOIR0EKs_LWNwswSupTjut7S1My2WrZTYzmVuSug/export?format=txt";


void setup() {
  Serial.begin(115200);

  // 1. Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");

  // 2. Fetch the file
  if (WiFi.status() == WL_CONNECTED) {
    checkForFirmwareUpdate();
  }
}

void loop() {
  // Nothing here
}

void checkForFirmwareUpdate() {
  Serial.println(F("--- Boot Firmware Check ---"));

  // Check RAM
  if (ESP.getFreeHeap() < 30000) {
    Serial.println(F("⚠️ Low RAM. Skipping check to prevent crash."));
    return;
  }

  String latestVersion = fetchLatestVersion();

  if (latestVersion == "") {
    Serial.println(F("Could not fetch version."));
    return;
  }

  String cleanVersion = "";
  for (int i = 0; i < latestVersion.length(); i++) {
    if (isdigit(latestVersion[i]) || latestVersion[i] == '.') {
      cleanVersion += latestVersion[i];
    }
  }
  latestVersion = cleanVersion;

  if (latestVersion != currentFirmwareVersion) {
    Serial.println("New firmware found: " + latestVersion);
    downloadAndApplyFirmware(latestVersion);
  } else {
    String msg = "Firmware is up to date: " + String(currentFirmwareVersion);
    Serial.println(msg);
  }
}

String fetchLatestVersion() {
  String latestVersion = "";

  // Check if we have enough RAM to even try HTTPS (approx 20kb needed)
  if (ESP.getFreeHeap() < 20000) {
    Serial.println(F("❌ Not enough RAM for HTTPS check!"));
    return "";
  }

  // Use 'new' to allocate client on Heap, not Stack
  WiFiClientSecure* client = new WiFiClientSecure;
  if (!client) {
    Serial.println(F("Failed to allocate WiFiClientSecure"));
    return "";
  }

  client->setInsecure();  // Ignore certs
  client->setTimeout(10000);

  HTTPClient http;

  if (http.begin(*client, versionUrl)) {
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      latestVersion = http.getString();
      latestVersion.trim();  // Clean it immediately
    } else {
      // Remember: Do not use F() inside printf/log_printf
      Serial.printf("[HTTP] Version Check Failed, code: %d\n", httpCode);
    }
    http.end();
  } else {
    Serial.println(F("[HTTP] Unable to connect to version URL"));
  }

  // Clean up memory
  delete client;
  return latestVersion;
}

void downloadAndApplyFirmware(String newVersion) {
  Serial.println(F("--- Preparing Download ---"));

  WiFi.setTxPower(WIFI_POWER_11dBm);
  WiFiClientSecure* client = new WiFiClientSecure;
  if (!client) {
    Serial.println(F("❌ Could not allocate Secure Client"));
    return;
  }

  client->setInsecure();
  client->setTimeout(30000);

  HTTPClient http;
  http.setUserAgent("ESP32-OTA");

  if (http.begin(*client, firmwareUrl)) {
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      int contentLength = http.getSize();

      String sizeKB = String(contentLength / 1024.0, 1);
      String msg = "Starting Update... | Ver: " + newVersion + " | Size: " + sizeKB + " KB";
      Serial.println(msg);

      if (contentLength > 0) {
        if (Update.begin(contentLength)) {
          Serial.println(F("Writing to Flash..."));

          WiFiClient* stream = http.getStreamPtr();
          uint8_t buff[1280];
          size_t written = 0;
          int prevProgress = -1;
          int lastMqttProgress = -20;

          while (http.connected() && (written < contentLength)) {
            size_t size = stream->available();
            if (size) {
              int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
              Update.write(buff, c);
              written += c;

              int progress = (written * 100) / contentLength;

              if (progress != prevProgress) {
                // Using printf for formatting, no F() here
                Serial.printf("Progress: %d%%\n", progress);
                prevProgress = progress;

                if ((progress - lastMqttProgress) >= 20 || progress == 100) {
                  String pMsg = "Update Progress: " + String(progress) + "%";
                  lastMqttProgress = progress;
                }
              }
            }
            yield();
          }
          Serial.println();

          if (written == contentLength) {
            if (Update.end() && Update.isFinished()) {
              Serial.println(F("✅ Update Success! Rebooting..."));
              delete client;
              ESP.restart();
            }
          } else {
            Serial.printf("Error: Written only %d/%d\n", written, contentLength);
          }
        } else {
          Serial.println(F("Error: Not enough space for OTA"));
        }
      }
    } else {
      Serial.printf("HTTP Failed: %d\n", httpCode);
    }
    http.end();
  } else {
    Serial.println(F("Error: Connection failed"));
  }

  delete client;
}
