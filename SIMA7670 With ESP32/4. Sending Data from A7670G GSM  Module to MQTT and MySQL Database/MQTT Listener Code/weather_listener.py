import paho.mqtt.client as mqtt
import json
import mysql.connector
from config import *

# === MySQL CONNECTION ===
db = mysql.connector.connect(
    host=mysql_host,
    user=mysql_user,
    password=mysql_password,
    database=mysql_db
)
cursor = db.cursor()

# Called when the client connects to the broker
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker!")
        client.subscribe(topic)
    else:
        print("Failed to connect, return code:", rc)

# Called when a message is received
def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode()
        print(f"📨 Message received on topic '{msg.topic}': {payload}")
        data = json.loads(payload)

        device_id = data.get("device_id")
        temperature = data.get("temperature")
        humidity = data.get("humidity")

        print(f"🌡️ Temperature: {temperature} °C")
        print(f"💧 Humidity: {humidity} %\n")

        # SQL Insert using table_name variable
        sql = f"INSERT INTO {table_name} (device_id, temperature, humidity) VALUES (%s, %s, %s)"
        values = (device_id, temperature, humidity)
        cursor.execute(sql, values)
        db.commit()

    except Exception as e:
        print("❗ Error processing message:", e)

# Create client and set username/password
client = mqtt.Client()
client.username_pw_set(username, password)

client.on_connect = on_connect
client.on_message = on_message

# Connect and loop forever
client.connect(broker, port, keepalive=60)
client.loop_forever()
