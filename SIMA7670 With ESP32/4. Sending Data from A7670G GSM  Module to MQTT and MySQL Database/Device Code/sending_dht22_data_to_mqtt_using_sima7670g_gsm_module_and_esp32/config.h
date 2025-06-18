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

const char *mqtt_broker = "";
int port = 1883;
const char *mqtt_username = "";
const char *mqtt_password = "";
const char *publish_topic = "weather/data";
const char *subs_topic = "device/command";