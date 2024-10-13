import cellular
import time
import socket
import machine
import ntptime
from machine import Pin

DEVICE_ID = "a9g"  
MQTT_BROKER_ADDRESS = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_USERNAME = ""
MQTT_PASSWORD = ""
SUBSCRIBE_TOPIC = "test/sub"
PUBLISH_TOPIC = "test/pub"
APN = "internet"

WAIT_FOR_SIM_INIT_TIMEOUT = 15  # seconds
WAIT_FOR_NETWORK_TIMEOUT = 15   # seconds
PUBLISH_INTERVAL = 5            # seconds
WAIT_FOR_GPRS_TIMEOUT = 5       # seconds 

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

def mqtt_setup(client):
    """Setup MQTT client with subscription and callback."""
    def mqtt_callback(topic, msg):
        print("Received message from topic: {}, Message: {}".format(topic, msg))

    client.set_callback(mqtt_callback)
    client.connect()
    print("Connected to MQTT Broker")
    client.subscribe(SUBSCRIBE_TOPIC)
    print("Subscribed to topic:", SUBSCRIBE_TOPIC)

def send_mqtt_data(client):
    """Send data to the MQTT broker."""
    message = "hello from a9g"
    formatted_message = "ID:{} Message: {}".format(DEVICE_ID, message)
    print('Sending Data: Topic = {}, Msg = {}'.format(PUBLISH_TOPIC, formatted_message))
    client.publish(PUBLISH_TOPIC, formatted_message)

    # Indicate the message has been sent by toggling the LED
    led.value(1)
    time.sleep(1)
    led.value(0)

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

    try:
        from umqtt import simple
    except ImportError:
        import upip
        upip.install("micropython-umqtt.simple")
        from umqtt import simple

    # Set up MQTT client
    client = simple.MQTTClient(DEVICE_ID, MQTT_BROKER_ADDRESS, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD)
    mqtt_setup(client)

    while True:
        send_mqtt_data(client)
        time.sleep(PUBLISH_INTERVAL)
        client.check_msg()

main()
