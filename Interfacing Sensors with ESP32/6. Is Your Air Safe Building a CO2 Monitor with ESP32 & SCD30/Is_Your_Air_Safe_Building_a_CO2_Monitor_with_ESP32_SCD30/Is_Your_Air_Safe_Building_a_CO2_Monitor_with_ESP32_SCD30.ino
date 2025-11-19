#include <Wire.h>
#include "Adafruit_SCD30.h"

// Create an SCD30 sensor object
Adafruit_SCD30 scd30;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // Wait for Serial Monitor to open
  }

  Serial.println("SCD30 Test Initializing...");

  // Initialize the I2C bus
  Wire.begin();

  // Try to initialize the SCD30
  if (!scd30.begin()) {
    Serial.println("Failed to find SCD30 sensor. Check wiring!");
    while (1) {
      delay(10);
    }
  }
  Serial.println("SCD30 sensor found!");
}

// Function to determine status based on the image chart provided
void checkAirQuality(float co2_reading) {
  Serial.print("Status: ");
  
  if (co2_reading <= 350) {
    Serial.println("Healthy outside air level (Excellent)");
  } 
  else if (co2_reading <= 600) {
    Serial.println("Healthy indoor climate (Good)");
  } 
  else if (co2_reading <= 800) {
    Serial.println("Acceptable level (Fair)");
  } 
  else if (co2_reading <= 1000) {
    Serial.println("Ventilation required (Poor)");
  } 
  else if (co2_reading <= 1200) {
    Serial.println("Ventilation necessary (Bad)");
  } 
  else if (co2_reading <= 2500) {
    Serial.println("Negative health effects (Very Bad)");
  } 
  else {
    // Covers 2000 to 5000+
    Serial.println("HAZARDOUS PROLONGED EXPOSURE (DANGER)");
  }
}

void loop() {
  // Check if new data is available
  if (scd30.dataReady()) {
    Serial.println("--- New Data ---");
    
    // Read the sensor data
    if (!scd30.read()) {
      Serial.println("Error reading sensor data");
      return;
    }

    // Print the readings
    Serial.print("CO2 (ppm): ");
    Serial.println(scd30.CO2);

    // --- CALL THE NEW FUNCTION HERE ---
    checkAirQuality(scd30.CO2);
    // ----------------------------------

    Serial.print("Temperature (C): ");
    Serial.println(scd30.temperature);

    Serial.print("Humidity (%): ");
    Serial.println(scd30.relative_humidity);
    Serial.println();

  } else {
    // Serial.println("No new data yet..."); 
    // Commented out to reduce spam in monitor
  }

  delay(2000); // Adjusted to 2s to match sensor refresh rate
}