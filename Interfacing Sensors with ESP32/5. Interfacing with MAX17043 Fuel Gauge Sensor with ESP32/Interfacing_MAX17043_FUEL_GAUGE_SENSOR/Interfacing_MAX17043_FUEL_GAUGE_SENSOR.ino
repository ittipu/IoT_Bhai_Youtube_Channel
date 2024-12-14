#include <Wire.h>
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>

SFE_MAX1704X lipo;  // Defaults to the MAX17043


double voltage = 0;
double soc = 0;
bool alert;

void setup() {
  Serial.begin(115200);  // Start serial, to output debug data
  Serial.println(F("MAX17043 Example"));
  Wire.begin();
  lipo.enableDebugging();
  if (lipo.begin() == false) {
    Serial.println(F("MAX17043 not detected. Please check wiring. Freezing."));
    while (1)
      ;
  }

  lipo.quickStart();
  // We can set an interrupt to alert when the battery SoC gets too low.
  // We can alert at anywhere between 1% - 32%:
  lipo.setThreshold(20);  // Set alert threshold to 20%.
}

void loop() {
  voltage = lipo.getVoltage();
  soc = lipo.getSOC();
  alert = lipo.getAlert();

  // Print the variables:
  Serial.print("Voltage: ");
  Serial.print(voltage);
  Serial.println(" V");

  Serial.print("Percentage: ");
  Serial.print(soc);
  Serial.println(" %");

  Serial.print("Alert: ");
  Serial.println(alert);
  Serial.println();
  delay(5000);
}
