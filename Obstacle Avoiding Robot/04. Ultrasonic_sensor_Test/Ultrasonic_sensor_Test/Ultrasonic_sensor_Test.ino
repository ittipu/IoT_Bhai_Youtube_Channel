#include <NewPing.h>      // NewPing is a dedicated library for measuing distance of Ultrasonic Sensor
#define TRIG_PIN A4       // Define the pin for the trigger signal
#define ECHO_PIN A5       // Define the pin for the echo signal
#define MAX_DISTANCE 200  // Maximum distance to measure (in cm)


// Initialize the NewPing object with the defined trigger, echo pins, and maximum distance
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);



// Function prototype for reading distance from the ultrasonic sensor
int readPing();



void setup() {
  Serial.begin(9600);  // Initialize serial communication at 9600 baud rate
}



void loop() {
  int distance = readPing();    // Get the distance reading from the ultrasonic sensor
  Serial.print("Distance : ");  // Print "Distance :" to the serial monitor
  Serial.print(distance);       // Print the measured distance
  Serial.println(" CM");        // Print "CM" and move to the next line in the serial monitor
  delay(1000);                  // Wait for 1 second before the next reading
}



// Function to read and return the distance measured by the ultrasonic sensor
int readPing() {
  delay(70);                 // Short delay to allow sensor stability between readings
  int cm = sonar.ping_cm();  // Get the distance in centimeters from the ultrasonic sensor
  if (cm == 0) {             // If no object is detected or out of range
    cm = 250;                // Set distance to 250 cm as a default for out of range
  }

  return cm;  // Return the measured or default distance
}
