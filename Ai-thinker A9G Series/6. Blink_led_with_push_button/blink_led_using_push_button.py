from machine import Pin
import time

# Configure the GPIO pin for the button as input with an internal pull-up resistor
button = Pin(25, Pin.IN, Pin.PULL_UP)  # Adjust pin as needed

# Configure the GPIO pin for the LED as output
led = Pin(30, Pin.OUT)  # Adjust pin as needed

while True:
    if button.value() == 1:  # Button pressed (active-low)
        print("Button Pressed!")
        led.value(1)  # Turn LED on
    else:
        print("Button Released!")
        led.value(0)  # Turn LED off
    
    time.sleep(0.5)  # Small delay to debounce the button
