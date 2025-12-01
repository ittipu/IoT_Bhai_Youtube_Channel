#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "WebPageHandler.h"

Preferences prefs;
WebServer* server = nullptr;
WebPageHandler* pageHandler = nullptr;


#define AP_NAME "test_device"
#define AP_PASS "12345678"
#define HOLD_TIME 10000  // 10 seconds
#define BUTTON_PIN 23

const unsigned long CONFIG_PORTAL_TIMEOUT = 120000;  // Timeout: 2 minutes
volatile bool buttonPressed = false;
static bool messageShown = false;
unsigned long buttonPressStart = 0;


String name, email, password;
bool configRunning = false;

void IRAM_ATTR handleButtonInterrupt() {
  if (!buttonPressed) {
    buttonPressed = true;
    buttonPressStart = millis();  // Capture start time
  }
}

void runConfigPortal() {
  configRunning = true;
  WiFi.disconnect(true);
  delay(100);
  WiFi.softAPdisconnect(true);
  delay(100);
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_AP);

  WiFi.softAP(AP_NAME, AP_PASS);
  Serial.println("AP Mode SSID: " + String(AP_NAME));
  Serial.println("AP IP Address: " + WiFi.softAPIP().toString());

  if (server) delete server;
  if (pageHandler) delete pageHandler;

  server = new WebServer(80);
  pageHandler = new WebPageHandler(*server);
  pageHandler->begin();

  unsigned long startTime = millis();
  while (millis() - startTime < CONFIG_PORTAL_TIMEOUT) {
    server->handleClient();
    delay(1);

    if (pageHandler->isConfigDone()) {
      Serial.println("Configuration completed. Exiting portal.");
      break;
    }
  }

  server->stop();
  WiFi.disconnect(true);
  delay(100);
  WiFi.softAPdisconnect(true);
  delay(100);
  WiFi.mode(WIFI_OFF);
  delay(100);
  Serial.println("Portal closed.");

  prefs.begin("device_prefs", true);
  name = prefs.getString("device_name", "N/A");
  email = prefs.getString("email", "N/A");
  password = prefs.getString("password", "N/A");
  prefs.end();

  Serial.println("New data:");
  Serial.println("Device Name: " + name);
  Serial.println("Email: " + email);
  Serial.println("Password: " + password);
  Serial.println("---------------------------");

  configRunning = false;
}


void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonInterrupt, FALLING);
  prefs.begin("device_prefs", true);
  name = prefs.getString("device_name", "N/A");
  email = prefs.getString("email", "N/A");
  password = prefs.getString("password", "N/A");

  Serial.println("From Preferences:");
  Serial.println("Device Name: " + name);
  Serial.println("Email: " + email);
  Serial.println("Password: " + password);
  Serial.println("---------------------------");
  delay(2000);
  if (name == "N/A" || email == "N/A" || password == "N/A") {
    Serial.println("No valid config found. Launching config portal...");
    runConfigPortal();
  }
}

void loop() {
  if (buttonPressed) {
    if (digitalRead(BUTTON_PIN) == LOW) {
      if (!messageShown) {
        Serial.println("Button pressed. Hold to confirm...");
        messageShown = true;
      }

      if (millis() - buttonPressStart >= HOLD_TIME) {
        Serial.println("Button held for 10s — starting config portal...");
        buttonPressed = false;
        messageShown = false;
        runConfigPortal();
      }
    } else {
      buttonPressed = false;
      messageShown = false;
    }
  }
}
