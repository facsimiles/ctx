import display
import leds
import time

_rand = 123456789


def rand():
    global _rand
    _rand = (1103515245 * _rand + 12345) & 0xFFFFFF
    return _rand


gs = 160
colors = [((i >> 2) * gs, (i >> 1 & 1) * gs, (i & 1) * gs) for i in range(1, 8)]

nick = "sample text"
try:
    with open("/nickname.txt") as f:
        nick = f.read()
except:
    pass

while True:
    with display.open() as d:
        for k in range(4):
            (x1, y1) = (rand() % 159, rand() % 79)
            (x2, y2) = (min(x1 + rand() % 40, 159), min(y1 + rand() % 40, 79))
            try:
                d.rect(x1, y1, x2, y2, col=colors[rand() % len(colors)], filled=True)
            except:
                pass
        fg = colors[rand() % len(colors)]
        nx = 80 - round(len(nick) / 2 * 14)
        d.print(
            nick,
            fg=fg,
            bg=[0xFF - c for c in fg],
            posx=(nx - 8) + rand() % 16,
            posy=22 + rand() % 16,
        )
        d.update()
        d.close()
    leds.set(rand() % 11, colors[rand() % len(colors)])
    leds.set_rocket(rand() % 3, rand() % 32)
    time.sleep_us(1)  # Feed watch doge
