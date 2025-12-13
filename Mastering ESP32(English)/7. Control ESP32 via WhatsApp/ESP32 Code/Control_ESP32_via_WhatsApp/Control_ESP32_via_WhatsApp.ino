#include <WiFi.h>
#include <PubSubClient.h>

// ============ CONFIGURATION ============
const char* ssid = "tipu_pc";
const char* password = "tipu1234@";

// MQTT Broker
const char* mqtt_server = "172.105.149.22";
// MUST match Python script exactly
const char* topic_cmd = "my_home_123/commands";
const char* topic_msg = "my_home_123/replies";

WiFiClient espClient;
PubSubClient client(espClient);

const int LED_PIN = 2;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  
  // 1. Connect WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");

  // 2. Setup MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Keeps MQTT alive
  delay(100);
}

// ============ LOGIC ============
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("🔥 Command: ");
  Serial.println(message);

  if (message == "led on") {
    digitalWrite(LED_PIN, HIGH);
    client.publish(topic_msg, "LED is ON ✅");
  }
  else if (message == "led off") {
    digitalWrite(LED_PIN, LOW);
    client.publish(topic_msg, "LED is OFF ❌");
  }
  else if (message == "status") {
    int state = digitalRead(LED_PIN);
    String status = "System Online. Light is " + String(state ? "ON" : "OFF");
    client.publish(topic_msg, status.c_str());
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection... ");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Resubscribe to commands
      client.subscribe(topic_cmd);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}