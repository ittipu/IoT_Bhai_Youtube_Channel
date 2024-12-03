// Define motor control pins for a motor driver
const int motorPin1 = 11;   // Motor 1 control pin 1
const int motorPin2 = 12;   // Motor 1 control pin 2
const int motorPin3 = 6;    // Motor 2 control pin 1
const int motorPin4 = 5;    // Motor 2 control pin 2

// Function prototypes to declare movement functions
void moveStop();
void moveForward();
void moveBackward();
void turnRight();
void turnLeft();

void setup() {
  // Initialize serial communication at a baud rate of 9600
  Serial.begin(9600);
  
  // Set motor control pins as outputs
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);
}

void loop() {
  // Stop the motor, move forward, backward, turn right and left in sequence
  moveStop();
  moveForward();
  moveBackward();
  turnRight();
  turnLeft();
  
  // Wait for 2 seconds before repeating the loop
  delay(2000);
}

// Function to stop all motors
void moveStop() {
  Serial.println("Motor in Stop Mode!"); // Print the current action
  // Set all motor pins to 0 (stop)
  analogWrite(motorPin1, 0);
  analogWrite(motorPin2, 0);
  analogWrite(motorPin3, 0);
  analogWrite(motorPin4, 0);
  delay(3000); // Wait for 3 seconds before next command
}

// Function to move motors forward
void moveForward() {
  Serial.println("Motor Moving Forward!"); // Print the current action
  // Set motorPin1 and motorPin3 high, and motorPin2 and motorPin4 low
  analogWrite(motorPin1, 180);
  analogWrite(motorPin2, 0);
  analogWrite(motorPin3, 180);
  analogWrite(motorPin4, 0);
  delay(3000); // Wait for 3 seconds before next command
}

// Function to move motors backward
void moveBackward() {
  Serial.println("Motor Moving Backward!"); // Print the current action
  // Set motorPin2 and motorPin4 high, and motorPin1 and motorPin3 low
  analogWrite(motorPin1, 0);
  analogWrite(motorPin2, 180);
  analogWrite(motorPin3, 0);
  analogWrite(motorPin4, 180);
  delay(3000); // Wait for 3 seconds before next command
}

// Function to turn right by activating motors on one side
void turnRight() {
  Serial.println("Motor Turning Right!"); // Print the current action
  // Set motorPin1 and motorPin4 high, and motorPin2 and motorPin3 low
  analogWrite(motorPin1, 180);
  analogWrite(motorPin2, 0);
  analogWrite(motorPin3, 0);
  analogWrite(motorPin4, 180);
  delay(3000); // Wait for 3 seconds before next command
}

// Function to turn left by activating motors on the opposite side
void turnLeft() {
  Serial.println("Motor Turning Left!"); // Print the current action
  // Set motorPin2 and motorPin3 high, and motorPin1 and motorPin4 low
  analogWrite(motorPin1, 0);
  analogWrite(motorPin2, 180);
  analogWrite(motorPin3, 180);
  analogWrite(motorPin4, 0);
  delay(3000); // Wait for 3 seconds before next command
}
