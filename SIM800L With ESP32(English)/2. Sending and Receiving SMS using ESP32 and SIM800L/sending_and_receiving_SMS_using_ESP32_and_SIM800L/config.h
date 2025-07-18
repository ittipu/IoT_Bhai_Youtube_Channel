// put your phone number with country code for sending SMS
#define ADMIN_NUMBER "" 

// ESP32 and SIM800l pins
#define MODEM_TX 26
#define MODEM_RX 27
#define MODEM_RST 14
#define MODEM_DTR 25
#define MODEM_RING 34

// APN Settings
const char apn[] = "internet"; // put your SIM APN
const char gprsUser[] = "";
const char gprsPass[] = "";

#define SerialMon Serial
#define SerialAT Serial1
#define TINY_GSM_DEBUG SerialMon
#define GSM_PIN ""
#define NTP_SERVER "132.163.96.5"

