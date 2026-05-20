import paho.mqtt.publish as publish
import time

BROKER = "localhost"  # oder IP deines Pi
TOPIC = "led/strip/color"

def set_color(r, g, b):
    msg = f"{r},{g},{b}"
    publish.single(TOPIC, msg, hostname=BROKER)
    print(f"Sent: {msg}")

# Test: Farben wechseln
set_color(255, 0, 0)   # Rot
time.sleep(1)
set_color(0, 255, 0)   # Grün
time.sleep(1)
set_color(0, 0, 255)   # Blau
time.sleep(1)
set_color(255, 255, 255)  # Weiß
time.sleep(1)
