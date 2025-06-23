#define DEVICE_ID "W001"

// === DHT11 Pin Definitions ===
#define DHTPIN 23
#define DHTTYPE DHT11  // DHT 22  (AM2302), AM2321


// === SIM7600 Pin Definitions ===
#define MODEM_RESET_PIN 5
#define MODEM_PWKEY 4
#define MODEM_POWER_ON 12
#define MODEM_TX 26
#define MODEM_RX 27
#define MODEM_RESET_LEVEL HIGH
#define BUILTIN_LED 2

const char apn[] = "internet";
const char gprsUser[] = "";
const char gprsPass[] = "";

// MQTT details
#define MAX_MQTT_RECONNECT_COUNTER 10
const char* broker = "a1aqkedukfaxny-ats.iot.ap-southeast-1.amazonaws.com";
int broker_port = 8883;
const char* client_id  = "A76XX";
const char* publish_topic = "device/weather";
const char* subscribe_topic = "device/cmd";