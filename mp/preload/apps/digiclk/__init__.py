import os
import sys
import leds
import display
import buttons

sys.path.append("/apps/digiclk/")
import monotime as time
import draw

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


def renderNum(d, num, x):
    draw.Grid7Seg(d, x, 0, 7, DIGITS[num // 10], (255, 255, 255))
    draw.Grid7Seg(d, x + 5, 0, 7, DIGITS[num % 10], (255, 255, 255))


def renderColon(d):
    draw.GridVSeg(d, 11, 2, 7, 2, (255, 255, 255))
    draw.GridVSeg(d, 11, 4, 7, 2, (255, 255, 255))


def renderText(d, text, blankidx=None):
    bs = bytearray(text)

    if blankidx != None:
        bs[blankidx : blankidx + 1] = b"_"

    d.print(
        MODES[MODE] + " " + bs.decode(), fg=(255, 255, 255), bg=None, posx=0, posy=7 * 8
    )


def renderBar(d, num):
    d.rect(20, 78, 20 + num * 2, 80, col=(255, 255, 255))


def render(d):
    ltime = time.localtime()
    years = ltime[0]
    months = ltime[1]
    days = ltime[2]
    hours = ltime[3]
    mins = ltime[4]
    secs = ltime[5]

    d.clear()

    if MODE == CHANGE_YEAR:
        renderNum(d, years // 100, 1)
        renderNum(d, years % 100, 13)
    elif MODE == CHANGE_MONTH:
        renderNum(d, months, 13)
    elif MODE == CHANGE_DAY:
        renderNum(d, days, 13)
    else:
        renderNum(d, hours, 1)
        renderNum(d, mins, 13)

    if MODE not in (CHANGE_YEAR, CHANGE_MONTH, CHANGE_DAY) and secs % 2 == 0:
        renderColon(d)

    renderText(d, NAME, None)
    renderBar(d, secs)

    d.update()


LONG_DELAY = 400
BUTTON_UPDATE_TIME = 100
BUTTON_SEL = 1 << 0
BUTTON_SEL_LONG = 1 << 1
BUTTON_UP = 1 << 2
BUTTON_UP_LONG = 1 << 3
BUTTON_DOWN = 1 << 4
BUTTON_DOWN_LONG = 1 << 5
pressed_prev = 0
button_long_prev = {BUTTON_SEL: False, BUTTON_UP: False, BUTTON_DOWN: False}
button_times = {BUTTON_SEL: 0, BUTTON_UP: 0, BUTTON_DOWN: 0}


def checkButton(button, button_long, osbutton, pressed, t):
    cur_buttons = 0

    if pressed & osbutton and not pressed_prev & osbutton:
        button_times[button] = t
        button_long_prev[button] = False
    elif pressed_prev & osbutton:
        if button_times[button] + LONG_DELAY < t:
            cur_buttons |= button_long
            button_times[button] = t
            button_long_prev[button] = True
        elif not pressed & osbutton and not button_long_prev[button]:
            cur_buttons |= button

    return cur_buttons


def checkButtons():
    global pressed_prev

    t = time.time_monotonic_ms()
    pressed = buttons.read(
        buttons.BOTTOM_LEFT | buttons.TOP_RIGHT | buttons.BOTTOM_RIGHT
    )
    cur_buttons = 0

    cur_buttons |= checkButton(
        BUTTON_SEL, BUTTON_SEL_LONG, buttons.BOTTOM_LEFT, pressed, t
    )
    cur_buttons |= checkButton(BUTTON_UP, BUTTON_UP_LONG, buttons.TOP_RIGHT, pressed, t)
    cur_buttons |= checkButton(
        BUTTON_DOWN, BUTTON_DOWN_LONG, buttons.BOTTOM_RIGHT, pressed, t
    )

    pressed_prev = pressed
    return cur_buttons


def modTime(yrs, mth, day, hrs, mns, sec):
    ltime = time.localtime()
    new = time.mktime(
        (
            ltime[0] + yrs,
            ltime[1] + mth,
            ltime[2] + day,
            ltime[3] + hrs,
            ltime[4] + mns,
            ltime[5] + sec,
            None,
            None,
        )
    )
    time.set_time(new)


def ctrl_display(bs):
    global MODE, updated
    updated = True
    if bs & BUTTON_SEL_LONG:
        MODE = CHANGE_HOURS
    else:
        updated = False


def ctrl_chg_hrs(bs):
    global MODE, updated
    updated = True
    if bs & BUTTON_SEL_LONG:
        MODE = DISPLAY
    elif bs & BUTTON_SEL:
        MODE = CHANGE_MINUTES
    elif bs & BUTTON_UP_LONG:
        modTime(0, 0, 0, 10, 0, 0)
    elif bs & BUTTON_DOWN_LONG:
        modTime(0, 0, 0, -10, 0, 0)
    elif bs & BUTTON_UP:
        modTime(0, 0, 0, 1, 0, 0)
    elif bs & BUTTON_DOWN:
        modTime(0, 0, 0, -1, 0, 0)
    else:
        updated = False


def ctrl_chg_mns(bs):
    global MODE, updated
    updated = True
    if bs & BUTTON_SEL_LONG:
        MODE = DISPLAY
    elif bs & BUTTON_SEL:
        MODE = CHANGE_SECONDS
    elif bs & BUTTON_UP_LONG:
        modTime(0, 0, 0, 0, 10, 0)
    elif bs & BUTTON_DOWN_LONG:
        modTime(0, 0, 0, 0, -10, 0)
    elif bs & BUTTON_UP:
        modTime(0, 0, 0, 0, 1, 0)
    elif bs & BUTTON_DOWN:
        modTime(0, 0, 0, 0, -1, 0)
    else:
        updated = False


def ctrl_chg_sec(bs):
    global MODE, updated
    updated = True
    if bs & BUTTON_SEL_LONG:
        MODE = DISPLAY
    elif bs & BUTTON_SEL:
        MODE = CHANGE_YEAR
    elif bs & BUTTON_UP_LONG:
        modTime(0, 0, 0, 0, 0, 10)
    elif bs & BUTTON_DOWN_LONG:
        modTime(0, 0, 0, 0, 0, -10)
    elif bs & BUTTON_UP:
        modTime(0, 0, 0, 0, 0, 1)
    elif bs & BUTTON_DOWN:
        modTime(0, 0, 0, 0, 0, -1)
    else:
        updated = False


def ctrl_chg_yrs(bs):
    global MODE, updated
    updated = True
    if bs & BUTTON_SEL_LONG:
        MODE = DISPLAY
    elif bs & BUTTON_SEL:
        MODE = CHANGE_MONTH
    elif bs & BUTTON_UP_LONG:
        modTime(10, 0, 0, 0, 0, 0)
    elif bs & BUTTON_DOWN_LONG:
        modTime(-10, 0, 0, 0, 0, 0)
    elif bs & BUTTON_UP:
        modTime(1, 0, 0, 0, 0, 0)
    elif bs & BUTTON_DOWN:
        modTime(-1, 0, 0, 0, 0, 0)
    else:
        updated = False


def ctrl_chg_mth(bs):
    global MODE, updated
    updated = True
    if bs & BUTTON_SEL_LONG:
        MODE = DISPLAY
    elif bs & BUTTON_SEL:
        MODE = CHANGE_DAY
    elif bs & BUTTON_UP_LONG:
        modTime(0, 6, 0, 0, 0, 0)
    elif bs & BUTTON_DOWN_LONG:
        modTime(0, -6, 0, 0, 0, 0)
    elif bs & BUTTON_UP:
        modTime(0, 1, 0, 0, 0, 0)
    elif bs & BUTTON_DOWN:
        modTime(0, -1, 0, 0, 0, 0)
    else:
        updated = False


def ctrl_chg_day(bs):
    global MODE, updated
    updated = True
    if bs & BUTTON_SEL_LONG:
        MODE = DISPLAY
    elif bs & BUTTON_SEL:
        MODE = CHANGE_HOURS
    elif bs & BUTTON_UP_LONG:
        modTime(0, 0, 10, 0, 0, 0)
    elif bs & BUTTON_DOWN_LONG:
        modTime(0, 0, -10, 0, 0, 0)
    elif bs & BUTTON_UP:
        modTime(0, 0, 1, 0, 0, 0)
    elif bs & BUTTON_DOWN:
        modTime(0, 0, -1, 0, 0, 0)
    else:
        updated = False


NAME = None
FILENAME = "nickname.txt"


def load_nickname():
    global NAME
    if FILENAME in os.listdir("."):
        with open("nickname.txt", "rb") as f:
            name = f.read().strip()
    else:
        name = b"no nick"

    if len(name) > 7:
        name = name[0:7]
    else:
        name = b" " * (7 - len(name)) + name

    NAME = name


# MODE values
DISPLAY = 0
CHANGE_HOURS = 1
CHANGE_MINUTES = 2
CHANGE_SECONDS = 3
CHANGE_YEAR = 4
CHANGE_MONTH = 5
CHANGE_DAY = 6

MODE = DISPLAY
MODES = {
    DISPLAY: "---",
    CHANGE_HOURS: "HRS",
    CHANGE_MINUTES: "MNS",
    CHANGE_SECONDS: "SEC",
    CHANGE_YEAR: "YRS",
    CHANGE_MONTH: "MTH",
    CHANGE_DAY: "DAY",
}
updated = False

CTRL_FNS = {
    DISPLAY: ctrl_display,
    CHANGE_HOURS: ctrl_chg_hrs,
    CHANGE_MINUTES: ctrl_chg_mns,
    CHANGE_SECONDS: ctrl_chg_sec,
    CHANGE_YEAR: ctrl_chg_yrs,
    CHANGE_MONTH: ctrl_chg_mth,
    CHANGE_DAY: ctrl_chg_day,
}


def main():
    global updated
    try:
        load_nickname()
        with display.open() as d:
            last_secs, secs = 0, 0
            last_msecs, msecs = 0, 0
            while True:
                updated = False

                bs = checkButtons()
                CTRL_FNS[MODE](bs)

                last_secs, secs = secs, time.time_monotonic()
                if updated or secs > last_secs:
                    render(d)

                last_msecs, msecs = msecs, time.time_monotonic_ms()
                if msecs - last_msecs < BUTTON_UPDATE_TIME:
                    time.sleep_ms(BUTTON_UPDATE_TIME - (msecs - last_msecs))
    except KeyboardInterrupt:
        pass


main()
