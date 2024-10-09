import machine
import gps
import time

def main():
    print("Device Init")
    gps.on()  # Turn on the GPS module
    led = machine.Pin(27, machine.Pin.OUT)  # Initialize LED on pin 27

    while True:
        location = gps.get_location()  # Fetch GPS location data
        if location is not None:  # Check if valid GPS data is available
            formatted_message = "Lat: {:.6f}, Lng: {:.6f}".format(location[0], location[1])
            print(formatted_message)

            # Blink the LED when valid data is found
            led.value(1)  # Turn LED on
            time.sleep(1)  # Keep it on for 1 second
            led.value(0)  # Turn LED off
            time.sleep(1)  # Keep it off for 1 second
        else:
            print("No valid GPS data found")
            time.sleep(2)  # Wait 2 seconds before checking again

main()
