#include "WebPageHandler.h"
#include <WiFi.h>

WebPageHandler::WebPageHandler(WebServer& server) : _server(server) {}

void WebPageHandler::begin() {
  _prefs.begin("device_prefs", false);
  _server.on("/", HTTP_GET, [this]() { handleRoot(); });
  _server.on("/scan", HTTP_GET, [this]() { handleScan(); });
  _server.on("/save", HTTP_POST, [this]() { handleSave(); });
  _server.on("/status", HTTP_GET, [this]() { handleGetStatus(); });
  _server.on("/check_connect", HTTP_GET, [this]() { handleCheckConnect(); });

  _server.begin();
}

void WebPageHandler::handleRoot() {
  Serial.println("[WEB] Client accessed Root page (/)");
  _server.send(200, "text/html", indexHtml);
}

void WebPageHandler::handleGetStatus() {
  String savedSSID = _prefs.getString("wifi_ssid", "None");
  String json = "{\"ssid\":\"" + savedSSID + "\"}";
  _server.send(200, "application/json", json);
}

void WebPageHandler::handleScan() {
  Serial.println("[WEB] Scan Requested. Scanning networks...");
  int n = WiFi.scanNetworks();
  Serial.print("[WEB] Scan Complete. Found: ");
  Serial.println(n);

  String json = "[";
  for (int i = 0; i < n; ++i) {
    if (i) json += ",";
    json += "{";
    json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
    json += "\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false");
    json += "}";
  }
  json += "]";
  _server.send(200, "application/json", json);
}

void WebPageHandler::handleSave() {
  if (_server.hasArg("ssid") && _server.hasArg("pass")) {
    String ssid = _server.arg("ssid");
    String pass = _server.arg("pass");
    String name = _server.arg("name");
    String email = _server.arg("email");

    Serial.println("[WEB] Save Requested.");
    Serial.println("[WEB] SSID: " + ssid);
    Serial.println("[WEB] Pass: ********");
    Serial.println("[WEB] Name: " + name);

    // 1. Temporarily save to RAM variables for testing
    // We don't save to Prefs yet until confirmed (optional, but safer to save so we don't lose it)
    _prefs.putString("wifi_ssid", ssid);
    _prefs.putString("wifi_pass", pass);
    _prefs.putString("device_name", name);
    _prefs.putString("email", email);
    
    // 2. Switch to Dual Mode (AP + Station)
    // This allows us to keep the user connected to AP while the ESP connects to Router
    Serial.println("[WEB] Switching to AP+STA mode to verify credentials...");
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    
    isTestingConnection = true;
    testStartTime = millis();

    _server.send(200, "application/json", "{\"status\":\"testing\"}");
  } else {
    Serial.println("[WEB] Save Error: Missing fields");
    _server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"Missing fields\"}");
  }
}

void WebPageHandler::handleCheckConnect() {
  if (!isTestingConnection) {
    _server.send(200, "application/json", "{\"status\":\"idle\"}");
    return;
  }

  // Check connection status
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WEB] Verification SUCCESS! Connected to router.");
    Serial.print("[WEB] New IP: ");
    Serial.println(WiFi.localIP());
    
    isTestingConnection = false;
    // Return Success JSON
    _server.send(200, "application/json", "{\"status\":\"connected\", \"ip\":\"" + WiFi.localIP().toString() + "\"}");
    
    Serial.println("[WEB] Sending Success response and Restarting...");
    delay(500);
    ESP.restart(); // Reboot to go into full Station mode cleanly
  } 
  else if (millis() - testStartTime > 15000) { // 15 seconds timeout
    Serial.println("[WEB] Verification FAILED. Timeout/Wrong Password.");
    isTestingConnection = false;
    WiFi.disconnect(); // Stop trying
    // WiFi.mode(WIFI_AP); // Optional: Revert to just AP
    _server.send(200, "application/json", "{\"status\":\"failed\"}");
  } 
  else {
    // Still trying
    _server.send(200, "application/json", "{\"status\":\"trying\"}");
  }
}

// --- Frontend Code ---
const char* WebPageHandler::indexHtml = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Device Configuration</title>
  <style>
    :root {
      --primary: #6366f1;
      --primary-hover: #4f46e5;
      --bg-gradient: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      --card-bg: rgba(255, 255, 255, 0.96);
      --text: #1f2937;
      --text-light: #6b7280;
    }
    
    * { box-sizing: border-box; margin: 0; padding: 0; }
    
    body { 
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      background: var(--bg-gradient); 
      display: flex; 
      align-items: center; 
      justify-content: center; 
      min-height: 100vh; 
      padding: 20px; 
      color: var(--text);
    }

    .container { 
      background: var(--card-bg); 
      width: 100%; 
      max-width: 400px; 
      border-radius: 20px; 
      padding: 32px; 
      box-shadow: 0 20px 25px -5px rgba(0, 0, 0, 0.1), 0 10px 10px -5px rgba(0, 0, 0, 0.04); 
      position: relative; 
      backdrop-filter: blur(10px);
    }

    h2 { 
      text-align: center; 
      color: #333; 
      margin-bottom: 24px; 
      font-weight: 800;
      font-size: 1.6rem;
      letter-spacing: -0.5px;
    }
    
    label { 
      display: block; 
      margin: 16px 0 6px; 
      font-weight: 600; 
      font-size: 0.9rem; 
      color: #4b5563;
    }

    .password-container {
      position: relative;
    }

    .toggle-password {
      position: absolute;
      right: 12px;
      top: 50%;
      transform: translateY(-50%);
      cursor: pointer;
      color: #6b7280;
      font-size: 0.9rem;
      user-select: none;
      background: none;
      border: none;
      padding: 0;
      margin: 0;
    }

    .toggle-password:hover {
      color: var(--primary);
    }
    
    input { 
      width: 100%; 
      padding: 12px 16px; 
      border: 2px solid #e5e7eb; 
      border-radius: 12px; 
      font-size: 1rem;
      transition: all 0.3s ease;
      background: #f9fafb;
    }

    input:focus {
      outline: none;
      border-color: var(--primary);
      background: white;
      box-shadow: 0 0 0 4px rgba(99, 102, 241, 0.15);
    }
    
    button { 
      width: 100%; 
      padding: 14px; 
      border: none; 
      border-radius: 12px; 
      font-weight: 700; 
      font-size: 1rem;
      cursor: pointer; 
      margin-top: 24px; 
      transition: all 0.2s ease;
      letter-spacing: 0.5px;
    }
    
    /* Improved Scan Button */
    .btn-scan { 
      background: white; 
      color: var(--primary); 
      border: 2px solid var(--primary);
      margin-top: 10px;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 10px;
    }
    
    .btn-scan svg {
      width: 20px;
      height: 20px;
      fill: currentColor;
      transition: transform 0.3s;
    }
    
    .btn-scan:hover { 
      background: #eef2ff; 
      transform: translateY(-2px);
      box-shadow: 0 4px 12px rgba(99, 102, 241, 0.1);
    }

    .btn-scan.scanning {
      background: #f3f4f6;
      border-color: #d1d5db;
      color: #6b7280;
      cursor: wait;
    }
    
    .btn-scan.scanning svg {
      animation: pulse 1s infinite;
    }

    @keyframes pulse {
      0% { opacity: 1; transform: scale(1); }
      50% { opacity: 0.5; transform: scale(0.9); }
      100% { opacity: 1; transform: scale(1); }
    }
    
    .btn-save { 
      background: var(--primary); 
      color: white; 
      box-shadow: 0 4px 6px -1px rgba(99, 102, 241, 0.3);
      text-transform: uppercase;
    }
    .btn-save:hover { 
      background: var(--primary-hover); 
      transform: translateY(-2px);
      box-shadow: 0 6px 12px -1px rgba(99, 102, 241, 0.4);
    }
    .btn-save:active { transform: translateY(0); }

    /* WiFi List */
    #wifi-list { 
      display: none; 
      margin-top: 15px; 
      border: 1px solid #e5e7eb; 
      border-radius: 12px; 
      max-height: 200px; 
      overflow-y: auto; 
      background: white;
      box-shadow: 0 4px 6px -1px rgba(0,0,0,0.05);
    }
    .wifi-item { 
      padding: 12px 16px; 
      border-bottom: 1px solid #f3f4f6; 
      cursor: pointer; 
      display: flex; 
      justify-content: space-between; 
      align-items: center;
      transition: background 0.2s;
    }
    .wifi-item:hover { background: #eff6ff; }
    .wifi-item:last-child { border-bottom: none; }
    .wifi-item span { font-weight: 600; color: #374151; }
    .wifi-item small { color: #9ca3af; }

    /* Overlay */
    .overlay { 
      position: absolute; 
      top: 0; left: 0; right: 0; bottom: 0; 
      background: rgba(255,255,255,0.98); 
      border-radius: 20px; 
      display: flex; 
      flex-direction: column; 
      align-items: center; 
      justify-content: center; 
      opacity: 0; 
      pointer-events: none; 
      transition: opacity 0.3s ease; 
      z-index: 10;
    }
    .overlay.active { opacity: 1; pointer-events: all; }
    
    .spinner { 
      width: 50px; 
      height: 50px; 
      border: 5px solid #e5e7eb; 
      border-top-color: var(--primary); 
      border-radius: 50%; 
      animation: spin 1s linear infinite; 
      margin-bottom: 24px; 
    }
    @keyframes spin { 100% { transform: rotate(360deg); } }
    
    .msg-box {
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      text-align: center;
      width: 100%;
    }

    .msg-box h3 { margin: 10px 0 5px; color: #111; font-size: 1.25rem; }
    .msg-box p { color: #666; font-size: 1rem; }
    .error-text { color: #ef4444 !important; }
    .success-text { color: #10b981 !important; }
    
    .icon-large { font-size: 64px; margin-bottom: 16px; display: block; line-height: 1; }
    .icon-success { color: #10b981; }
    .icon-error { color: #ef4444; }

  </style>
</head>
<body>

  <div class="container">
    <!-- Overlay for status -->
    <div id="overlay" class="overlay">
      <div id="loading-ui">
        <div class="spinner"></div>
        <h3>Verifying Connection...</h3>
        <p>Please wait while we test the WiFi.</p>
      </div>
      
      <div id="success-ui" class="msg-box" style="display:none;">
        <span class="icon-large icon-success">✓</span>
        <h3 class="success-text">Connected!</h3>
        <p>Credentials correct. Rebooting...</p>
      </div>

      <div id="error-ui" class="msg-box" style="display:none;">
        <span class="icon-large icon-error">✕</span>
        <h3 class="error-text">Connection Failed</h3>
        <p>Could not connect. Check password.</p>
        <button onclick="closeOverlay()" style="background:#ef4444; color:white; margin-top:20px;">Try Again</button>
      </div>
    </div>

    <h2>Device Setup</h2>
    
    <button type="button" class="btn-scan" onclick="scanWifi()">
      <!-- WiFi Icon -->
      <svg viewBox="0 0 24 24"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-1 17.93c-3.95-.49-7-3.85-7-7.93 0-.62.08-1.21.21-1.79L9 15v1c0 1.1.9 2 2 2v1.93zm6.9-2.54c-.26-.81-1-1.39-1.9-1.39h-1v-3c0-.55-.45-1-1-1H8v-2h2c.55 0 1-.45 1-1V7h2c1.1 0 2-.9 2-2v-.41c2.93 1.19 5 4.06 5 7.41 0 2.08-1.07 3.97-2.74 5.27z"/></svg>
      <span>Scan for Networks</span>
    </button>
    <div id="wifi-list"></div>

    <form id="config-form" onsubmit="handleFormSubmit(event)">
      <label>WiFi SSID</label>
      <input type="text" id="ssid" name="ssid" required placeholder="Select or type SSID">
      
      <label>WiFi Password</label>
      <div class="password-container">
        <input type="password" id="pass" name="pass" placeholder="Enter WiFi Password">
        <span class="toggle-password" onclick="togglePassword()">Show</span>
      </div>
      
      <label>Device Name</label>
      <input type="text" name="name" placeholder="e.g. Living Room Lamp">
      
      <label>Email (Optional)</label>
      <input type="email" name="email" placeholder="admin@example.com">

      <button type="submit" class="btn-save">Save & Connect</button>
    </form>
  </div>

<script>
  function scanWifi() {
    const list = document.getElementById('wifi-list');
    const btn = document.querySelector('.btn-scan');
    const btnText = btn.querySelector('span');
    
    list.style.display = 'none';
    btnText.innerText = "Scanning...";
    btn.classList.add('scanning');
    btn.disabled = true;
    
    fetch('/scan').then(res => res.json()).then(data => {
      list.innerHTML = '';
      btnText.innerText = "Scan for Networks";
      btn.classList.remove('scanning');
      btn.disabled = false;
      
      list.style.display = 'block';
      data.forEach(net => {
        const div = document.createElement('div');
        div.className = 'wifi-item';
        div.innerHTML = `<span>${net.ssid}</span> <small>${net.rssi} dBm</small>`;
        div.onclick = () => { document.getElementById('ssid').value = net.ssid; list.style.display='none'; };
        list.appendChild(div);
      });
    })
    .catch(err => {
      btnText.innerText = "Scan Failed";
      btn.classList.remove('scanning');
      btn.disabled = false;
    });
  }

  function togglePassword() {
    const passInput = document.getElementById('pass');
    const toggleBtn = document.querySelector('.toggle-password');
    if (passInput.type === 'password') {
      passInput.type = 'text';
      toggleBtn.textContent = 'Hide';
    } else {
      passInput.type = 'password';
      toggleBtn.textContent = 'Show';
    }
  }

  function handleFormSubmit(e) {
    e.preventDefault();
    const overlay = document.getElementById('overlay');
    const loading = document.getElementById('loading-ui');
    const success = document.getElementById('success-ui');
    const error = document.getElementById('error-ui');
    
    // Reset UI
    overlay.classList.add('active');
    loading.style.display = 'block';
    success.style.display = 'none';
    error.style.display = 'none';

    // Submit Form
    const formData = new FormData(document.getElementById('config-form'));
    const params = new URLSearchParams(formData);

    fetch('/save', { method: 'POST', body: params })
      .then(res => res.json())
      .then(data => {
        if(data.status === 'testing') {
          // Start Polling
          pollStatus();
        }
      });
  }

  function pollStatus() {
    // Check every 1 second
    const interval = setInterval(() => {
      fetch('/check_connect')
        .then(res => res.json())
        .then(data => {
          if (data.status === 'connected') {
            clearInterval(interval);
            document.getElementById('loading-ui').style.display = 'none';
            document.getElementById('success-ui').style.display = 'block';
          } else if (data.status === 'failed') {
            clearInterval(interval);
            document.getElementById('loading-ui').style.display = 'none';
            document.getElementById('error-ui').style.display = 'block';
          }
        })
        .catch(err => {
           // If fetch fails (maybe ESP rebooted?), assume success or stop polling
           // clearInterval(interval);
        });
    }, 1000);
  }

  function closeOverlay() {
    document.getElementById('overlay').classList.remove('active');
  }
</script>
</body>
</html>
)rawliteral";