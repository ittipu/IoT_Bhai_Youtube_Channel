import ntptime
import utime
import time
import socket
import machine
import cellular
from machine import Pin

DEVICE_ID = "a9g"
APN = "internet"
WAIT_FOR_SIM_INIT_TIMEOUT = 20  # seconds
WAIT_FOR_NETWORK_TIMEOUT = 30   # seconds
PUBLISH_INTERVAL = 5            # seconds
WAIT_FOR_GPRS_TIMEOUT = 5       # seconds
TIME_ZONE = +6

# LED pin for indication
led = Pin(2, Pin.OUT)

def blink_led(times, interval=0.5):
    """Blink the LED a specified number of times."""
    for _ in range(times):
        led.value(1)
        time.sleep(interval)
        led.value(0)
        time.sleep(interval)

def countdown_timer(seconds, message="Waiting"):
    """Display countdown timer for a given period in seconds."""
    for remaining in range(seconds, 0, -1):
        print("{}... {} seconds remaining".format(message, remaining))  # Updated to use .format()
        time.sleep(1)

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

    print("SIM inserted. ICCID:", cellular.get_iccid())
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

def connect_gprs():
    """Connect to GPRS using the specified APN."""
    print("Attempting GPRS connection...")

    # Countdown for GPRS connection attempt
    countdown_timer(WAIT_FOR_GPRS_TIMEOUT, message="Waiting for GPRS connection")

    try:
        cellular.gprs(APN, "", "")
        print("GPRS Connected")
        blink_led(3, 0.3)
        sync_ntp()  # Sync time after GPRS connection
        return True
    except Exception as e:
        print("GPRS connection failed:", e)
        machine.reset()

def sync_ntp():
    """Synchronize time with NTP server."""
    try:
        print("Syncing with NTP server...")
        ntptime.host = 'pool.ntp.org'
        ntptime.settime()  # Synchronize the time
        print("NTP time synchronized!")
    except Exception as e:
        print("Failed to sync with NTP server:", e)

def sync_ntp():
    """Synchronize time with NTP server."""
    try:
        print("Syncing with NTP server...")
        ntptime.host = 'pool.ntp.org'
        ntptime.settime()  # Synchronize the time
        print("NTP time synchronized!")
    except Exception as e:
        print("Failed to sync with NTP server:", e)

def print_timestamp():
    """Print the current timestamp with the given timezone offset in hours."""
    try:
        # Get the current UTC time as a struct_time tuple
        utc_time = utime.time()

        # Print the timestamp in seconds
        print("Timestamp: {}".format(utc_time))
        
    except Exception as e:
        print("Error retrieving timestamp:", e)

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

    if not connect_gprs():
        print("GPRS connection failed. Restarting...")
        machine.reset()

    print("IP Address:", socket.get_local_ip())

    while True:
        print_timestamp()  # Print time with UTC+6 offset
        time.sleep(PUBLISH_INTERVAL)

# Run the main function
main()

