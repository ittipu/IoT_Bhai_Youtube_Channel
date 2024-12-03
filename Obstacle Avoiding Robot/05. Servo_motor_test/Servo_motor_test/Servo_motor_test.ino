#include <Servo.h>

int servoPin = 9;                          // Pin connected to the servo control signal
Servo myServo;                             // Create a Servo object to control the servo



 // Function prototypes for controlling the servo
void servoControl();       
void servoControl_degree_wise();

void setup() {
  Serial.begin(9600);                       // Initialize serial communication at 9600 baud
  myServo.attach(servoPin);                 // Attach the servo to the specified pin
  myServo.write(0);                         // Set the servo to the initial position (0 degrees)
}

void loop() {
  servoControl();                            // Call the function to move the servo to predefined angles
  delay(1000);                               // Wait for 1 second before moving to the next function
  servoControl_degree_wise();                // Call the function to move the servo angle by angle
}

void servoControl() {
  myServo.write(0);                          // Move the servo to 0 degrees
  Serial.println("Servo at 0 degrees");
  delay(1000);                               // Wait for 1 second
  
  myServo.write(90);                         // Move the servo to 90 degrees
  Serial.println("Servo at 90 degrees");
  delay(1000);                              // Wait for 1 second

  myServo.write(180);                       // Move the servo to 180 degrees
  Serial.println("Servo at 180 degrees");
  delay(1000);                              // Wait for 1 second
}



void servoControl_degree_wise() {
  for (int angle = 0; angle <= 180; angle++) {
    myServo.write(angle);                        // Set the servo to the current angle (0 to 180)
    Serial.print("Servo angle: ");
    Serial.println(angle);                       // Display the current angle in the serial monitor
    delay(20);                                   // Short delay for smooth servo movement
  }

  delay(1000);                                   // Wait for 1 second before reversing the direction

  for (int angle = 180; angle >= 0; angle--) {
    myServo.write(angle);                       // Move the servo back from 180 to 0 degrees
    Serial.print("Servo angle: ");
    Serial.println(angle);                      // Display the current angle in the serial monitor
    delay(20);                                  // Short delay for smooth servo movement
  }

  delay(1000);                                 // Wait for 1 second before the next iteration
}
