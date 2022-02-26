import buttons
import color
import display
import sys
import time
import config

TIMEOUT = 0x100
""":py:func:`~simple_menu.button_events` timeout marker."""
LONG_PRESS_MS = 1000
RETRIGGER_MS = 250

try:
    LONG_PRESS_MS = int(config.get_string("long_press_ms"))
except Exception:
    pass

try:
    RETRIGGER_MS = int(config.get_string("retrigger_ms"))
except Exception:
    pass


def button_events(timeout=None, long_press_ms=LONG_PRESS_MS, retrigger_ms=RETRIGGER_MS):
    """
    Iterate over button presses (event-loop).

    This is just a helper function used internally by the menu.  But you can of
    course use it for your own scripts as well.  It works like this:

    .. code-block:: python

        import simple_menu, buttons

        for ev in simple_menu.button_events():
            if ev == buttons.BOTTOM_LEFT:
                # Left
                pass
            elif ev == buttons.BOTTOM_RIGHT:
                # Right
                pass
            elif ev == buttons.TOP_RIGHT:
                # Select
                pass

    .. versionadded:: 1.4

    :param float,optional timeout:
       Timeout after which the generator should yield in any case.  If a
       timeout is defined, the generator will periodically yield
       :py:data:`simple_menu.TIMEOUT`.

       .. versionadded:: 1.9

    :param int,optional long_press_ms: Time for long key press in ms.
       .. versionadded:: 1.17
    :param int,optional retrigger_ms: Time for repeating key press on hold in ms.
       .. versionadded:: 1.17

    """
    yield 0

    v = buttons.read(buttons.BOTTOM_LEFT | buttons.BOTTOM_RIGHT | buttons.TOP_RIGHT)
    button_pressed = True if v != 0 else False
    t0 = time.time_ms()
    still_pressed = False

    if timeout is not None:
        timeout = int(timeout * 1000)
        next_tick = time.time_ms() + timeout

    while True:
        if timeout is not None:
            current_time = time.time_ms()
            if current_time >= next_tick:
                next_tick += timeout
                yield TIMEOUT

        v = buttons.read(buttons.BOTTOM_LEFT | buttons.BOTTOM_RIGHT | buttons.TOP_RIGHT)

        if v == 0:
            button_pressed = False
            still_pressed = False
        else:
            if not button_pressed:
                button_pressed = True
                t0 = time.time_ms()

                if v & buttons.BOTTOM_LEFT:
                    yield buttons.BOTTOM_LEFT

                if v & buttons.BOTTOM_RIGHT:
                    yield buttons.BOTTOM_RIGHT

                if v & buttons.TOP_RIGHT:
                    yield buttons.TOP_RIGHT

            if (
                not still_pressed
                and long_press_ms
                and ((time.time_ms() - t0) <= long_press_ms)
            ):
                pass
            else:
                if retrigger_ms and time.time_ms() - t0 > retrigger_ms:
                    button_pressed = False
                    still_pressed = True


class _ExitMenuException(Exception):
    pass


class Menu:
    """
    A simple menu for card10.

    This menu class is supposed to be inherited from to create a menu as shown
    in the example above.

    To instanciate the menu, pass a list of entries to the constructor:

    .. code-block:: python

        m = Menu(os.listdir("."))
        m.run()

    Then, call :py:meth:`~simple_menu.Menu.run` to start the event loop.

    .. versionadded:: 1.4
    """

    color_1 = color.CHAOSBLUE
    """Background color A."""
    color_2 = color.CHAOSBLUE_DARK
    """Background color B."""
    color_text = color.WHITE
    """Text color."""
    color_sel = color.COMMYELLOW
    """Color of the selector."""

    scroll_speed = 0.5
    """
    Time to wait before scrolling to the right.

    .. versionadded:: 1.9
    """

    timeout = None
    """
    Optional timeout for inactivity.  Once this timeout is reached,
    :py:meth:`~simple_menu.Menu.on_timeout` will be called.

    .. versionadded:: 1.9
    """

    right_buttons_scroll = False
    """
    Use top right and bottom right buttons to move in the list
    instead of bottom left and bottom right buttons.

    .. versionadded:: 1.13
    """

    def on_scroll(self, item, index):
        """
        Hook when the selector scrolls to a new item.

        This hook is run everytime a scroll-up or scroll-down is performed.
        Overwrite this function in your own menus if you want to do some action
        every time a new item is scrolled onto.

        :param item: The item which the selector now points to.
        :param int index: Index into the ``entries`` list of the ``item``.
        """
        pass

    def on_long_select(self, item, index):
        """
        Hook when an item as selected using a long press.

        The given ``index`` was selected with a long SELECT button press.  Overwrite
        this function in your menu to perform an action on select.

        :param item: The item which was selected.
        :param int index: Index into the ``entries`` list of the ``item``.
        """
        pass

    def on_select(self, item, index):
        """
        Hook when an item as selected.

        The given ``index`` was selected with a SELECT button press.  Overwrite
        this function in your menu to perform an action on select.

        :param item: The item which was selected.
        :param int index: Index into the ``entries`` list of the ``item``.
        """
        pass

    def on_timeout(self):
        """
        The inactivity timeout has been triggered.  See
        :py:attr:`simple_menu.Menu.timeout`.

        .. versionadded:: 1.9
        """
        self.exit()

    def exit(self):
        """
        Exit the event-loop.  This should be called from inside an ``on_*`` hook.

        .. versionadded:: 1.9
        .. versionchanged:: 1.11

            Fixed this function not working properly.
        """
        raise _ExitMenuException()

    def __init__(self, entries):
        if len(entries) == 0:
            raise ValueError("at least one entry is required")

        self.entries = entries
        self.idx = 0
        self.select_time = time.time_ms()
        self.disp = display.open()
        try:
            right_scroll_str = config.get_string("right_scroll")
            if right_scroll_str.lower() in ["true", "1"]:
                right_buttons_scroll = True
            if right_scroll_str.lower() in ["false", "0"]:
                right_buttons_scroll = False

        except OSError:
            right_buttons_scroll = self.right_buttons_scroll

        self.button_scroll_up = (
            buttons.TOP_RIGHT if right_buttons_scroll else buttons.BOTTOM_LEFT
        )
        self.button_select = (
            buttons.BOTTOM_LEFT if right_buttons_scroll else buttons.TOP_RIGHT
        )

    def entry2name(self, value):
        """
        Convert an entry object to a string representation.

        Overwrite this functio if your menu items are not plain strings.

        **Example**:

        .. code-block:: python

            class MyMenu(simple_menu.Menu):
                def entry2name(self, value):
                    return value[0]

            MyMenu(
                [("a", 123), ("b", 321)]
            ).run()
        """
        return str(value)

    def draw_entry(self, value, index, offset):
        """
        Draw a single entry.

        This is an internal function; you can override it for customized behavior.

        :param value: The value for this entry.  Use this to identify
            different entries.
        :param int index: A unique index per entry. Stable for a certain entry,
            but **not** an index into ``entries``.
        :param int offset: Y-offset for this entry.
        """
        string = self.entry2name(value)

        if offset == 20 and len(string) >= 10:
            # Slowly scroll entry to the side
            time_offset = (time.time_ms() - self.select_time) // int(
                self.scroll_speed * 1000
            )
            time_offset = time_offset % (len(string) - 7) - 1
            time_offset = min(len(string) - 10, max(0, time_offset))
            string = string[time_offset:]

        self.disp.rect(
            0,
            offset,
            180,
            offset + 20,
            col=self.color_1 if index % 2 == 0 else self.color_2,
        )
        self.disp.print(string, posx=14, posy=offset, fg=self.color_text, bg=None)

    def draw_menu(self, offset=0):
        """
        Draw the menu.

        You'll probably never need to call this yourself; it is called
        automatially in the event loop (:py:meth:`~simple_menu.Menu.run`).
        """
        self.disp.clear()

        # Wrap around the list and draw entries from idx - 3 to idx + 4
        for y, i in enumerate(
            range(len(self.entries) + self.idx - 3, len(self.entries) + self.idx + 4)
        ):
            self.draw_entry(
                self.entries[i % len(self.entries)], i, offset + y * 20 - 40
            )

        self.disp.line(4, 22, 11, 29, col=self.color_sel, size=2)
        self.disp.line(4, 36, 11, 29, col=self.color_sel, size=2)

        self.disp.update()

    def error(self, line1, line2=""):
        """
        Display an error message.

        :param str line1: First line of the error message.
        :param str line2: Second line of the error message.

        .. versionadded:: 1.9
        """
        self.disp.clear(color.COMMYELLOW)

        offset = max(0, (160 - len(line1) * 14) // 2)
        self.disp.print(
            line1, posx=offset, posy=20, fg=color.COMMYELLOW_DARK, bg=color.COMMYELLOW
        )

        offset = max(0, (160 - len(line2) * 14) // 2)
        self.disp.print(
            line2, posx=offset, posy=40, fg=color.COMMYELLOW_DARK, bg=color.COMMYELLOW
        )

        self.disp.update()

    def run(self, long_press_ms=LONG_PRESS_MS, retrigger_ms=RETRIGGER_MS):
        """
        Start the event-loop.

        :param int,optional long_press_ms: Time for long key press in ms.
            .. versionadded:: 1.17
        :param int,optional retrigger_ms: Time for repeating key press on hold in ms.
            .. versionadded:: 1.17
        """
        try:
            timeout = self.scroll_speed
            if self.timeout is not None and self.timeout < self.scroll_speed:
                timeout = self.timeout

            for ev in button_events(timeout, long_press_ms, retrigger_ms):
                if ev == buttons.BOTTOM_RIGHT:
                    self.select_time = time.time_ms()
                    self.draw_menu(-8)
                    self.idx = (self.idx + 1) % len(self.entries)
                    try:
                        self.on_scroll(self.entries[self.idx], self.idx)
                    except _ExitMenuException:
                        raise
                    except Exception as e:
                        print("Exception during menu.on_scroll():")
                        sys.print_exception(e)
                elif ev == self.button_scroll_up:
                    self.select_time = time.time_ms()
                    self.draw_menu(8)
                    self.idx = (self.idx + len(self.entries) - 1) % len(self.entries)
                    try:
                        self.on_scroll(self.entries[self.idx], self.idx)
                    except _ExitMenuException:
                        raise
                    except Exception as e:
                        print("Exception during menu.on_scroll():")
                        sys.print_exception(e)
                elif ev == self.button_select:
                    t0 = time.time_ms()
                    long_press = False
                    while buttons.read(self.button_select) > 0:
                        if time.time_ms() - t0 > long_press_ms:
                            long_press = True
                            break

                    try:
                        if long_press:
                            self.on_long_select(self.entries[self.idx], self.idx)
                        else:
                            self.on_select(self.entries[self.idx], self.idx)
                        self.select_time = time.time_ms()
                    except _ExitMenuException:
                        raise
                    except Exception as e:
                        print("Menu crashed!")
                        sys.print_exception(e)
                        self.error("Menu", "crashed")
                        time.sleep(1.0)

                self.draw_menu()

                if self.timeout is not None and (
                    time.time_ms() - self.select_time
                ) > int(self.timeout * 1000):
                    self.on_timeout()
        except _ExitMenuException:
            pass
