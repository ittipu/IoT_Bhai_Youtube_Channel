#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "WebPageHandler.h"

#define BUTTON_PIN 23
#define AP_NAME "ESP32_Device"
#define AP_PASS "12345678"
#define HOLD_TIME 3000         // 3 seconds hold to trigger config
#define WIFI_TIMEOUT 15000     // 15 seconds timeout for connecting to saved WiFi
#define CONFIG_TIMEOUT 180000  // 3 minutes timeout for the portal

Preferences prefs;
WebServer* server = nullptr;
WebPageHandler* pageHandler = nullptr;


volatile bool buttonPressed = false;
unsigned long buttonPressStart = 0;


void IRAM_ATTR handleButtonInterrupt() {
  if (!buttonPressed && digitalRead(BUTTON_PIN) == LOW) {
    buttonPressed = true;
    buttonPressStart = millis();
  }
}

void runConfigPortal() {
  Serial.println("\n[PORTAL] --------------------------------");
  Serial.println("[PORTAL] Starting Configuration Mode");
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_AP);
  Serial.println("[PORTAL] WiFi Mode: Access Point (AP)");

  IPAddress local_IP(192, 168, 0, 1);
  IPAddress gateway(192, 168, 0, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(AP_NAME, AP_PASS);
  Serial.print("[PORTAL] AP Created: ");
  Serial.println(AP_NAME);
  Serial.print("[PORTAL] AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  if (server) delete server;
  server = new WebServer(80);
  if (pageHandler) delete pageHandler;
  pageHandler = new WebPageHandler(*server);
  pageHandler->begin();
  Serial.println("[PORTAL] Web Server Started");

  unsigned long startTime = millis();
  Serial.println("[PORTAL] Waiting for clients to connect...");
  while (millis() - startTime < CONFIG_TIMEOUT) {
    server->handleClient();
    delay(2);
    if ((millis() - startTime) % 10000 < 5) {
    }
  }
  Serial.println("\n[PORTAL] Timeout reached (3 mins). Restarting device...");
  ESP.restart();
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonInterrupt, FALLING);
  Serial.println("\n\n[BOOT] System Starting...");
  prefs.begin("device_prefs", true);  // Read-only
  String ssid = prefs.getString("wifi_ssid", "");
  String pass = prefs.getString("wifi_pass", "");
  String name = prefs.getString("device_name", "N/A");
  String email = prefs.getString("email", "N/A");
  prefs.end();

  Serial.println("[BOOT] Preferences Loaded");
  Serial.println("--------------------------------");
  Serial.println("   ESP32 BOOT INFO SYSTEM       ");
  Serial.println("--------------------------------");
  Serial.println("Device Name : " + name);
  Serial.println("Saved SSID  : " + (ssid == "" ? "No SSID Saved" : ssid));
  Serial.println("Saved Pass  : " + String(pass == "" ? "No Pass Saved" : "********"));
  Serial.println("--------------------------------");

  if (ssid != "") {
    Serial.print("[WIFI] Connecting to: ");
    Serial.println(ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    unsigned long startAttempt = millis();
    bool connected = false;
    while (millis() - startAttempt < WIFI_TIMEOUT) {
      if (WiFi.status() == WL_CONNECTED) {
        connected = true;
        break;
      }
      delay(500);
      Serial.print(".");
      if (buttonPressed && (millis() - buttonPressStart >= HOLD_TIME)) {
        buttonPressed = false;
        Serial.println("\n[INPUT] Button Interrupt during boot -> Launching Portal");
        runConfigPortal();
        return;
      }
    }

    Serial.println();
    if (connected) {
      Serial.println("[WIFI] Success! Connected.");
      Serial.print("[WIFI] IP Address: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("[WIFI] Connection Failed (Timeout/Wrong Password).");
      Serial.println("[WIFI] Launching Config Portal...");
      runConfigPortal();
    }
  } else {
    Serial.println("[BOOT] No Credentials found. Launching Config Portal...");
    runConfigPortal();
  }
}

void loop() {
  if (buttonPressed) {
    if (digitalRead(BUTTON_PIN) == LOW) {
      unsigned long holdDuration = millis() - buttonPressStart;
      static unsigned long lastPrint = 0;
      if (holdDuration > 1000 && millis() - lastPrint > 1000) {
        Serial.print("[INPUT] Holding button... ");
        Serial.print(holdDuration / 1000);
        Serial.println("s");
        lastPrint = millis();
      }

      if (holdDuration >= HOLD_TIME) {
        Serial.println("[INPUT] Hold time reached -> Starting Config Portal");
        buttonPressed = false;
        runConfigPortal();  // This will now block until timeout or restart
      }
    } else {
      if (millis() - buttonPressStart > 100) {  // filter noise
        Serial.println("[INPUT] Button released early.");
      }
      buttonPressed = false;
    }
  }
}