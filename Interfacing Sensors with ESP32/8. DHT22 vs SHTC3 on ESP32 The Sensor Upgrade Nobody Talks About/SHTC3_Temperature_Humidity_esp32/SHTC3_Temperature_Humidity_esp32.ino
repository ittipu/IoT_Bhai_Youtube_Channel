/*
 * SHTC3 Temperature & Humidity — ESP32 demo
 * I2C: SDA = GPIO22, SCL = GPIO19  (swap in Wire.begin if yours differ)
 * Sensor is 3.3V only — power from 3V3, not 5V.
 * Library: "Adafruit SHTC3" (Library Manager) -> pulls Adafruit BusIO + Unified Sensor
 */
#include <Wire.h>
#include "Adafruit_SHTC3.h"

#define SDA_PIN 22
#define SCL_PIN 19

Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Wire.begin(SDA_PIN, SCL_PIN);        // custom I2C pins

  if (!shtc3.begin()) {                // default I2C address 0x70
    Serial.println("SHTC3 not found — check wiring / 3.3V power!");
    while (1) delay(10);
  }
  Serial.println("SHTC3 ready.");
}

void loop() {
  sensors_event_t humidity, temp;
  shtc3.getEvent(&humidity, &temp);    // reads both in one shot

  Serial.printf("Temp: %.2f C   RH: %.2f %%\n",
                temp.temperature, humidity.relative_humidity);

  delay(200);
}
