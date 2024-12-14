#include <Wire.h>

// SI7021 I2C Address
#define SI7021_ADDR 0x40
#define HUMIDITY_CMD 0xF5
#define TEMPERATURE_CMD 0xF3

// Function prototypes
float readHumidity();
float readTemperatureCelsius();
float celsiusToFahrenheit(float celsius);
bool checkI2CConnection(uint8_t address);

void setup() {
  // Initialize I2C communication and Serial monitor
  Wire.begin();
  Serial.begin(115200);
  delay(300); // Stabilization delay
  
  // Check if the sensor is connected
  if (checkI2CConnection(SI7021_ADDR)) {
    Serial.println("SI7021 sensor detected successfully!");
  } else {
    Serial.println("Error: SI7021 sensor not detected. Please check connections.");
    while (true); // Stop the program if the sensor is not found
  }
}

void loop() {
  // Read and display humidity
  float humidity = readHumidity();
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" % RH");

  // Read and display temperature in Celsius and Fahrenheit
  float temperatureC = readTemperatureCelsius();
  float temperatureF = celsiusToFahrenheit(temperatureC);
  Serial.print("Temperature: ");
  Serial.print(temperatureC);
  Serial.println(" °C");
  Serial.print("Temperature: ");
  Serial.print(temperatureF);
  Serial.println(" °F");
  Serial.println();

  // Delay before next measurement
  delay(5000);
}

// Function to read humidity from SI7021
float readHumidity() {
  Wire.beginTransmission(SI7021_ADDR);
  Wire.write(HUMIDITY_CMD); // Send humidity measurement command
  Wire.endTransmission();
  delay(500);

  Wire.requestFrom(SI7021_ADDR, 2); // Request 2 bytes of data
  if (Wire.available() == 2) {
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    // Convert raw data to humidity percentage
    float humidity = ((msb << 8) | lsb) * 125.0 / 65536.0 - 6.0;
    return humidity;
  } else {
    Serial.println("Error: Failed to read humidity data.");
    return -1.0; // Return an error value
  }
}

// Function to read temperature in Celsius from SI7021
float readTemperatureCelsius() {
  Wire.beginTransmission(SI7021_ADDR);
  Wire.write(TEMPERATURE_CMD); // Send temperature measurement command
  Wire.endTransmission();
  delay(500);

  Wire.requestFrom(SI7021_ADDR, 2); // Request 2 bytes of data
  if (Wire.available() == 2) {
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    // Convert raw data to temperature in Celsius
    float temperature = ((msb << 8) | lsb) * 175.72 / 65536.0 - 46.85;
    return temperature;
  } else {
    Serial.println("Error: Failed to read temperature data.");
    return -100.0; // Return an error value
  }
}

// Function to convert Celsius to Fahrenheit
float celsiusToFahrenheit(float celsius) {
  return celsius * 1.8 + 32;
}

// Function to check if an I2C device is connected
bool checkI2CConnection(uint8_t address) {
  Wire.beginTransmission(address);
  uint8_t error = Wire.endTransmission();
  return (error == 0); // Returns true if the device is connected
}
