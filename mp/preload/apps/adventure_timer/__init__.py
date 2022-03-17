import json
import os
import display
import time
import buttons

CONFIG_NAME = "at-timestamp.json"


def init():
    if CONFIG_NAME not in os.listdir("."):
        at_config = {"time_start": "unset"}
        f = open(CONFIG_NAME, "w")
        f.write(json.dumps(at_config))
        f.close()

    if is_timestamp_set():
        global menu_state
        menu_state = 1
        timestamp_read()


def is_timestamp_set():
    f = open(CONFIG_NAME, "r")
    c = json.loads(f.read())
    f.close()
    if c["time_start"] == "unset":
        return False
    else:
        return True


def triangle(disp, x, y, left):
    yf = 1 if left else -1
    scale = 6
    disp.line(x - scale * yf, int(y + scale / 2), x, y)
    disp.line(x, y, x, y + scale)
    disp.line(x, y + scale, x - scale * yf, y + int(scale / 2))


def timestamp_reset():
    f = open(CONFIG_NAME, "r")
    c = json.loads(f.read())
    c["time_start"] = "unset"
    f.close()
    f = open(CONFIG_NAME, "w")
    f.write(json.dumps(c))
    f.close()


def timestamp_read():
    global time_start
    f = open(CONFIG_NAME, "r")
    c = json.loads(f.read())
    time_start = c["time_start"]
    f.close()


def timestamp_write():
    f = open(CONFIG_NAME, "r")
    c = json.loads(f.read())
    c["time_start"] = time.time()
    f.close()
    f = open(CONFIG_NAME, "w")
    f.write(json.dumps(c))
    f.close()


def headline():
    disp.print("Adventure \n Time", posy=0, fg=[0, 255, 255])


def menu():
    if menu_state == 0:
        disp.print("start?", posy=40, fg=[255, 255, 255])
        triangle(disp, 10, 66, True)
        disp.print("start.", posx=15, posy=60, fg=[0, 255, 0])
    elif menu_state == 1:
        seconds = time.time() - time_start
        m, s = divmod(seconds, 60)
        h, m = divmod(m, 60)
        disp.print("%02d:%02d:%02d" % (h, m, s), posy=40, fg=[255, 255, 255])
        triangle(disp, 10, 66, True)
        disp.print("reset", posx=15, posy=60, fg=[255, 0, 0])
    elif menu_state == 2:
        disp.print("reset?", posy=40, fg=[255, 255, 255])
        triangle(disp, 10, 66, True)
        triangle(disp, 150, 66, False)
        disp.print("no", posx=15, posy=60, fg=[255, 0, 0])
        disp.print("yes", posx=105, posy=60, fg=[0, 255, 0])


menu_state = 0
time_start = 0
disp = display.open()
button_pressed = False
init()

while True:
    disp.clear()
    headline()
    v = buttons.read(buttons.BOTTOM_LEFT | buttons.BOTTOM_RIGHT)
    if v == 0:
        button_pressed = False
    if menu_state == 0:
        if not button_pressed and v & buttons.BOTTOM_LEFT != 0:
            button_pressed = True
            timestamp_write()
            timestamp_read()
            menu_state = 1
    elif menu_state == 1:
        if not button_pressed and v & buttons.BOTTOM_LEFT != 0:
            button_pressed = True
            menu_state = 2
    elif menu_state == 2:
        if not button_pressed and v & buttons.BOTTOM_LEFT != 0:
            button_pressed = True
            menu_state = 1
        if not button_pressed and v & buttons.BOTTOM_RIGHT != 0:
            button_pressed = True
            timestamp_reset()
            menu_state = 0
    menu()
    disp.update()
    time.sleep(0.1)
