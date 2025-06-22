#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>

// Sound sensor settings
const int sampleWindow = 50; // Sample window width in ms
unsigned int sample;
const float VREF = 0.0001; // Reference voltage for dB calculation

// OLED settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2); // Larger text size for visibility
}

void loop() {
  unsigned long startMillis = millis();
  unsigned int signalMax = 0;
  unsigned int signalMin = 4095;

  // Capture peak-to-peak audio signal
  while (millis() - startMillis < sampleWindow) {
    sample = analogRead(35);
    if (sample < 4096) {
      if (sample > signalMax) signalMax = sample;
      if (sample < signalMin) signalMin = sample;
    }
  }

  unsigned int peakToPeak = signalMax - signalMin;
  float volts = (peakToPeak * 3.3) / 4095.0;
  float dB = (volts > VREF) ? 20 * log10(volts / VREF) : -100;

  // Debug to serial
  Serial.print("Sound Level: ");
  Serial.print(dB, 2);
  Serial.println(" dB");

  // Display on OLED (2 lines, vertically centered)
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor((SCREEN_WIDTH - 12 * 6) / 2, 16);  // Centered horizontally
  display.println("Sound");
  display.setCursor((SCREEN_WIDTH - 12 * 6) / 2, 40);  // Centered below
  display.print(dB, 1);
  display.println(" dB");
  display.display();

  delay(200);
}
