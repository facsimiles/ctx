import os
import display
import time
import buttons
import light_sensor
import math

WIDTH = 160
HEIGHT = 80

disp = display.open()

light_sensor.start()

history = []

while True:
    disp.clear()

    value = light_sensor.get_reading()

    history.insert(0, value)
    if len(history) > WIDTH:
        history.pop()

    disp.print("%i" % value)

    for i in range(0, len(history)):
        # Rescale to range 0 <= value < HEIGHT-1
        y = math.floor(history[i] * (HEIGHT - 2) / max(history))

        disp.pixel(WIDTH - i, HEIGHT - y - 1)

    disp.update()
    time.sleep(0.1)
