import machine
import time

# Built-in blue LED on the pudding board
led = machine.Pin(27, machine.Pin.OUT, 0)

def main():
    while True:
        print("LED HIGH")
        led.value(1)
        time.sleep(1)
        print("LED LOW")
        led.value(0)
        time.sleep(1)
    
