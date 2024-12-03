const int motorPin3 = 6;                   // Pin connected to motor terminal 3 (for controlling motor direction/speed)
const int motorPin4 = 5;                   // Pin connected to motor terminal 4 (for controlling motor direction/speed)

// Function prototypes for controlling motor movement
void moveStop();     
void moveForward();  
void moveBackward(); 

void setup() {
  Serial.begin(9600);                       // Initialize serial communication at 9600 baud
  Serial.println("Code started!");          // Indicate that the code execution has started
  pinMode(motorPin3, OUTPUT);               // Set motorPin3 as an output pin
  pinMode(motorPin4, OUTPUT);               // Set motorPin4 as an output pin
  Serial.println("Pin Mode Define Done!");  // Indicate that pin mode setup is complete
}

void loop() {
  moveStop();                               // Stop the motor
  delay(1000);                              // Wait for 1 second

  moveForward();                            // Move the motor forward
  delay(1000);                              // Wait for 1 second

  moveBackward();                           // Move the motor backward
  delay(1000);                              // Wait for 1 second
}

void moveStop() {
  analogWrite(motorPin3, 0);                 // Set motorPin3 to 0, stopping the motor
  analogWrite(motorPin4, 0);                 // Set motorPin4 to 0, stopping the motor
  Serial.println("Motor Stopped!");          // Print status to the serial monitor
}

void moveForward() {
  analogWrite(motorPin3, 180);               // Set motorPin3 to a PWM value of 180 (approx. 70% speed) to move forward
  analogWrite(motorPin4, 0);                 // Set motorPin4 to 0, ensuring correct direction
  Serial.println("Motor Move Forward!");     // Print status to the serial monitor
}

void moveBackward() {
  analogWrite(motorPin3, 0);                 // Set motorPin3 to 0
  analogWrite(motorPin4, 180);               // Set motorPin4 to a PWM value of 180 (approx. 70% speed) to move backward
  Serial.println("Motor Move Backward!");    // Print status to the serial monitor
}
