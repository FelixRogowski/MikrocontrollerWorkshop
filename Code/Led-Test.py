from rpi_ws281x import PixelStrip, Color
import time

# LED-Konfiguration
LED_COUNT = 30      # Anzahl der LEDs
LED_PIN = 18        # GPIO-Pin
LED_FREQ_HZ = 800000
LED_DMA = 10
LED_BRIGHTNESS = 255
LED_INVERT = False
LED_CHANNEL = 0

strip = PixelStrip(LED_COUNT, LED_PIN, LED_FREQ_HZ, LED_DMA, LED_INVERT, LED_BRIGHTNESS, LED_CHANNEL)
strip.begin()

# Alle LEDs rot leuchten lassen
for i in range(strip.numPixels()):
    strip.setPixelColor(i, Color(255, 0, 0))
strip.show()

time.sleep(1)

# Alle LEDs ausschalten
for i in range(strip.numPixels()):
    strip.setPixelColor(i, Color(0, 0, 0))
strip.show()
