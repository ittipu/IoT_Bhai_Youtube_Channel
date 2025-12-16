// --- CONFIGURATION ---
const char currentVersion[] = "1.0.0"; 
const char apn[] = "internet"; // Change to your APN
const char gprsUser[] = "";
const char gprsPass[] = "";

// Server Details (HTTP Port 5000)
const char server[]   = "139.162.175.131";
const int  port       = 5000;
const char pathVersion[]  = "/api/version";
const char pathFirmware[] = "/api/firmware.bin";

// ESP32 and SIM800l pins
#define MODEM_TX 26
#define MODEM_RX 27
#define MODEM_RST 14
#define MODEM_DTR 25
#define MODEM_RING 34

#define BUILTIN_LED 2
