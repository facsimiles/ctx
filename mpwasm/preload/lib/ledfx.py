import leds
import math
import time


def col_cor(colors, brightness=1, gamma=1):
    """
    Gamma and  brightness correction for the RGB channels
    """
    return [
        [int(255 * brightness * math.pow((y / 255.0), gamma)) for y in x]
        for x in colors
    ]


def halo(colors):
    """
    Set the four bottom/side LEDs to colors corresponding to the color spectrum
    on the outermost of the top 11 LEDs.
    """
    used_leds = len(colors)

    # add additional RGB-Color-lists to the colors-list to fill up the top LEDs with emptiness
    colors += [[0, 0, 0]] * (11 - used_leds)

    # add four additional colors. the last one, the first one twice, the last one.
    colors += [colors[used_leds - 1]] + [colors[0]] * 2 + [colors[used_leds - 1]]
    return colors


def kitt(
    cycles=100,
    delay=80,
    power=10,
    minimum=0.3,
    rgb=[255, 0, 0],
    spectrum=[],
    halo=False,
):
    """
    LED Animation. Knight rider-Style.

    :param int cycles: Amount of cycles for the animation
    :param int delay: Time in microseconds until the animation moves on (Inverse of Framerate).
    :param int power: Shape of your brightness curve.  Bigger values make a
       steeper curve, smaller values make the curve wider.
    :param float minimum: Minimal brightness.
    :param [r,g,b] rgb: If you don't enter a spectrum this is the color used.
    :param list spectrum: A color spectrum consisting of up to 11 RGB-Value-Lists
       (e.g. ``[[255,255,255], [0,0,0], [255,255,255], ...]`` ). If you use
       less, the animation will be less wide.
    :param func halo: Halo function.  See :py:func:`ledfx.halo`.
    """

    # create a basic table of values for a smooth increment of the LED
    # brightness (if you don't understand this, don't worry, i don't either.
    # just paste it into the python shell and see the output). Basically
    # creates a negative cosinus curve.
    kitt_table = [((-math.cos(math.pi * (x / 10.0))) + 1) / 2.0 for x in range(21)]

    # adjust the values to start with a minimum brightness and the width of the
    # curve to the given power.
    kitt_table = [math.pow(x, power) * (1 - minimum) + minimum for x in kitt_table]

    # for the amount of specified cycles
    for i in range(cycles):
        # repeat every 20 steps
        j = i % 20
        # and go backwards after 10 steps
        if j > 10:
            j = 20 - j

        if spectrum == []:
            used_leds = 11

            # set the color values to the LEDs by multiplying the given color
            # value with the corresponding brightness value in the kitt table
            output = [
                [int(x * y) for y in rgb] for x in kitt_table[j : (j + used_leds)]
            ]
        else:
            used_leds = len(spectrum)

            # multiply the color values in the corresponding spectrum tuple
            # with the brightness value in the kitt table
            output = [
                [int(y * kitt_table[j + x]) for y in spectrum[x]]
                for x in range(used_leds)
            ]

        if halo:
            halo(output)

        leds.set_all(output)
        time.sleep_ms(delay)

    leds.clear()
