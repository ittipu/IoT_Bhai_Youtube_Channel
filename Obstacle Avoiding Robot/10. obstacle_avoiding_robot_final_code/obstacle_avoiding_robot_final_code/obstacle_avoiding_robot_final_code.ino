// Md Shohanuzzaman //
// Obstacle avoiding robot //
#include <NewPing.h>
#include <Servo.h>
#define TRIG_PIN A4
#define ECHO_PIN A5
#define MAX_DISTANCE 200
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);
Servo myservo;
boolean goesForward = false;
int distance = 100;
int speedSet = 0;
const int motorPin1 = 11;
const int motorPin2 = 12;
const int motorPin3 = 6;
const int motorPin4 = 5;

void setup() {

  myservo.attach(9);
  myservo.write(115);
  delay(2000);
  distance = readPing();
  delay(100);
  distance = readPing();
  delay(100);
  distance = readPing();
  delay(100);
  distance = readPing();
  delay(100);
  distance = readPing();
  delay(100);
}

void loop() {
  int distanceR = 0;
  int distanceL = 0;
  delay(40);

  if (distance <= 30) {
    moveStop();
    delay(100);
    moveBackward();
    delay(400);
    moveStop();
    delay(200);
    distanceR = lookRight();
    delay(200);
    distanceL = lookLeft();
    delay(200);
    if (distanceR >= distanceL) {
      turnRight();
      moveStop();
    } else {
      turnLeft();
      moveStop();
    }
  } else {
    moveForward();
  }
  distance = readPing();
}



int lookRight() {
  myservo.write(50);
  delay(400);
  int distance = readPing();
  delay(100);
  myservo.write(115);
  return distance;
}

int lookLeft() {
  myservo.write(170);
  delay(400);
  int distance = readPing();
  delay(100);
  myservo.write(115);
  return distance;
  delay(100);
}
int readPing() {
  delay(70);
  int cm = sonar.ping_cm();
  if (cm == 0) {
    cm = 250;
  }

  return cm;
}

void moveStop() {
  analogWrite(motorPin1, 0);
  analogWrite(motorPin2, 0);
  analogWrite(motorPin3, 0);
  analogWrite(motorPin4, 0);
}

void moveForward() {
  analogWrite(motorPin1, 180);
  analogWrite(motorPin2, 0);
  analogWrite(motorPin3, 180);
  analogWrite(motorPin4, 0);
}


void moveBackward() {
  analogWrite(motorPin1, 0);
  analogWrite(motorPin2, 180);
  analogWrite(motorPin3, 0);
  analogWrite(motorPin4, 180);
}


void turnRight() {
  analogWrite(motorPin1, 180);
  analogWrite(motorPin2, 0);
  analogWrite(motorPin3, 0);
  analogWrite(motorPin4, 180);
  delay(300);
  moveForward();
}

void turnLeft() {
  analogWrite(motorPin1, 0);
  analogWrite(motorPin2, 180);
  analogWrite(motorPin3, 180);
  analogWrite(motorPin4, 0);
  delay(300);
  moveForward();
}
