import i2c
import time

# Global I2C Port Number
I2C_PORT = 2  # Change this to 2 or 3 as needed

# Initialize I2C with the desired parameters
i2c.init(I2C_PORT, 100000)

def scan_i2c():
    print("Scanning I2C bus...")
    for addr in range(0x01, 0x78):
        try:
            i2c.receive(I2C_PORT, addr, 1, 10)  # Attempt to read from the address
            print("I2C device found at address: 0x{:02X}".format(addr))
        except Exception as e:
            pass  # You can optionally log the exception if needed

# Continuously scan I2C bus
while True:
    scan_i2c()
    time.sleep(5)  # Delay between scans (adjust as needed)
