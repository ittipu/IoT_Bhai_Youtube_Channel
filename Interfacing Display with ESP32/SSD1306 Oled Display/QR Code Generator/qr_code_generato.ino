#include <Arduino.h>
#define OLEDDISPLAY
#include <qrcodeoled.h>
#include <SSD1306.h>

SSD1306 display(0x3c, 21, 22);  // Only change

QRcodeOled qrcode(&display);


void setup() {

  Serial.begin(115200);
  Serial.println("");
  Serial.println("Starting...");

  setupDisplay();
  displayCenteredText("Scan QR", 2000);
  displayQR("www.google.com", 100000);
}

void loop() {}


void setupDisplay() {
  display.init();
  display.clear();
  display.display();
  display.flipScreenVertically();
}

void displayQR(String text, int duration) {
  // Initialize QRcode display using library
  qrcode.init();
  // create qrcode
  qrcode.create(text);
  delay(duration);
  // Clear the screen after the delay
  display.clear();
  display.display();
}


void displayCenteredText(String text, int duration) {
  // Set the font size
  display.setFont(ArialMT_Plain_16);

  // Calculate the width and height of the text to center it
  int16_t textWidth = display.getStringWidth(text);  // Get text width
  int16_t textHeight = 16;                           // Font height for ArialMT_Plain_16 is 16 pixels

  // Set cursor position to center (horizontal and vertical centering)
  int16_t x = (display.getWidth() - textWidth) / 2;
  int16_t y = (display.getHeight() - textHeight) / 2;

  // Clear screen and draw the text at the calculated position
  display.clear();
  display.drawString(x, y, text);  // Use drawString to draw text at the calculated position
  display.display();

  // Keep the text on the screen for the specified duration (in milliseconds)
  delay(duration);

  // Clear the screen after the delay
  display.clear();
  display.display();
}