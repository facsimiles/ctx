import ucollections

_ColorTuple = ucollections.namedtuple("Color", ["red", "green", "blue"])


class Color(_ColorTuple):
    """
    A color in 24-bit RGB.

    You can create a new color from red, green, and blue::

        yellow = Color(255, 255, 0)

    You can get the HTML representation for a color using::

        html_repr = str(Color(128, 128, 128))
        assert html_repr == "#808080"

    .. py:attribute:: red

        Red component of this color.

    .. py:attribute:: green

        Green component of this color.

    .. py:attribute:: blue

        Blue component of this color.
    """

    @classmethod
    def from_hex(cls, color):
        """
        Create a color from a hexadecimal number.

        This function is available both as a class method and directly inside
        the color module:

        **Example**:

        .. code-block:: python

            from color import Color

            # Magenta
            Color.from_hex(0xff00ff)

        .. code-block:: python

            import color

            # Cyan
            color.from_hex(0x00ffff)
        """
        red = (color & 0xFF0000) >> 16
        green = (color & 0x00FF00) >> 8
        blue = color & 0x0000FF
        return cls(red, green, blue)

    @classmethod
    def from_hsv(cls, hue, saturation, value):
        """
        Create a color from a HSV tuple (hue, saturation, value).

        This function is available both as a class method and directly inside
        the color module:

        **Example**:

        .. code-block:: python

            from color import Color

            # Magenta
            Color.from_hsv(300, 1, 1)

        .. code-block:: python

            import color

            # Cyan
            color.from_hsv(180, 1, 1)

        Code via https://en.wikipedia.org/wiki/HSL_and_HSV#HSV_to_RGB_alternative

        """

        def f(n):
            k = (n + (hue / 60)) % 6
            return value - (value * saturation * max(min(k, 4 - k, 1), 0))

        def f2(x):
            return round(f(x) * 255)

        return cls(f2(5), f2(3), f2(1))

    @classmethod
    def from_hsl(cls, hue, saturation, lightness):
        """
        Create a color from a HSL tuple (hue, saturation, lightness).

        This function is available both as a class method and directly inside
        the color module:

        **Example**:

        .. code-block:: python

            from color import Color

            # Magenta
            Color.from_hsl(300, 1, 0.5)

        .. code-block:: python

            import color

            # Cyan
            color.from_hsv(180, 1, 0.5)

        Code via https://en.wikipedia.org/wiki/HSL_and_HSV#HSL_to_RGB_alternative

        """
        a = saturation * min(lightness, 1 - lightness)

        def f(n):
            k = (n + hue / 30) % 12
            return lightness - (a * max(min(k - 3, 9 - k, 1), -1))

        def f2(x):
            return round(f(x) * 255)

        return cls(f2(0), f2(8), f2(4))

    def __to_hsx(self, hsv=True):
        """
        Code via https://en.wikipedia.org/wiki/HSL_and_HSV#From_RGB
        """
        xr = self.red / 255
        xg = self.green / 255
        xb = self.blue / 255
        if xr == xg and xr == xb:
            h = 0
            c_max = xr
            c_min = xr
            x_max = self.red
            x_min = self.red
        else:
            c_max = max(xr, xg, xb)
            c_min = min(xr, xg, xb)
            x_max = max(self.red, self.green, self.blue)
            x_min = min(self.red, self.green, self.blue)
            if self.red == x_max:
                h = 60 * (0 + (xg - xb) / (c_max - c_min))
            elif self.green == x_max:
                h = 60 * (2 + (xb - xr) / (c_max - c_min))
            elif self.blue == x_max:
                h = 60 * (4 + (xr - xg) / (c_max - c_min))
            else:
                h = 0
        while h < 0:
            h = h + 360

        if hsv:
            if x_max == 0:
                s = 0
            else:
                s = (c_max - c_min) / c_max
            v = c_max
            return [round(h), round(s, 2), round(v, 2)]
        else:
            l = (c_max + c_min) / 2
            if x_max == 0:
                s = 0
            elif x_min == 1:
                s = 0
            else:
                s = (c_max - l) / min(l, 1 - l)
            return [round(h), round(s, 2), round(l, 2)]

    def to_hsv(self):
        """
        Create a HSV tuple (hue, saturation, value) from this color.

        **Example**:

        .. code-block:: python

            from color import Color

            # orange
            c = Color.from_hex(0xff9900)
            print(c.to_hsv())
            # [36, 1.0, 1.0]

        """
        return self.__to_hsx(True)

    def to_hsl(self):
        """
        Create a HSL tuple (hue, saturation, lightness) from this color.

        **Example**:

        .. code-block:: python

            from color import Color

            # orange
            c = Color.from_hex(0xff9900)
            print(c.to_hsl())
            # [36, 1.0, 0.5]

        """
        return self.__to_hsx(False)

    def __change_sat(self, change, to_fn, from_fn):
        hsx = to_fn()
        sat = hsx[1] + change
        if sat > 1:
            sat = 1
        if sat < 0:
            sat = 0
        rv = from_fn(hsx[0], sat, hsx[2])
        self.red = rv.red
        self.green = rv.green
        self.blue = rv.blue

    def __change_vlx(self, change, to_fn, from_fn):
        hsx = to_fn()
        vlx = hsx[2] + change
        if vlx > 1:
            vlx = 1
        if vlx < 0:
            vlx = 0
        rv = from_fn(hsx[0], hsx[1], vlx)
        self.red = rv.red
        self.green = rv.green
        self.blue = rv.blue

    def change_hue(self, change):
        """
        Change the hue of this color (doesn't matter if HSL or HSV).
        Hue is always between 0 and 360.

        **Example**:

        .. code-block:: python

            from color import Color

            # red
            c = Color.from_hex(0xff0000)
            print(c)
            # #ff0000
            c.change_hue(90)
            print(c)
            # #80ff00

        """
        hsx = self.to_hsl()
        hue = hsx[0] + change
        hue = hue % 360
        rv = self.from_hsl(hue, hsx[1], hsx[2])
        self.red = rv.red
        self.green = rv.green
        self.blue = rv.blue

    def change_saturation_hsv(self, change):
        """
        Change the saturation of this color (in HSV).
        Saturation is always between 0 and 1.

        **Example**:

        .. code-block:: python

            from color import Color

            # red
            c = Color.from_hex(0xff0000)
            print(c)
            # #ff0000
            c.change_saturation_hsv(-0.5)
            print(c)
            # #ff8080

        """
        return self.__change_sat(change, self.to_hsv, self.from_hsv)

    def change_saturation_hsl(self, change):
        """
        Change the saturation of this color (in HSL).
        Saturation is always between 0 and 1.

        **Example**:

        .. code-block:: python

            from color import Color

            # red
            c = Color.from_hex(0xff0000)
            print(c)
            # #ff0000
            c.change_saturation_hsl(-0.5)
            print(c)
            # #bf4040

        """
        return self.__change_sat(change, self.to_hsl, self.from_hsl)

    def change_value(self, change):
        """
        Change the value of this color (in HSV).
        Value is always between 0 and 1.

        **Example**:

        .. code-block:: python

            from color import Color

            # red
            c = Color.from_hex(0xff0000)
            print(c)
            # #ff0000
            c.change_value(-0.5)
            print(c)
            # #800000

        """
        return self.__change_vlx(change, self.to_hsv, self.from_hsv)

    def change_lightness(self, change):
        """
        Change the lightness of this color (in HSL).
        Lightness is always between 0 and 1.

        **Example**:

        .. code-block:: python

            from color import Color

            # red
            c = Color.from_hex(0xff0000)
            print(c)
            # #ff0000
            c.change_lightness(0.2)
            print(c)
            # #ff6666

        """
        return self.__change_vlx(change, self.to_hsl, self.from_hsl)

    def get_complementary(self):
        """
        Get the complementary color of this color.

        **Example**:

        .. code-block:: python

            from color import Color

            # red
            c = Color.from_hex(0xff0000)
            print(c)
            # #ff0000
            c.get_complementary()
            print(c)
            # #00ffff

        """
        rv = self.to_hsl()
        h = (rv[0] + 180) % 360
        return Color.from_hsl(round(h), round(rv[1], 2), round(rv[2], 2))

    def darken(self, change):
        """
        Darken a color. Uses change_saturation_hsl.
        """
        self.change_saturation_hsl(0 - abs(change))

    def brighten(self, change):
        """
        Brighten a color. Uses change_saturation_hsl.
        """
        self.change_saturation_hsl(0 + abs(change))

    def __get_bounded(self, r, g, b):
        if r < 0:
            r = 0
        if r > 255:
            r = 255
        if g < 0:
            g = 0
        if b > 255:
            b = 255
        return Color(r, g, b)

    def __mul__(self, other):
        """
        Multiply with a scalar to dim the color.

        **Example**:

        .. code-block:: python

            from color import Color

            # red
            c = Color.from_hex(0x0000ff)
            print(c)
            # #0000ff
            c2 = c1 * 0.5
            print(c2)
            # #000080

        """
        try:
            other = float(other)
        except ValueError:
            return NotImplemented
        r = self.red * other
        g = self.green * other
        b = self.blue * other
        return self.__get_bounded(round(r), round(g), round(b))

    def __add__(self, other):
        """
        Add two colors to add their respective color values.

        **Example**:

        .. code-block:: python

            from color import Color

            # two shades of grey
            c1 = Color.from_hex(0x323232)
            c2 = Color.from_hex(0x646464)
            print(c1)
            # #323232
            print(c2)
            # #646464
            c3 = c1 + c2
            print(c3)
            # #969696

        """
        if not isinstance(other, Color):
            return NotImplemented
        r = self.red + other.red
        g = self.green + other.green
        b = self.blue + other.blue
        return self.__get_bounded(r, g, b)

    def __sub__(self, other):
        """
        Subtract two colors to subtract their respective color values.

        **Example**:

        .. code-block:: python

            from color import Color

            # two shades of grey
            c1 = Color.from_hex(0x969696)
            c2 = Color.from_hex(0x646464)
            print(c1)
            # #969696
            print(c2)
            # #646464
            c3 = c1 - c2
            print(c3)
            # #323232

        """
        if not isinstance(other, Color):
            return NotImplemented
        r = self.red - other.red
        g = self.green - other.green
        b = self.blue - other.blue
        return self.__get_bounded(r, g, b)

    def __str__(self):
        # Return the color in hex
        return "#{:02x}{:02x}{:02x}".format(self.red, self.green, self.blue)


# fmt: off
Color.BLACK   = Color.from_hex(0x000000)
Color.WHITE   = Color.from_hex(0xffffff)
Color.RED     = Color.from_hex(0xff0000)
Color.GREEN   = Color.from_hex(0x00ff00)
Color.YELLOW  = Color.from_hex(0xffff00)
Color.BLUE    = Color.from_hex(0x0000ff)
Color.MAGENTA = Color.from_hex(0xff00ff)
Color.CYAN    = Color.from_hex(0x00ffff)

Color.CHAOSBLUE       = Color.from_hex(0x0076ba)
Color.CHAOSBLUE_DARK  = Color.from_hex(0x005383)
Color.COMMYELLOW      = Color.from_hex(0xffc600)
Color.COMMYELLOW_DARK = Color.from_hex(0xd39a00)
Color.CAMPGREEN       = Color.from_hex(0x99ba00)
Color.CAMPGREEN_DARK  = Color.from_hex(0x6f8700)
# fmt: on


# Add the colors and constructors to the module for convenience
# This allows you to do the following:
#
#    import colors
#
#    colors.BLACK
#    colors.from_hex(0xc6c6c6)
globals().update(
    {n: getattr(Color, n) for n in dir(Color) if n.startswith("from") or n.isupper()}
)
