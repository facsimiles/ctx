import sys_leds
import math

BOTTOM_LEFT = 11
""" ID of the LED in the bottom left corner. """
BOTTOM_RIGHT = 12
""" ID of the LED in the bottom right corner. """
TOP_RIGHT = 13
""" ID of the LED in the top right corner. """
TOP_LEFT = 14
""" ID of the LED in the top left corner. """


def update():
    """
    Updates the RGB LEDs.

    This will apply changes that have been set with :func:`leds.prep` or
    :func:`leds.prep_hsv`.  The LEDs can be only updated in bulk, so using this
    approach instead of :func:`leds.set` or :func:`leds.set_hsv` significantly
    reduces the load on the corresponding hardware bus.
    """
    sys_leds.update()


def clear():
    """
    Turns all LEDs off.

    Does **not** reactivate powersave if it has been deactivated, in which case
    ~15mA will be wasted.
    """
    values = [[0, 0, 0] for x in range(15)]
    sys_leds.set_all(values)


def set_flashlight(on):
    """
    Turn on the bright side LED.

    This LED can serve as a flashlight if worn on the left wrist or as a rad
    tattoo illuminator if worn on the right wrist.

    .. warning::

        Because of a small error in the Harmonic Board layout, we could not
        populate the flashlight LEDs in the production run.  You can handsolder
        it, though you have to reverse the direction.

    :param bool on:  Side LED on if ``True``.
    """
    sys_leds.set_flashlight(on)


def set_rocket(led, value):
    """
    Set brightness of one of the rocket LEDs.

    :param int led: Choose your rocket!

       +-------+--------+----------+
       |   ID  | Color  | Location |
       +=======+========+==========+
       | ``0`` | Blue   | Left     |
       +-------+--------+----------+
       | ``1`` | Yellow | Top      |
       +-------+--------+----------+
       | ``2`` | Green  | Right    |
       +-------+--------+----------+
    :param int value:  Brightness of LED (value between 0 and 31).
    """
    sys_leds.set_rocket(led, value)


def get_rocket(led):
    """
    Get brightness of one of the rocket LEDs.

    :param int led: Choose your rocket!

       +-------+--------+----------+
       |   ID  | Color  | Location |
       +=======+========+==========+
       | ``0`` | Blue   | Left     |
       +-------+--------+----------+
       | ``1`` | Yellow | Top      |
       +-------+--------+----------+
       | ``2`` | Green  | Right    |
       +-------+--------+----------+
    :rtype: int
    :returns: Brightness of LED (value between 0 and 31).

    .. versionadded:: 1.10
    """
    return sys_leds.get_rocket(led)


def flash_rocket(led, value, millis):
    """
    Let one of the rockets flash for a certain time without
    blocking the python interpreter.

    :param int led: Choose your rocket!
    :param int value: brightness value (0 < value < 32)
    :param int millis: duration of the rocket being on in milliseconds.

    .. versionadded:: 1.16
    """
    return sys_leds.flash_rocket(led, value, millis)


def dim_top(value):
    """
    Set global brightness for top RGB LEDs.

    :param int value: Brightness. Default = 1, Range = 1..8
    """
    sys_leds.dim_top(value)


def dim_bottom(value):
    """
    Set global brightness for bottom RGB LEDs.

    :param int value: Brightness. Default = 8, Range = 1..8
    """
    sys_leds.dim_bottom(value)


def prep(led, color):
    """
    Prepare am RGB LED to be set to an RGB value.

    Changes are applied upon calling :func:`leds.update`. This is faster than
    individual :func:`leds.set` or :func:`leds.set_hsv` calls in case of
    multiple changes.

    :param int led: Which LED to prepare. 0-10 are the LEDs on the top and
       11-14 are the 4 "ambient" LEDs
    :param [r,g,b] color: RGB triplet
    """
    sys_leds.prep(led, color)


def prep_hsv(led, color):
    """
    Prepare an RGB LED to be set to an HSV value.

    Changes are applied upon calling :func:`leds.update`. This is faster than
    individual :func:`leds.set` or :func:`leds.set_hsv` calls in case of
    multiple changes.

    :param int led: Which LED to prepare. 0-10 are the LEDs on the top and
       11-14 are the 4 "ambient" LEDs
    :param [h,s,v] color: HSV triplet
    """
    sys_leds.prep_hsv(led, color)


def set(led, color):
    """
    Set an RGB LED to an RGB value.

    :param int led: Which LED to set. 0-10 are the LEDs on the top and 11-14
       are the 4 "ambient" LEDs
    :param [r,g,b] color: RGB triplet
    """
    sys_leds.set(led, color)


def get_rgb(led):
    """
    Get the current RGB value for an RGB LED.

    :param int led: Which LED to set. 0-10 are the LEDs on the top and 11-14
       are the 4 "ambient" LEDs
    :return [r,g,b] color: RGB triplet

    .. versionadded:: 1.10
    """
    return sys_leds.get_rgb(led)


def set_hsv(led, color):
    """
    Prepare an RGB LED to be set to an HSV value.

    :param int led: Which LED to set. 0-10 are the LEDs on the top and 11-14
       are the 4 "ambient" LEDs
    :param [h,s,v] color: HSV triplet
    """
    sys_leds.set_hsv(led, color)


def set_all(colors):
    """
    Set multiple RGB LEDs to RGB values.

    Filling starts at LED0 ascending.

    **Example**:

    .. code-block:: python

       import leds, color

       # 1st red, 2nd green & 3rd blue:
       leds.set_all([color.RED, color.GREEN, color.BLUE])

    :param colors: List of RGB triplets
    """
    sys_leds.set_all(colors)


def set_all_hsv(colors):
    """
    Set multiple RGB LEDs to HSV values.

    Filling starts at LED0 ascending.

    :param colors: List of HSV triplets
    """
    sys_leds.set_all_hsv(colors)


def gay(value=0.5):
    """
    Gamma Adjust Yassistant. Prints a rainbow.

    Recommended calibration prodecure:

    .. code-block:: python

       import leds

       leds.gay(1)
       # adjust gain for uniform brightness
       leds.gamma_rgb(channel=..., gain=...)

       leds.gay(0.5)
       # adjust power~4 for uniform brightness
       leds.gamma_rgb(channel=..., power=...)

    :param value: Brightness. Default = 0.5
    """
    values = [[((x * 360.0) / 11), 1.0, value] for x in range(11)]
    sys_leds.set_all_hsv(values)


def set_powersave(eco=True):
    """
    Enable or disable powersave mode.

    Even when set to zero, the RGB LEDs still individually consume ~1mA.
    Powersave intelligently switches the supply power in groups. This
    introduces delays in the magnitude of ~10us, so it can be disabled for high
    speed applications such as POV.

    :param bool eco:  Activates powersave if ``True``, disables it when ``False``.
    """
    sys_leds.set_powersave(eco)


def set_gamma(power=4.0):
    """
    Applies same power function gamma correction to all RGB channels.

    :param float power: Exponent of power function.
    """
    table = [int(math.ceil(math.pow((x / 255.0), power) * 255)) for x in range(256)]
    for i in range(3):
        sys_leds.set_gamma(i, table)
    sys_leds.update()


def set_gamma_rgb(channel, power=4.0, gain=1.0):
    """
    Applies power function gamma correction with optional amplification to a single RGB channel.

    :param int channel: RGB channel to be adjusted. 0->Red, 1->Green, 2->Blue.
    :param float power: Exponent of power function.
    :param float gain: Amplification of channel. Values above 1.0 might cause
       overflow.
    """
    table = [
        int(math.ceil(math.pow((x / 255.0), power) * gain * 255)) for x in range(256)
    ]
    sys_leds.set_gamma(channel, table)
    sys_leds.update()
