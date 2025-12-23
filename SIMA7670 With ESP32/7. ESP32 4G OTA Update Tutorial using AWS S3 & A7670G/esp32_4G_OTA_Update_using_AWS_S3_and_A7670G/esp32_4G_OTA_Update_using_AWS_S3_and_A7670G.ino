/**
 * AWS S3 OTA (FIXED FOR ERROR 715)
 */

#define TINY_GSM_MODEM_A7670
#define TINY_GSM_RX_BUFFER 1024

#define SerialMon Serial
#define SerialAT Serial1
#define TINY_GSM_DEBUG SerialMon
#define GSM_PIN ""

#include <TinyGsmClient.h>
#include <Update.h>

// === AWS CONFIGURATION ===
const char apn[] = "internet";
const char currentVersion[] = "1.0.0";

// Singapore S3 URLs
const char *URL_VERSION = "https://esp32-ota-update-storage.s3.ap-southeast-1.amazonaws.com/version.txt";
const char *URL_FIRMWARE = "https://esp32-ota-update-storage.s3.ap-southeast-1.amazonaws.com/esp32_4G_OTA_Update_using_AWS_S3_and_A7670G.ino.bin";


// === PINS ===
#define MODEM_RESET_PIN 5
#define MODEM_PWKEY 4
#define MODEM_POWER_ON 12
#define MODEM_TX 26
#define MODEM_RX 27
#define MODEM_RESET_LEVEL HIGH
#define BUILTIN_LED 2

TinyGsm modem(SerialAT);

// Forward Declarations
void configureAWS_SNI();
void performUpdate();
String getHttpsBody(size_t size);

void setup() {
  SerialMon.begin(115200);

  // Power ON
  pinMode(MODEM_POWER_ON, OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);
  digitalWrite(MODEM_POWER_ON, HIGH);

  // Reset
  pinMode(MODEM_RESET_PIN, OUTPUT);
  digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
  delay(100);
  digitalWrite(MODEM_RESET_PIN, MODEM_RESET_LEVEL);
  delay(2600);
  digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);

  // PwrKey
  pinMode(MODEM_PWKEY, OUTPUT);
  digitalWrite(MODEM_PWKEY, LOW);
  delay(100);
  digitalWrite(MODEM_PWKEY, HIGH);
  delay(1000);
  digitalWrite(MODEM_PWKEY, LOW);

  SerialMon.println("Wait ...");
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  SerialMon.println("Initializing modem...");
  if (!modem.init()) {
    SerialMon.println("Failed to restart modem");
    return;
  }

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    return;
  }
  SerialMon.println(" success");

  SerialMon.print("Connecting to APN: ");
  if (!modem.gprsConnect(apn)) {
    SerialMon.println(" fail");
    return;
  }
  SerialMon.println(" OK");

  // 1. SYNC TIME (Critical)
  SerialMon.println("Syncing Time...");
  modem.NTPServerSync("pool.ntp.org", 20);

  // 2. CONFIGURE AWS SNI (Updated to cover all contexts)
  configureAWS_SNI();

  // 3. CHECK VERSION
  SerialMon.println("Checking Version...");
  modem.https_begin();

  if (!modem.https_set_url(URL_VERSION)) {
    SerialMon.println("Failed to set URL");
  } else {
    size_t size = 0;
    int httpCode = modem.https_get(&size);

    if (httpCode == 200) {
      String serverVersion = getHttpsBody(size);
      serverVersion.trim();

      SerialMon.printf("Server: %s | Device: %s\n", serverVersion.c_str(), currentVersion);

      if (!serverVersion.equals(currentVersion) && serverVersion.length() > 0) {
        SerialMon.println("New Firmware Found! Starting OTA...");
        modem.https_end();
        performUpdate();
      } else {
        SerialMon.println("System Up to Date");
      }
    } else {
      SerialMon.printf("Version Check Failed. Code: %d\n", httpCode);
    }
  }
  modem.https_end();
}

void loop() {
}

String getHttpsBody(size_t contentSize) {
  String body = "";
  char buffer[128];
  int totalRead = 0;
  while (totalRead < contentSize) {
    int len = modem.https_body((uint8_t *)buffer, sizeof(buffer) - 1);
    if (len > 0) {
      buffer[len] = '\0';
      body += buffer;
      totalRead += len;
    } else {
      break;
    }
  }
  return body;
}

// --- UPDATED CONFIGURATION FUNCTION ---
void configureAWS_SNI() {
  SerialMon.println("Applying SSL Config to Context 0 and 1...");

  // === CONTEXT 0 ===
  modem.sendAT("+CSSLCFG=\"sslversion\",0,3");  // TLS 1.2
  modem.waitResponse();
  modem.sendAT("+CSSLCFG=\"sni\",0,1");  // Enable SNI
  modem.waitResponse();
  modem.sendAT("+CSSLCFG=\"ignoreinvalidcert\",0,1");  // Ignore Certs
  modem.waitResponse();
  modem.sendAT("+CSSLCFG=\"ciphersuite\",0,0xFFFF");  // Allow all ciphers
  modem.waitResponse();

  // === CONTEXT 1 (TinyGSM might use this one) ===
  modem.sendAT("+CSSLCFG=\"sslversion\",1,3");
  modem.waitResponse();
  modem.sendAT("+CSSLCFG=\"sni\",1,1");
  modem.waitResponse();
  modem.sendAT("+CSSLCFG=\"ignoreinvalidcert\",1,1");
  modem.waitResponse();
  modem.sendAT("+CSSLCFG=\"ciphersuite\",1,0xFFFF");
  modem.waitResponse();
}

// -----------------------------------------------------------
// FIXED UPDATE FUNCTION WITH DELAYS & RETRY
// -----------------------------------------------------------
void performUpdate() {
  SerialMon.println("\n--- Starting Firmware Download ---");

  // 1. CRITICAL: Give the modem time to close the previous "version.txt" connection
  SerialMon.println("Cooling down modem (5s)...");
  delay(5000);

  // 2. Restart HTTPS Stack
  SerialMon.println("Initializing HTTPS for Download...");
  modem.https_begin();

  // 3. Set URL with Error Checking
  if (!modem.https_set_url(URL_FIRMWARE)) {
    SerialMon.println("ERROR: Failed to set Firmware URL (Modem busy?)");
    modem.https_end();
    return;
  }

  // 4. Request Firmware Size (GET Header)
  SerialMon.println("Requesting Firmware Size...");
  size_t size = 0;

  // We try to get the size. If it returns 0, we retry ONCE.
  int code = modem.https_get(&size);

  if (code == 0) {
    SerialMon.println("Download Warning: Code 0. Retrying in 3s...");
    delay(3000);
    code = modem.https_get(&size);  // Retry
  }

  if (code == 200) {
    SerialMon.printf("Firmware Found! Size: %d bytes\n", size);

    // Check if size is valid
    if (size == 0) {
      SerialMon.println("Error: Firmware size is 0 bytes. Check S3 file.");
      modem.https_end();
      return;
    }

    if (!Update.begin(size)) {
      SerialMon.println("Error: Not enough space on ESP32");
      modem.https_end();
      return;
    }

    uint8_t buff[1024];
    int totalRead = 0;
    int retries = 0;

    // 5. Download Loop
    while (totalRead < size) {
      // Read 1024 bytes (or remaining) from modem
      int len = modem.https_body(buff, sizeof(buff));

      if (len > 0) {
        Update.write(buff, len);
        totalRead += len;
        retries = 0;  // Reset retry counter on success

        // Show progress every 10KB
        if (totalRead % 10240 == 0) {
          SerialMon.printf("Progress: %d / %d bytes\n", totalRead, size);
        }
      } else {
        // If modem returns 0 bytes, it might be pausing. Wait and retry.
        retries++;
        if (retries > 10) {
          SerialMon.println("Error: Timeout waiting for data.");
          break;
        }
        delay(100);
      }
    }

    // 6. Finalize
    if (totalRead == size && Update.end()) {
      SerialMon.println("\nOTA Success! Rebooting in 3s...");
      modem.https_end();
      delay(3000);
      ESP.restart();
    } else {
      SerialMon.printf("\nOTA Failed. Total Read: %d / %d\n", totalRead, size);
      modem.https_end();
    }

  } else {
    SerialMon.printf("Download Request Failed. HTTP Code: %d\n", code);
    modem.https_end();
  }
}