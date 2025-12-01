#include "WebPageHandler.h"
#include <WiFi.h>

const char* WebPageHandler::htmlForm = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Config</title>
  <style>
    body {
      font-family: 'Segoe UI', sans-serif;
      background: linear-gradient(to right, #83a4d4, #b6fbff);
      margin: 0;
      padding: 0;
      display: flex;
      align-items: center;
      justify-content: center;
      height: 100vh;
    }
    .container {
      background: white;
      padding: 24px;
      border-radius: 12px;
      box-shadow: 0 4px 12px rgba(0,0,0,0.15);
      width: 90%;
      max-width: 400px;
    }
    h2 {
      text-align: center;
      color: #333;
    }
    label {
      display: block;
      margin-top: 15px;
      font-weight: bold;
    }
    input[type="text"],
    input[type="email"],
    input[type="password"] {
      width: 100%;
      padding: 10px;
      margin-top: 6px;
      border: 1px solid #ccc;
      border-radius: 6px;
      box-sizing: border-box;
    }
    input[type="submit"] {
      width: 100%;
      margin-top: 20px;
      padding: 12px;
      background-color: #0078D7;
      border: none;
      color: white;
      font-size: 16px;
      border-radius: 6px;
      cursor: pointer;
      transition: background 0.3s;
    }
    input[type="submit"]:hover {
      background-color: #005bb5;
    }
    @media (max-width: 600px) {
      .container {
        margin: 10px;
        padding: 20px;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <h2>Device Configuration</h2>
    <form action="/submit" method="POST">
      <label for="device_name">Device Name</label>
      <input type="text" id="device_name" name="device_name" required>

      <label for="email">Email</label>
      <input type="email" id="email" name="email" required>

      <label for="password">Password</label>
      <input type="password" id="password" name="password" required>

      <input type="submit" value="Save Settings">
    </form>
  </div>
</body>
</html>
)rawliteral";

WebPageHandler::WebPageHandler(WebServer& server) : _server(server) {}

void WebPageHandler::begin() {
    _prefs.begin("device_prefs", false);

    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/submit", HTTP_POST, [this]() { handleSubmit(); });
    _server.begin();
}

void WebPageHandler::handleRoot() {
    _server.send(200, "text/html", htmlForm);
}

void WebPageHandler::handleSubmit() {
    String deviceName = _server.arg("device_name");
    String email = _server.arg("email");
    String password = _server.arg("password");

    _prefs.putString("device_name", deviceName);
    _prefs.putString("email", email);
    _prefs.putString("password", password);

    _server.send(200, "text/html", buildSuccessPage());
    delay(2000);
    configDone = true; 
}

String WebPageHandler::buildSuccessPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Settings Saved</title>
  <style>
    body {
      font-family: 'Segoe UI', sans-serif;
      background: linear-gradient(to right, #83a4d4, #b6fbff);
      margin: 0;
      padding: 0;
      display: flex;
      align-items: center;
      justify-content: center;
      height: 100vh;
    }
    .container {
      background: white;
      padding: 24px;
      border-radius: 12px;
      box-shadow: 0 4px 12px rgba(0,0,0,0.15);
      width: 90%;
      max-width: 400px;
      text-align: center;
    }
    h2 {
      color: #333;
    }
  </style>
</head>
<body>
  <div class="container">
    <h2>Settings Saved Successfully!</h2>
    <p>You can now restart the device or close this window.</p>
  </div>
</body>
</html>
)rawliteral";
}

bool WebPageHandler::isConfigDone() { return configDone; } // in .cpp
