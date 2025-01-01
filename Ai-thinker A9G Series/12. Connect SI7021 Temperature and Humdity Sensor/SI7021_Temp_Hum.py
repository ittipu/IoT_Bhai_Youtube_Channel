import i2c
import time

# Global I2C Port Number
I2C_PORT = 2  # Change this to 2 or 3 as needed

# SI7021 I2C Address
SI7021_ADDRESS = 0x40

# SI7021 Commands
SI7021_MEASURE_HUMIDITY = 0xF5  # Command to measure humidity
SI7021_MEASURE_TEMPERATURE = 0xE0  # Command to measure temperature

# Default timeout for I2C operations
I2C_DEFAULT_TIME_OUT = 10  # 10 ms

# Initialize I2C
i2c.init(I2C_PORT, 100000)  # Using the global I2C_PORT variable
time.sleep(0.1)  # Allow time for device to settle

def read_humidity():
    """Read humidity from the SI7021."""
    try:
        # Trigger humidity measurement
        pData = bytes([SI7021_MEASURE_HUMIDITY])
        i2c.transmit(I2C_PORT, SI7021_ADDRESS, pData, I2C_DEFAULT_TIME_OUT)
        time.sleep(0.05)  # Wait for measurement
        # Read 2 bytes of humidity data
        data = i2c.receive(I2C_PORT, SI7021_ADDRESS, 2, I2C_DEFAULT_TIME_OUT)
        if data is not None:
            humidity = ((data[0] << 8) + data[1]) * 125 / 65536 - 6
            return humidity
    except Exception as e:
        print("Error reading humidity: {}".format(e))
    return None

def read_temperature():
    """Read temperature from the SI7021."""
    try:
        # Trigger temperature measurement
        pData = bytes([SI7021_MEASURE_TEMPERATURE])
        i2c.transmit(I2C_PORT, SI7021_ADDRESS, pData, I2C_DEFAULT_TIME_OUT)
        time.sleep(0.05)  # Wait for measurement
        # Read 2 bytes of temperature data
        data = i2c.receive(I2C_PORT, SI7021_ADDRESS, 2, I2C_DEFAULT_TIME_OUT)
        if data is not None:
            temperature = ((data[0] << 8) + data[1]) * 175.72 / 65536 - 46.85
            return temperature
    except Exception as e:
        print("Error reading temperature: {}".format(e))
    return None

while True:
    humidity = read_humidity()
    temperature = read_temperature()

    if humidity is not None:
        print("Humidity: {:.2f}%".format(humidity))
    else:
        print("Error reading humidity")

    if temperature is not None:
        print("Temperature: {:.2f} °C".format(temperature))
    else:
        print("Error reading temperature")

    time.sleep(5)  # Wait for 5 seconds before the next reading
