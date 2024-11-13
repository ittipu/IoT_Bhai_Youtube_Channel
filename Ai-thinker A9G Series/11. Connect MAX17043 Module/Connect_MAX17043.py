import i2c
import time

# Global I2C Port Number
I2C_PORT = 3  # Change this to 2 or 3 as needed

# MAX17043 I2C Address
MAX17043_ADDR = 0x36

# MAX17043 Register Addresses
MAX17043_VCELL = 0x02      # Register for cell voltage
MAX17043_SOC = 0x04        # Register for state of charge
MAX17043_MODE = 0x06       # Register to send special commands
MAX17043_VERSION = 0x08    # Register for IC version
MAX17043_CONFIG = 0x0C     # Register for configuration
MAX17043_COMMAND = 0xFE    # Command register

# Default timeout for I2C operations
I2C_DEFAULT_TIME_OUT = 10  # 10 ms

# Initialize I2C
i2c.init(I2C_PORT, 100000)  # Use the global I2C_PORT variable
time.sleep(0.1)  # Allow time for device to settle

def read_register(reg_addr):
    """Read a 16-bit register from the MAX17043."""
    pData = bytes([reg_addr])
    try:
        # Transmit register address to read
        i2c.transmit(I2C_PORT, MAX17043_ADDR, pData, I2C_DEFAULT_TIME_OUT)
        time.sleep(0.1)  # Small delay
        # Receive 2 bytes of data from the MAX17043
        data = i2c.receive(I2C_PORT, MAX17043_ADDR, 2, I2C_DEFAULT_TIME_OUT)
        if data is not None:
            return (data[0] << 8) | data[1]  # Combine the two bytes
    except Exception as e:
        print("Error reading register {}: {}".format(hex(reg_addr), e))
    return None

def read_voltage():
    """Read the battery voltage from the MAX17043."""
    voltage_raw = read_register(MAX17043_VCELL)
    if voltage_raw is not None:
        voltage = (voltage_raw >> 4) * 1.25 / 1000  # Convert to volts
        return voltage
    return None

def read_soc():
    """Read the state of charge from the MAX17043."""
    soc_raw = read_register(MAX17043_SOC)
    if soc_raw is not None:
        return soc_raw / 256  # Convert to percentage
    return None

def read_version():
    """Read the IC version from the MAX17043."""
    version_raw = read_register(MAX17043_VERSION)
    if version_raw is not None:
        return version_raw
    return None

def send_command(command):
    """Send a command to the MAX17043."""
    pData = bytes([MAX17043_COMMAND, command])  # Command to send
    try:
        return i2c.transmit(I2C_PORT, MAX17043_ADDR, pData, I2C_DEFAULT_TIME_OUT)
    except Exception as e:
        print("Error sending command: {}".format(e))
        return False

def configure_device():
    """Configure the MAX17043 as needed."""
    # Example: Just read the current configuration without modifying it
    config_raw = read_register(MAX17043_CONFIG)
    if config_raw is not None:
        print("Current Configuration: {}".format(hex(config_raw)))
    else:
        print("Error reading configuration register.")

# Optional: Configure the device at startup
configure_device()

while True:
    voltage = read_voltage()
    soc = read_soc()
    version = read_version()
    
    if voltage is not None:
        print("Battery Voltage: {:.2f} V".format(voltage))
    else:
        print("Error reading battery voltage")
    
    if soc is not None:
        print("State of Charge: {:.2f}%".format(soc))
    else:
        print("Error reading state of charge")
    
    if version is not None:
        print("IC Version: {}".format(version))
    else:
        print("Error reading IC version")
    
    time.sleep(5)  # Wait for 5 seconds before the next reading
