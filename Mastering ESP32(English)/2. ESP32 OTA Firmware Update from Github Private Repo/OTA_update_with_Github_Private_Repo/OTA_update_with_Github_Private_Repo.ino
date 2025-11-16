#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include "secrets.h"  // Contains your github_pat token

const char* ssid = "";
const char* password = "";

// --- GitHub Repo Details ---
const char* github_owner = "ittipu";
const char* github_repo = "esp32_ota_update_private_repo";
const char* firmware_asset_name = "OTA_update_with_Github_Private_Repo.ino.bin";

// --- Current Firmware Version ---
const char* currentFirmwareVersion = "1.0.0";

// --- Update Check Timer ---
unsigned long lastUpdateCheck = 0;
const long updateCheckInterval = 1 * 60 * 1000;  // 5 minutes in milliseconds

// =================================================================================
// SETUP: Runs once at boot.
// =================================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nBooting up...");
  Serial.println("Current Firmware Version: " + String(currentFirmwareVersion));
  connectToWiFi();
  delay(6000);
  checkForFirmwareUpdate();
}

// =================================================================================
// LOOP: Runs continuously. This is the heart of the application.
// =================================================================================
void loop() {
}

// =================================================================================
// HELPER FUNCTIONS
// =================================================================================

void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nFailed to connect to WiFi. Will retry later.");
  }
}



void checkForFirmwareUpdate() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Skipping update check.");
    return;
  }

  String apiUrl = "https://api.github.com/repos/" + String(github_owner) + "/" + String(github_repo) + "/releases/latest";

  Serial.println("---------------------------------");
  Serial.println("Checking for new firmware...");
  Serial.println("Fetching release info from: " + apiUrl);

  HTTPClient http;
  http.begin(apiUrl);  
  http.addHeader("Authorization", "token " + String(github_pat));
  http.addHeader("Accept", "application/vnd.github.v3+json");
  http.setUserAgent("ESP32-OTA-Client");

  Serial.println("Sending API request...");
  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("API request failed. HTTP code: %d\n", httpCode);
    Serial.println("Full response: " + http.getString());  // Print error
    http.end();
    return;
  }
  Serial.printf("API request successful (HTTP %d). Parsing JSON.\n", httpCode);
  StaticJsonDocument<4096> doc;
  DeserializationError error = deserializeJson(doc, http.getStream());
  http.end();

  if (error) {
    Serial.println("Failed to parse JSON: " + String(error.c_str()));
    return;
  }

  String latestVersion = doc["tag_name"].as<String>();
  if (latestVersion.isEmpty() || latestVersion == "null") {
    Serial.println("Could not find 'tag_name' in JSON response.");
    return;
  }
  Serial.println("Current Version: " + String(currentFirmwareVersion));
  Serial.println("Latest Version:  " + latestVersion);

  if (latestVersion != currentFirmwareVersion) {
    Serial.println(">>> New firmware available! <<<");
    Serial.println("Searching for asset named: " + String(firmware_asset_name));
    String firmwareUrl = "";
    JsonArray assets = doc["assets"].as<JsonArray>();

    for (JsonObject asset : assets) {
      String assetName = asset["name"].as<String>();
      Serial.println("Found asset: " + assetName);

      if (assetName == String(firmware_asset_name)) {
        String assetId = asset["id"].as<String>();
        firmwareUrl = "https://api.github.com/repos/" + String(github_owner) + "/" + String(github_repo) + "/releases/assets/" + assetId;
        Serial.println("Found matching asset! Preparing to download.");
        break;
      }
    }

    if (firmwareUrl.isEmpty()) {
      Serial.println("Error: Could not find the specified firmware asset in the release.");
      return;
    }
    downloadAndApplyFirmware(firmwareUrl);

  } else {
    Serial.println("Device is up to date. No update needed.");
  }
  Serial.println("---------------------------------");
}

void downloadAndApplyFirmware(String url) {
  HTTPClient http;
  Serial.println("Starting firmware download from: " + url);

  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setUserAgent("ESP32-OTA-Client");
  http.begin(url);
  http.addHeader("Accept", "application/octet-stream");
  http.addHeader("Authorization", "token " + String(github_pat));

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Download failed, HTTP code: %d\n", httpCode);
    http.end();
    return;
  }

  int contentLength = http.getSize();
  if (contentLength <= 0) {
    Serial.println("Error: Invalid content length.");
    http.end();
    return;
  }

  // Begin the OTA update
  if (!Update.begin(contentLength)) {
    Serial.printf("Update begin failed: %s\n", Update.errorString());
    http.end();
    return;
  }
  Serial.println("Writing firmware... (this may take a moment)");
  WiFiClient* stream = http.getStreamPtr();
  uint8_t buff[1024];  
  size_t totalWritten = 0;
  int lastProgress = -1;

  while (totalWritten < contentLength) {
    int available = stream->available();
    if (available > 0) {
      int readLen = stream->read(buff, min((size_t)available, sizeof(buff)));
      if (readLen < 0) {
        Serial.println("Error reading from stream");
        Update.abort();
        http.end();
        return;
      }

      if (Update.write(buff, readLen) != readLen) {
        Serial.printf("Error: Update.write failed: %s\n", Update.errorString());
        Update.abort();
        http.end();
        return;
      }

      totalWritten += readLen;
      int progress = (int)((totalWritten * 100L) / contentLength);
      if (progress > lastProgress && (progress % 5 == 0 || progress == 100)) {
        Serial.printf("Progress: %d%%", progress);
        Serial.println();
        if (progress == 100) {
          Serial.println(); 
        } else {
          Serial.print("\r"); 
        }
        lastProgress = progress;
      }
    }
    delay(1);
  }
  Serial.println();

  if (totalWritten != contentLength) {
    Serial.printf("Error: Write incomplete. Wrote %d of %d bytes\n", totalWritten, contentLength);
    Update.abort();
  } else if (!Update.end()) {  // Finalize the update
    Serial.printf("Error: Update end failed: %s\n", Update.errorString());
  } else {
    Serial.println("Update complete! Restarting...");
    delay(1000);
    ESP.restart();
  }
  http.end();
}