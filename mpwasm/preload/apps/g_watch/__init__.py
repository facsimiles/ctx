import buttons
import display
import ledfx
import leds
import math
import bhi160
import time
import power
import light_sensor

disp = display.open()
sensor = 0
sensors = [{"sensor": bhi160.BHI160Orientation(sample_rate=8), "name": "Orientation"}]

DIGITS = [
    (True, True, True, True, True, True, False),
    (False, True, True, False, False, False, False),
    (True, True, False, True, True, False, True),
    (True, True, True, True, False, False, True),
    (False, True, True, False, False, True, True),
    (True, False, True, True, False, True, True),
    (True, False, True, True, True, True, True),
    (True, True, True, False, False, False, False),
    (True, True, True, True, True, True, True),
    (True, True, True, True, False, True, True),
]

DOW = ["Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"]

DEBUG_LEDS = False

led_count = 11
b7 = 255  # brightness of 7-segment display 0...255


def ceil_div(a, b):
    return (a + (b - 1)) // b


def tip_height(w):
    return ceil_div(w, 2) - 1


def draw_tip(x, y, w, c, invert=False, swapAxes=False):
    h = tip_height(w)
    for dy in range(h):
        for dx in range(dy + 1, w - 1 - dy):
            px = x + dx
            py = y + dy if not invert else y + h - 1 - dy
            if swapAxes:
                px, py = py, px
            disp.pixel(px, py, col=c)


def draw_seg(x, y, w, h, c, swapAxes=False):
    tip_h = tip_height(w)
    body_h = h - 2 * tip_h

    draw_tip(x, y, w, c, invert=True, swapAxes=swapAxes)

    px1, px2 = x, x + (w - 1)
    py1, py2 = y + tip_h, y + tip_h + (body_h - 1)
    if swapAxes:
        px1, px2, py1, py2 = py1, py2, px1, px2
    disp.rect(px1, py1, px2, py2, col=c)

    draw_tip(x, y + tip_h + body_h, w, c, invert=False, swapAxes=swapAxes)


def draw_Vseg(x, y, w, l, c):
    draw_seg(x, y, w, l, c)


def draw_Hseg(x, y, w, l, c):
    draw_seg(y, x, w, l, c, swapAxes=True)


def draw_grid_seg(x, y, w, l, c, swapAxes=False):
    sw = w - 2
    tip_h = tip_height(sw)

    x = x * w
    y = y * w
    l = (l - 1) * w
    draw_seg(x + 1, y + tip_h + 3, sw, l - 3, c, swapAxes=swapAxes)


def draw_grid_Vseg(x, y, w, l, c):
    draw_grid_seg(x, y, w, l, c)


def draw_grid_Hseg(x, y, w, l, c):
    draw_grid_seg(y, x, w, l, c, swapAxes=True)


def draw_grid(x1, y1, x2, y2, w, c):
    for x in range(x1 * w, x2 * w):
        for y in range(y1 * w, y2 * w):
            if x % w == 0 or x % w == w - 1 or y % w == 0 or y % w == w - 1:
                disp.pixel(x, y, col=c)


def draw_grid_7seg(x, y, w, segs, c):
    if segs[0]:
        draw_grid_Hseg(x, y, w, 4, c)
    if segs[1]:
        draw_grid_Vseg(x + 3, y, w, 4, c)
    if segs[2]:
        draw_grid_Vseg(x + 3, y + 3, w, 4, c)
    if segs[3]:
        draw_grid_Hseg(x, y + 6, w, 4, c)
    if segs[4]:
        draw_grid_Vseg(x, y + 3, w, 4, c)
    if segs[5]:
        draw_grid_Vseg(x, y, w, 4, c)
    if segs[6]:
        draw_grid_Hseg(x, y + 3, w, 4, c)


def render_num(num, x):
    draw_grid_7seg(x, 1, 7, DIGITS[num // 10], (b7, b7, b7))
    draw_grid_7seg(x + 5, 1, 7, DIGITS[num % 10], (b7, b7, b7))


def render_colon():
    draw_grid_Vseg(11, 2, 7, 2, (b7, b7, b7))
    draw_grid_Vseg(11, 4, 7, 2, (b7, b7, b7))


def render7segment():
    year, month, mday, hour, min, sec, wday, yday = time.localtime()

    render_num(hour, 1)
    render_num(min, 13)

    if sec % 2 == 0:
        render_colon()


with display.open() as disp:
    disp.clear().update()

    bri = 0
    threshold_angle = 35
    zn = 0

    yo = 0  # old y value
    yn = 0  # new y value
    yd = 0  # y difference
    ydl = 0  # yd lpf
    clock_on = time.monotonic_ms()  # time in ms when clock is turned on
    timeout = 7000  # time in ms how long clock will be displayed
    clock_off = clock_on + timeout  # time in ms when clock is turned off
    fade_time = 0  # fade out counter

    leds.dim_top(2)
    leds_on = 0

    while True:
        time.sleep(0.1)
        millis = time.monotonic_ms()
        # print("loop", millis)
        lt = time.localtime()
        dow = lt[6]

        # ---------------------------------------- read brightness sensor
        bri = light_sensor.get_reading()
        bri = int(
            fade_time * 100 / 1000 * bri / 200
        )  # calculate display brightness in percent (bri)

        if bri > 100:
            bri = 100
        if bri < 0:
            bri = 0

        ledbri = ((bri / 2) + 50) / 100  # calculate led bar brightness (ledbri = 0...1)

        # ---------------------------------------- read buttons
        pressed = buttons.read(buttons.BOTTOM_LEFT | buttons.BOTTOM_RIGHT)

        if DEBUG_LEDS and pressed & buttons.BOTTOM_LEFT != 0:
            leds_on = 0
            disp.clear()
            disp.print("LEDS OFF", posx=40, posy=30, font=2)
            disp.update()
            disp.backlight(brightness=50)
            time.sleep_ms(500)
            disp.backlight(brightness=0)
            for led in range(led_count):
                leds.prep_hsv(led, [0, 0, 0])
            leds.update()
            disp.update()

        if DEBUG_LEDS and pressed & buttons.BOTTOM_RIGHT != 0:
            leds_on = 1
            disp.clear()
            disp.print("LEDS ON", posx=40, posy=30, font=2)
            disp.update()
            disp.backlight(brightness=50)
            time.sleep_ms(500)
            disp.backlight(brightness=0)

        # ---------------------------------------- read orientation sensor
        samples = sensors[sensor]["sensor"].read()
        for sample in samples:
            yo = yn  # calculate absolute wrist rotation since last check
            yn = sample.y + 360
            yd = abs(yn - yo)
            yd = yd % 180
            yd = yd * 22  # multiply rotation with amplifier

            if abs(sample.z) > 50:  # if arm is hanging:
                yd = 0  # do not regard wrist rotation

            ydl = ydl * 0.9
            ydl = (yd + ydl * 9) / 10  # low pass filter wrist rotation

            if ydl > 100:  # check rottion against threshold and limit value
                ydl = 100

                if clock_on + timeout < millis:
                    clock_on = millis
                    clock_off = timeout + clock_on

        # .................................... display rotation bargraph on leds // full bar == hitting threshold

        if leds_on == 1:
            hour = lt[3]
            hue = 360 - (hour / 24 * 360)

            for led in range(led_count):
                if (led < int(ydl / 100 * 12) - 1) or millis < clock_off - 1500 - (
                    (10 - led) * 15
                ) + 300:
                    leds.prep_hsv(10 - led, [hue, 100, ledbri])  # led=0
                else:
                    leds.prep_hsv(10 - led, [0, 0, 0])
            leds.update()

        # ---------------------------------------- display clock
        if clock_off >= millis:
            disp.clear()

            # .................................... time
            lt = time.localtime()
            year = lt[0]
            month = lt[1]
            day = lt[2]
            hour = lt[3]
            mi = lt[4]
            sec = lt[5]
            dow = lt[6]

            fade_time = clock_off - millis - 1000  # calculate fade out

            if fade_time < 0:
                fade_time = 0

            if fade_time > 1000:
                fade_time = 1000

            disp.backlight(brightness=bri)

            render7segment()  # render time in 7-segment digiclock style

            disp.print(
                "{:02d}-{:02d}-{} {}".format(day, month, year, DOW[dow]),
                posx=10,
                posy=67,
                font=2,
            )  # display date

            # .................................... power
            pwr = math.sqrt(power.read_battery_voltage())

            # disp.print("%f" % power.read_battery_voltage(), posx=25, posy=58, font=2)      # display battery voltage
            full = 2.0
            empty = math.sqrt(3.4)
            pwr = pwr - empty
            full = full - empty
            pwrpercent = pwr * (100.0 / full)
            # disp.print("%f" % pwrpercent, posx=25, posy=67, font=2)                        # display battery percent

            if pwrpercent < 0:
                pwrpercent = 0

            if pwrpercent > 100:
                pwrpercent = 100

            disp.rect(8, 60, 153, 63, col=[100, 100, 100])  # draw battery bar

            c = [255, 0, 0]  # red=empty
            if pwrpercent > 10:
                c = [255, 255, 0]  # yellow=emptyish
            if pwrpercent > 25:
                c = [0, 255, 0]  # green=ok

            disp.rect(
                8, 60, int(pwrpercent * 1.43 + 8), 63, col=c
            )  # draw charge bar in battery bar
            disp.update()
