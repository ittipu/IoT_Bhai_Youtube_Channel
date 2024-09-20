#include "secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#define AWS_IOT_PUBLISH_TOPIC "test/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "test/sub"


WiFiClientSecure espClient = WiFiClientSecure();
PubSubClient client(espClient);

void connectAWS() {
  delay(100);
  client.setServer(AWS_IOT_ENDPOINT, 8883);
  client.setCallback(messageHandler);

  while (!client.connected()) {
    Serial.println("Connecting to AWS....");

    if (client.connect(THINGNAME)) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state ");
      Serial.println(client.state());
      delay(200);
    }
    // Subscribe to a topic
    client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  }
}

void publishMessage() {
  StaticJsonDocument<200> doc;
  char jsonBuffer[512];
  doc["millis"] = millis();
  Serial.println("Message: "); 
  serializeJson(doc, jsonBuffer);  // print to client
  serializeJson(doc, Serial);
  Serial.println();
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  Serial.println("Published\n");

}

void messageHandler(char* topic, byte* payload, unsigned int length) {
  Serial.print("incoming: ");
  Serial.println(topic);
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  connectAWS();
}

void loop() {
  if (!client.connected()) {
    reconnect_AWS();
  }
  publishMessage();

  client.loop();
  delay(5000);
}

void reconnect_AWS() {
  while (!client.connected()) {
    Serial.println("Attempting AWS IoT Core connection...");
    // Subscribe to a topic
    if (client.connect(THINGNAME)) {
      Serial.println("AWS IoT Connected!");
      client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}


void setup_wifi() {
  WiFi.mode(WIFI_STA);
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_NAME);
  WiFi.begin(WIFI_NAME, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi Connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  espClient.setCACert(AWS_CERT_CA);
  espClient.setCertificate(AWS_CERT_CRT);
  espClient.setPrivateKey(AWS_CERT_PRIVATE);
}
