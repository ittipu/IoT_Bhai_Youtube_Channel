#define SerialMon Serial
#define SerialAT Serial1
#define TINY_GSM_DEBUG SerialMon
#define GSM_PIN ""
#define NTP_SERVER "132.163.96.5"

// ESP32 and SIM800l pins
#define MODEM_TX 26
#define MODEM_RX 27
#define MODEM_RST 14
#define MODEM_DTR 25
#define MODEM_RING 34

#define BUILTIN_LED 2

// APN Settings
const char apn[] = "internet"; // put your SIM APN
const char gprsUser[] = "";
const char gprsPass[] = "";

// MQTT details
const char* mqtt_broker = "172.236.236.214";
const int mqtt_port = 1883;
const char* mqtt_username = "user1"; // leave blank if not mqtt user pass not configured
const char* mqtt_password = "user1"; 
const char* topic_pub = "esp32/data";
const char* topic_sub = "esp32/led";

const int UTC_OFFSET_HOURS = 6; // Define UTC offset for your timezone (e.g., +6 for Bangladesh)
const int timezoneParam = UTC_OFFSET_HOURS * 4;
const char* daysOfTheWeek[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}; // Helper array for printing the day of the week
