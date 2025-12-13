import json
from flask import Flask, request
from twilio.rest import Client
import paho.mqtt.client as mqtt

app = Flask(__name__)

# ============ CONFIGURATION ============
TWILIO_SID = ""
TWILIO_AUTH = ""
TWILIO_FROM = ""  # The Sandbox Number
MY_NUMBER = ""     # Your Number (with country code)

# MQTT Public Broker
MQTT_BROKER = "127.0.0.1"
MQTT_PORT = 1883
# TOPICS (Change these to something unique!)
TOPIC_CMD = "my_home_123/commands"  # VPS sends commands here
TOPIC_MSG = "my_home_123/replies"   # ESP32 replies here

# === FIX 1: Rename this variable to avoid conflict ===
twilio_client = Client(TWILIO_SID, TWILIO_AUTH)

mqtt_client = mqtt.Client()

# ============ MQTT FUNCTIONS ============
def on_connect(client, userdata, flags, rc):
    print("✅ Connected to MQTT Broker!")
    # Subscribe to replies from ESP32
    client.subscribe(TOPIC_MSG)

def on_message(client, userdata, msg):
    # This 'client' variable represents the MQTT connection, NOT Twilio.
    
    text = msg.payload.decode()
    print(f"📩 Received from ESP32: {text}")
    
    # Send to WhatsApp
    try:
        # === FIX 2: Use the global 'twilio_client' variable here ===
        twilio_client.messages.create(
            from_=TWILIO_FROM,
            body=f"🤖 ESP32: {text}",
            to=MY_NUMBER
        )
        print("✅ WhatsApp reply sent!")
    except Exception as e:
        print(f"Twilio Error: {e}")

# Setup MQTT
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

# Connect to Local Broker
try:
    mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
    # Start background thread
    mqtt_client.loop_start()
except Exception as e:
    print(f"❌ MQTT Connection Error: {e}")

# ============ FLASK (WEB) FUNCTIONS ============
@app.route('/whatsapp', methods=['POST'])
def whatsapp_webhook():
    msg = request.values.get('Body', '').lower()
    print(f"📱 WhatsApp: {msg}")
    
    # Publish command to ESP32 via MQTT
    mqtt_client.publish(TOPIC_CMD, msg)
    
    return "OK", 200

if __name__ == '__main__':
    # Run Flask with threading enabled
    app.run(host='0.0.0.0', port=5000, threaded=True)