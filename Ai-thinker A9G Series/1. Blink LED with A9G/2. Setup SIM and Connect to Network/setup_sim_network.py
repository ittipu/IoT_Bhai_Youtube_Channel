import cellular
import time
from machine import Pin

WAIT_FOR_SIM_INIT_TIMEOUT = 20  # seconds
WAIT_FOR_NETWORK_TIMEOUT = 30   # seconds


# LED pin for indication
led = Pin(27, Pin.OUT)

def blink_led(times, interval=0.5):
    """Blink the LED a specified number of times."""
    for _ in range(times):
        led.value(1)
        time.sleep(interval)
        led.value(0)
        time.sleep(interval)

def countdown_timer(seconds, message="Waiting"):
    """Display countdown timer for a given period in seconds on the same line."""
    for remaining in range(seconds, 0, -1):
        print("{}... {} seconds remaining".format(message, remaining), end='\r')  # Use \r and end='' to stay on the same line
        time.sleep(1)
    print()  # Ensure the cursor moves to the next line after countdown finishes

def initialize_device():
    """Initial setup and network registration."""
    print("Device Initialization...")
    blink_led(5)

    print("Getting SIM Info:")

    # Countdown for SIM initialization
    countdown_timer(WAIT_FOR_SIM_INIT_TIMEOUT, message="Initializing SIM")

    if not cellular.is_sim_present():
        print("SIM not inserted!")
        return False

    print("SIM inserted.")
    print("ICCID:", cellular.get_iccid())
    print("IMSI:", cellular.get_imsi())

    print("Waiting for network registration...")

    # Countdown for network registration
    countdown_timer(WAIT_FOR_NETWORK_TIMEOUT, message="Waiting for network registration")

    if not cellular.is_network_registered():
        print("Network not registered.")
        return False

    network_status_message = interpret_network_status(cellular.get_network_status())
    print("Network Status:", network_status_message)

    operator_info = cellular.register()
    print("Operator Name:", operator_info[1])
    print("Signal Quality:", cellular.get_signal_quality()[0])
    return True

def interpret_network_status(status_code):
    """Interpret network registration status."""
    status_messages = {
        0: "Not registered, not searching",
        1: "Registered, home network",
        2: "Not registered, searching for network",
        3: "Registration denied",
        4: "Unknown status",
        5: "Registered, roaming"
    }
    return status_messages.get(status_code, "Invalid status code")

def main():
    """Main entry point of the program."""
    if not initialize_device():
        print("Device initialization failed. Restarting...")
        machine.reset()

main()
