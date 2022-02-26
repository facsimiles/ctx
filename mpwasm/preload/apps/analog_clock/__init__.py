# Adapted from https://github.com/muccc/flipdots/blob/master/scripts/clock.py
import display
from time import sleep
import time
import math
import leds
import buttons
import json
import os

CONFIG_NAME = "clock.json"


class Clock:
    def __init__(
        self,
        sizex=80,
        sizey=80,
        radius=38,
        offsetx=30,
        hour_hand=True,
        minute_hand=True,
        second_hand=True,
        console_out=False,
        run_once=False,
        update_interval=0,
    ):
        self.sizex = sizex
        self.sizey = sizey
        self.radius = radius
        self.center = (int(self.sizex / 2), int(self.sizey / 2))
        self.hour_hand = hour_hand
        self.minute_hand = minute_hand
        self.second_hand = second_hand
        self.console_out = console_out
        self.update_interval = (
            update_interval if update_interval != 0 else (1 if self.second_hand else 30)
        )
        self.run_once = run_once
        self.offsetx = offsetx
        self.theme = 0
        self.default_themes = [
            {
                "background": [0, 0, 0],
                "center": [255, 255, 255],
                "m1": [255, 255, 255],
                "m5": [255, 255, 255],
                "hour_hand": [255, 255, 255],
                "minute_hand": [255, 255, 255],
                "second_hand": [255, 255, 255],
            },
            {
                "background": [130, 30, 70],
                "center": [255, 255, 255],
                "m1": [255, 255, 255],
                "m5": [255, 255, 255],
                "hour_hand": [255, 255, 255],
                "minute_hand": [255, 255, 255],
                "second_hand": [255, 255, 255],
            },
            {
                "background": [0, 80, 0],
                "center": [255, 255, 255],
                "m1": [255, 255, 255],
                "m5": [255, 255, 255],
                "hour_hand": [255, 255, 255],
                "minute_hand": [255, 255, 255],
                "second_hand": [255, 255, 255],
            },
            {
                "background": [0, 80, 80],
                "center": [255, 255, 255],
                "m1": [255, 255, 255],
                "m5": [255, 255, 255],
                "hour_hand": [255, 255, 255],
                "minute_hand": [255, 255, 255],
                "second_hand": [255, 255, 255],
            },
            {
                "background": [255, 255, 255],
                "center": [0, 0, 0],
                "m1": [0, 0, 0],
                "m5": [0, 0, 0],
                "hour_hand": [0, 0, 0],
                "minute_hand": [0, 0, 0],
                "second_hand": [0, 0, 0],
            },
        ]
        self.themes = self.default_themes

        # check for config file
        if CONFIG_NAME in os.listdir("."):
            self.readConfig()
        else:
            self.writeConfig()

        # load colors
        self.setTheme(self.theme)

    def readConfig(self):
        with open(CONFIG_NAME, "r") as f:
            try:
                c = json.loads(f.read())
                if (
                    "themes" in c
                    and len(c["themes"]) > 0
                    and isinstance(c["themes"], list)
                ):
                    self.themes = c["themes"]
                if "theme" and isinstance(c["theme"], int):
                    self.theme = c["theme"]
            except ValueError:
                print("parsing %s failed" % CONFIG_NAME)

    def writeConfig(self):
        with open(CONFIG_NAME, "w") as f:
            f.write(json.dumps({"theme": self.theme, "themes": self.themes}))

    def setTheme(self, theme):
        self.theme = theme % len(self.themes)
        self.background_col = (
            self.themes[self.theme]["background"]
            if "background" in self.themes[self.theme]
            else self.default_themes[0]["background"]
        )
        self.center_col = (
            self.themes[self.theme]["center"]
            if "center" in self.themes[self.theme]
            else self.default_themes[0]["center"]
        )
        self.m1_col = (
            self.themes[self.theme]["m1"]
            if "m1" in self.themes[self.theme]
            else self.default_themes[0]["m1"]
        )
        self.m5_col = (
            self.themes[self.theme]["m5"]
            if "m5" in self.themes[self.theme]
            else self.default_themes[0]["m5"]
        )
        self.hour_hand_col = (
            self.themes[self.theme]["hour_hand"]
            if "hour_hand" in self.themes[self.theme]
            else self.default_themes[0]["hour_hand"]
        )
        self.minute_hand_col = (
            self.themes[self.theme]["minute_hand"]
            if "minute_hand" in self.themes[self.theme]
            else self.default_themes[0]["minute_hand"]
        )
        self.second_hand_col = (
            self.themes[self.theme]["second_hand"]
            if "second_hand" in self.themes[self.theme]
            else self.default_themes[0]["second_hand"]
        )

    def loop(self):
        colored = False
        try:
            with display.open() as disp:
                while True:
                    localtime = time.localtime()
                    self.updateClock(disp, localtime)
                    if self.run_once:
                        break

                    # check for button presses
                    v = buttons.read(
                        buttons.BOTTOM_LEFT | buttons.BOTTOM_RIGHT | buttons.TOP_RIGHT
                    )
                    button_pressed = v != 0

                    if button_pressed and v & buttons.BOTTOM_LEFT != 0:
                        self.setTheme(self.theme - 1)
                        self.writeConfig()
                    elif button_pressed and v & buttons.BOTTOM_RIGHT != 0:
                        self.setTheme(self.theme + 1)
                        self.writeConfig()
                    elif button_pressed and v & buttons.TOP_RIGHT != 0:
                        self.setTime(disp, localtime)

                    time.sleep_ms(23)

        except KeyboardInterrupt:
            for i in range(11):
                leds.set(i, (0, 0, 0))
            return

    def updateClock(self, disp, localtime):
        disp.clear(self.background_col)

        disp.pixel(self.center[0] + self.offsetx, self.center[1], col=self.center_col)
        hour_coords = self.circlePoint(
            math.radians(
                (((localtime[3] % 12) / 12.0) if localtime[3] else 0) * 360
                + 270
                + (localtime[4] / 2)
            )
        )
        minute_coords = self.circlePoint(math.radians(localtime[4] * 6 + 270))
        second_coords = self.circlePoint(math.radians(localtime[5] * 6 + 270))

        for i in range(60):
            degree = i * 6 + 90
            radian = -math.radians(degree)
            coords = self.circlePoint(radian)

            if not i % 5:
                self.addLine(disp, coords, self.center, 3, 1, col=self.m5_col)
            else:
                self.addLine(disp, coords, self.center, 1, col=self.m1_col)

        if self.hour_hand:
            self.addLine(
                disp,
                self.center,
                hour_coords,
                int(self.radius / 3),
                1,
                col=self.hour_hand_col,
            )
        if self.minute_hand:
            self.addLine(
                disp,
                self.center,
                minute_coords,
                int(self.radius / 2),
                col=self.minute_hand_col,
            )
        if self.second_hand:
            self.addLine(
                disp,
                self.center,
                second_coords,
                self.radius - int(self.radius / 8.0),
                col=self.second_hand_col,
            )

        if self.console_out:
            for y in range(self.radius * 2):
                line = ""
                for x in range(self.radius * 2):
                    line = line + (
                        "."
                        if image[(self.center[1] - self.radius) + y][
                            (self.center[0] - self.radius) + x
                        ]
                        else " "
                    )
                print(line)

        disp.update()

    def setTime(self, disp, localtime):
        accepted = False
        previously_pressed_button = buttons.TOP_RIGHT
        button_repeat_counter = 0
        set_seconds = time.mktime(localtime)

        while not accepted:
            v = buttons.read(
                buttons.BOTTOM_LEFT | buttons.BOTTOM_RIGHT | buttons.TOP_RIGHT
            )
            button_pressed = v != 0

            if button_pressed:
                if v & previously_pressed_button != 0:
                    button_repeat_counter += 1
                else:
                    if v & buttons.BOTTOM_LEFT != 0:
                        previously_pressed_button = buttons.BOTTOM_LEFT
                    elif v & buttons.BOTTOM_RIGHT != 0:
                        previously_pressed_button = buttons.BOTTOM_RIGHT
                    elif (
                        v & buttons.TOP_RIGHT != 0
                        and previously_pressed_button != buttons.TOP_RIGHT
                    ):
                        accepted = True
                    else:
                        previously_pressed_button = 0
            else:
                previously_pressed_button = 0
                button_repeat_counter = 0

            seconds_change = int(min(1.1 ** button_repeat_counter, 60 * 23 + 1))
            if previously_pressed_button == buttons.BOTTOM_LEFT:
                set_seconds -= seconds_change
            elif previously_pressed_button == buttons.BOTTOM_RIGHT:
                set_seconds += seconds_change

            self.updateClock(disp, time.localtime(set_seconds))
            time.sleep_ms(23)

        time.set_time(int(set_seconds))
        time.sleep_ms(123)

    def circlePoint(self, t):
        return (
            int(round(self.radius * math.cos(t))) + self.center[0],
            int(round(self.radius * math.sin(t))) + self.center[1],
        )

    def addLine(self, disp, source, aim, length, thickness=1, col=(255, 255, 255)):
        vector = self.subVector(aim, source)
        vector = self.normVector(vector)
        destination = self.addVector(source, self.multiplyVector(vector, length))

        disp.line(
            round(source[0]) + self.offsetx,
            round(source[1]),
            round(destination[0]) + self.offsetx,
            round(destination[1]),
            col=col,
            size=thickness,
        )

    def normVector(self, v):
        length = math.sqrt(sum([i ** 2 for i in v]))
        new_v = []
        for i in range(len(v)):
            new_v.append(v[i] / length)
        return tuple(new_v)

    def subVector(self, v1, v2):
        res = []
        for i in range(len(v1)):
            res.append(v1[i] - v2[i])
        return tuple(res)

    def addVector(self, v1, v2):
        res = []
        for i in range(len(v1)):
            res.append(v1[i] + v2[i])
        return tuple(res)

    def multiplyVector(self, v, multiplier):
        return tuple([i * multiplier for i in v])


clock = Clock()
clock.loop()
