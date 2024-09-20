#include <WiFi.h>
#include <SPIFFS.h>
#include "Update.h"
#include <WiFiClientSecure.h>

// Define WiFi credentials
#define ssid "IoT"
#define password "tipu1234@"

// Define server details and file path
#define HOST "raw.githubusercontent.com"
#define PATH "/ittipu/esp32_firmware/main/get_datetime.ino.bin"
#define PORT 443



// Define the name for the downloaded firmware file
#define FILE_NAME "firmware.bin"

// Known file size (in bytes) for progress calculation
#define KNOWN_FILE_SIZE 997712 // Update this with the actual file size

void setup() {
  Serial.begin(115200);

  // Initialize SPIFFS and format if necessary
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed, formatting...");
    if (!SPIFFS.format()) {
      Serial.println("SPIFFS format failed!");
      return;
    }
    Serial.println("SPIFFS formatted successfully.");
    // Try to mount SPIFFS again after formatting
    if (!SPIFFS.begin(true)) {
      Serial.println("SPIFFS Mount Failed after formatting");
      return;
    }
  }

  connectToWiFi();
  getFileFromServer();
  performOTAUpdateFromSPIFFS();
}

void loop() {
  // Nothing to do here
}

void connectToWiFi() {
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("WiFi connected");
}

void getFileFromServer() {
  WiFiClientSecure client;
  client.setInsecure(); // Allow insecure connections

  if (client.connect(HOST, PORT)) {
    Serial.println("Connected to server");
    client.print("GET " + String(PATH) + " HTTP/1.1\r\n");
    client.print("Host: " + String(HOST) + "\r\n");
    client.println("Connection: close\r\n");
    client.println();

    File file = SPIFFS.open("/" + String(FILE_NAME), FILE_WRITE);
    if (!file) {
      Serial.println("Failed to open file for writing");
      return;
    }

    bool endOfHeaders = false;
    String headers = "";
    String http_response_code = "error";

    const size_t bufferSize = 4096;
    uint8_t buffer[bufferSize];
    size_t totalBytesRead = 0;

    // Read HTTP response headers
    while (client.connected() && !endOfHeaders) {
      if (client.available()) {
        char c = client.read();
        headers += c;
        if (headers.startsWith("HTTP/1.1")) {
          http_response_code = headers.substring(9, 12);
        }
        if (headers.endsWith("\r\n\r\n")) {
          endOfHeaders = true;
        }
      }
    }

    Serial.println("HTTP response code: " + http_response_code);
    Serial.println("Content-Length header missing or zero. Using known file size.");

    // Read and write file content
    while (client.connected() || client.available()) {
      if (client.available()) {
        size_t bytesRead = client.readBytes(buffer, bufferSize);
        if (bytesRead > 0) {
          file.write(buffer, bytesRead);
          totalBytesRead += bytesRead;

          float progress = (float)totalBytesRead / KNOWN_FILE_SIZE * 100.0;
          Serial.print("Download progress: ");
          Serial.print(progress);
          Serial.println("%");
        }
      }
    }

    file.close();
    client.stop();
    Serial.println("File saved successfully");
  } else {
    Serial.println("Failed to connect to server");
  }
}

void performOTAUpdateFromSPIFFS() {
  File file = SPIFFS.open("/" + String(FILE_NAME), FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.println("Starting update..");
  size_t fileSize = file.size();
  Serial.println("File size: " + String(fileSize));

  if (!Update.begin(fileSize, U_FLASH)) {
    Serial.println("Cannot do the update");
    return;
  }

  // Write firmware data from file to OTA update
  if (!Update.writeStream(file)) {
    Serial.println("Update write failed");
    return;
  }

  if (Update.end()) {
    Serial.println("Successful update");
  } else {
    Serial.println("Error Occurred: " + String(Update.getError()));
  }

  file.close();
  Serial.println("Reset in 4 seconds....");
  delay(4000);
  ESP.restart();
}