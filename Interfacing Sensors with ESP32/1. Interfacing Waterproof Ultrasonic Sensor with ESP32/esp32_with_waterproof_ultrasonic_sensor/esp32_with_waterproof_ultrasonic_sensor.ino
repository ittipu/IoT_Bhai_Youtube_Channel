#define ULTRASONIC_RX_PIN 22
#define ULTRASONIC_TX_PIN 23

unsigned char data[4] = {};
float distance;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, ULTRASONIC_RX_PIN, ULTRASONIC_TX_PIN);
}

void loop() {
  do {
    for (int i = 0; i < 4; i++) {
      data[i] = Serial2.read();
    }
  } while (Serial2.read() == 0xff);

  Serial2.flush();

  if (data[0] == 0xff) {
    int sum;
    sum = (data[0] + data[1] + data[2]) & 0x00FF;
    if (sum == data[3]) {
      distance = (data[1] << 8) + data[2];
      distance = distance / 10;
      Serial.print(distance);
      Serial.println(" cm");
    } else {
      Serial.println("ERROR");
    }
  }

  delay(100);
}
