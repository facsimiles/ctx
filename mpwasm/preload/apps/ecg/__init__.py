import os
import display
import leds
import time
import buttons
import max30001
import math
import struct
import itertools
import bluetooth
import ecg.settings

config = ecg.settings.ecg_settings()

WIDTH = 160
HEIGHT = 80
OFFSET_Y = 49
HISTORY_MAX = WIDTH * 4
DRAW_AFTER_SAMPLES = 5
SCALE_FACTOR = 30
MODE_USB = "USB"
MODE_FINGER = "Finger"
FILEBUFFERBLOCK = 4096
COLOR_BACKGROUND = [0, 0, 0]
COLOR_LINE = [255, 255, 255]
COLOR_TEXT = [255, 255, 255]
COLOR_MODE_FINGER = [0, 255, 0]
COLOR_MODE_USB = [0, 0, 255]
COLOR_WRITE_FG = [255, 255, 255]
COLOR_WRITE_BG = [255, 0, 0]
COLORS = [((23 + (15 * i)) % 360, 1.0, 1.0) for i in range(11)]

_IRQ_CENTRAL_CONNECT = const(1)
_IRQ_CENTRAL_DISCONNECT = const(2)
_IRQ_GATTS_WRITE = const(3)


class ECG:
    history = []

    # variables for file output
    filebuffer = bytearray()
    write = 0
    write_time_string = ""
    samples_since_start_of_write = 0

    update_screen = 0
    pause_screen = 0
    pause_graph = False
    graph_offset = 0
    sensor = None
    disp = display.open()
    last_sample_count = 1

    # variables for high-pass filter
    # note: corresponds to 1st order hpf with -3dB at ~18.7Hz
    # general formula: f(-3dB)=-(sample_rate/tau)*ln(1-betadash)
    moving_average = 0
    alpha = 2
    beta = 3
    betadash = beta / (alpha + beta)

    # variables for pulse detection
    pulse = -1
    samples_since_last_pulse = 0
    last_pulse_blink = 0
    q_threshold = -1
    r_threshold = 1
    q_spike = -500  # just needs to be long ago

    def ble_irq(self, event, data):
        if event == _IRQ_CENTRAL_CONNECT:
            print("BLE Connected")
        elif event == _IRQ_CENTRAL_DISCONNECT:
            print("BLE Disconnected")
            self.ble_streaming = False
            if not config.get_option("BLE Disp"):
                self.disp.backlight(20)
        elif event == _IRQ_GATTS_WRITE:
            conn_handle, value_handle = data
            if value_handle == self.ecg_cccd_handle:
                value = self.b.gatts_read(value_handle)
                print("New cccd value:", value)
                # Value of 0 means notifcations off
                if value == b"\x00\x00":
                    self.ble_streaming = False
                    if not config.get_option("BLE Disp"):
                        self.disp.backlight(20)
                else:
                    self.ble_streaming = True
                    self.ble_sample_count = 0
                    if not config.get_option("BLE Disp"):
                        self.disp.backlight(0)

    def __init__(self):
        try:
            self.b = bluetooth.BLE()
            self.b.active(True)
            self.b.irq(self.ble_irq)

            _ECG_UUID = bluetooth.UUID("42230300-2342-2342-2342-234223422342")
            _ECG_DATA = (
                bluetooth.UUID("42230301-2342-2342-2342-234223422342"),
                bluetooth.FLAG_READ | bluetooth.FLAG_NOTIFY,
            )

            _ECG_SERVICE = (_ECG_UUID, (_ECG_DATA,))
            ((self.ecg_data_handle,),) = self.b.gatts_register_services((_ECG_SERVICE,))
            self.ecg_cccd_handle = self.ecg_data_handle + 1

            # Disable streaming by default
            self.ble_streaming = False
            self.b.gatts_write(self.ecg_cccd_handle, "\x00\x00")
        except OSError:
            self.ble_streaming = False
            pass

        leds.dim_top(1)

    def update_history(self, datasets):
        self.last_sample_count = len(datasets)
        for val in datasets:
            if "HP" in config.get_option("Filter"):
                self.history.append(val - self.moving_average)
                self.moving_average += self.betadash * (val - self.moving_average)
                # identical to: moving_average = (alpha * moving_average + beta * val) / (alpha + beta)
            else:
                self.history.append(val)

        # trim old elements
        self.history = self.history[-HISTORY_MAX:]

    def detect_pulse(self, num_new_samples):
        # look at 3 consecutive samples, starting 2 samples before the samples that were just added, e.g.:
        # existing samples: "ABCD"
        # new samples: "EF" => "ABCDEF"
        # consider ["CDE", "DEF"]
        # new samples: "GHI" => "ABCDEFGHI"
        # consider ["EFG", "FGH", "GHI"]
        ecg_rate = config.get_option("Rate")

        def neighbours(n, lst):
            """
            neighbours(2, "ABCDE") = ("AB", "BC", "CD", "DE")
            neighbours(3, "ABCDE") = ("ABC", "BCD", "CDE")
            """

            for i in range(len(lst) - (n - 1)):
                yield lst[i : i + n]

        for [prev, cur, next_] in neighbours(3, self.history[-(num_new_samples + 2) :]):
            self.samples_since_last_pulse += 1

            if prev > cur < next_ and cur < self.q_threshold:
                self.q_spike = self.samples_since_last_pulse
                # we expect the next q-spike to be at least 60% as high as this one
                self.q_threshold = (cur * 3) // 5
            elif (
                prev < cur > next_
                and cur > self.r_threshold
                and self.samples_since_last_pulse - self.q_spike < ecg_rate // 10
            ):
                # the full QRS complex is < 0.1s long, so the q and r spike in particular cannot be more than ecg_rate//10 samples apart
                self.pulse = 60 * ecg_rate // self.samples_since_last_pulse
                self.q_spike = -ecg_rate
                if self.pulse < 30 or self.pulse > 210:
                    self.pulse = -1
                elif self.write > 0 and "pulse" in config.get_option("Log"):
                    self.write_pulse()
                # we expect the next r-spike to be at least 60% as high as this one
                self.r_threshold = (cur * 3) // 5
                self.samples_since_last_pulse = 0
            elif self.samples_since_last_pulse > 2 * ecg_rate:
                self.q_threshold = -1
                self.r_threshold = 1
                self.pulse = -1

    def callback_ecg(self, datasets):
        if len(datasets) == 0:
            return

        if self.ble_streaming:
            try:
                self.b.gatts_notify(
                    1,
                    self.ecg_data_handle,
                    struct.pack(">h", self.ble_sample_count & 0xFFFF)
                    + struct.pack(">" + ("h" * len(datasets)), *datasets),
                )
            except OSError:
                pass

            # We count all samples, even if we failed to send them
            self.ble_sample_count += len(datasets)

            # Don't update the screen if it should be off during a connection
            if not config.get_option("BLE Disp"):
                return

        self.update_screen += len(datasets)
        if self.write > 0:
            self.samples_since_start_of_write += len(datasets)

        # update graph datalist
        if not self.pause_graph:
            self.update_history(datasets)
            self.detect_pulse(len(datasets))

        # buffer for writes
        if self.write > 0 and "graph" in config.get_option("Log"):
            for value in datasets:
                self.filebuffer.extend(struct.pack("h", value))
                if len(self.filebuffer) >= FILEBUFFERBLOCK:
                    self.write_filebuffer()

        # don't update on every callback
        if self.update_screen >= DRAW_AFTER_SAMPLES:
            self.draw_graph()

    def append_to_file(self, fileprefix, content):
        # write to file
        filename = "/ecg_logs/{}-{}.log".format(fileprefix, self.write_time_string)

        # write stuff to disk
        try:
            f = open(filename, "ab")
            f.write(content)
            f.close()
        except OSError as e:
            print("Please check the file or filesystem", e)
            self.write = 0
            self.pause_screen = -1
            self.disp.clear(COLOR_BACKGROUND)
            self.disp.print("IO Error", posy=0, fg=COLOR_TEXT)
            self.disp.print("Please check", posy=20, fg=COLOR_TEXT)
            self.disp.print("your", posy=40, fg=COLOR_TEXT)
            self.disp.print("filesystem", posy=60, fg=COLOR_TEXT)
            self.disp.update()
            self.close_sensor()
        except:
            print("Unexpected error, stop writing logfile")
            self.write = 0

    def write_pulse(self):
        # estimates timestamp as calls to time.time() take too much time
        approx_timestamp = (
            self.write + self.samples_since_start_of_write // config.get_option("Rate")
        )
        self.append_to_file("pulse", struct.pack("ib", approx_timestamp, self.pulse))

    def write_filebuffer(self):
        self.append_to_file("ecg", self.filebuffer)
        self.filebuffer = bytearray()

    def open_sensor(self):
        self.sensor = max30001.MAX30001(
            usb=(config.get_option("Mode") == "USB"),
            bias=config.get_option("Bias"),
            sample_rate=config.get_option("Rate"),
            callback=self.callback_ecg,
        )

    def close_sensor(self):
        self.sensor.close()

    def toggle_write(self):
        self.pause_screen = time.time_ms() + 1000
        self.disp.clear(COLOR_BACKGROUND)
        if self.write > 0:
            self.write_filebuffer()
            self.write = 0
            self.disp.print("Stop", posx=50, posy=20, fg=COLOR_TEXT)
            self.disp.print("logging", posx=30, posy=40, fg=COLOR_TEXT)
        else:
            self.filebuffer = bytearray()
            self.write = time.time()
            lt = time.localtime(self.write)
            self.write_time_string = "{:04d}-{:02d}-{:02d}_{:02d}{:02d}{:02d}".format(
                *lt
            )
            self.samples_since_start_of_write = 0
            try:
                os.mkdir("ecg_logs")
            except:
                pass
            self.disp.print("Start", posx=45, posy=20, fg=COLOR_TEXT)
            self.disp.print("logging", posx=30, posy=40, fg=COLOR_TEXT)

        self.disp.update()

    def toggle_pause(self):
        if self.pause_graph:
            self.pause_graph = False
            self.history = []
        else:
            self.pause_graph = True
        self.graph_offset = 0
        leds.clear()

    def draw_leds(self, vmin, vmax):
        # vmin should be in [0, -1]
        # vmax should be in [0, 1]

        led_mode = config.get_option("LEDs")

        # stop blinking
        if not bool(led_mode):
            return

        # update led bar
        if "bar" in led_mode:
            for i in reversed(range(6)):
                leds.prep_hsv(
                    5 + i, COLORS[5 + i] if vmin <= 0 and i <= vmin * -6 else (0, 0, 0)
                )
            for i in reversed(range(6)):
                leds.prep_hsv(
                    i, COLORS[i] if vmax >= 0 and 5 - i <= vmax * 6 else (0, 0, 0)
                )

        # blink red on pulse
        if (
            "pulse" in led_mode
            and self.pulse > 0
            and self.samples_since_last_pulse < self.last_pulse_blink
        ):
            for i in range(4):
                leds.prep(11 + i, (255, 0, 0))
        elif "pulse" in led_mode:
            for i in range(4):
                leds.prep(11 + i, (0, 0, 0))
        self.last_pulse_blink = self.samples_since_last_pulse

        leds.update()

    def draw_graph(self):
        # skip rendering due to message beeing shown
        if self.pause_screen == -1:
            return
        elif self.pause_screen > 0:
            t = time.time_ms()
            if t > self.pause_screen:
                self.pause_screen = 0
            else:
                return

        self.disp.clear(COLOR_BACKGROUND)

        # offset in pause_graph mode
        timeWindow = config.get_option("Window")
        window_end = int(len(self.history) - self.graph_offset)
        s_end = max(0, window_end)
        s_start = max(0, s_end - WIDTH * timeWindow)

        # get max value and calc scale
        value_max = max(abs(x) for x in self.history[s_start:s_end])
        scale = SCALE_FACTOR / (value_max if value_max > 0 else 1)

        # draw graph
        # values need to be inverted so high values are drawn with low pixel coordinates (at the top of the screen)
        draw_points = (int(-x * scale + OFFSET_Y) for x in self.history[s_start:s_end])

        prev = next(draw_points)
        for x, value in enumerate(draw_points):
            self.disp.line(
                x // timeWindow, prev, (x + 1) // timeWindow, value, col=COLOR_LINE
            )
            prev = value

        # draw text: mode/bias/write
        if self.pause_graph:
            self.disp.print(
                "Pause"
                + (
                    " -{:0.1f}s".format(self.graph_offset / config.get_option("Rate"))
                    if self.graph_offset > 0
                    else ""
                ),
                posx=0,
                posy=0,
                fg=COLOR_TEXT,
            )
        else:
            led_range = self.last_sample_count if self.last_sample_count > 5 else 5
            self.draw_leds(
                min(self.history[-led_range:]) / value_max,
                max(self.history[-led_range:]) / value_max,
            )
            if self.pulse < 0:
                self.disp.print(
                    config.get_option("Mode")
                    + ("+Bias" if config.get_option("Bias") else ""),
                    posx=0,
                    posy=0,
                    fg=(
                        COLOR_MODE_FINGER
                        if config.get_option("Mode") == MODE_FINGER
                        else COLOR_MODE_USB
                    ),
                )
            else:
                self.disp.print(
                    "BPM: {}".format(self.pulse),
                    posx=0,
                    posy=0,
                    fg=(
                        COLOR_MODE_FINGER
                        if config.get_option("Mode") == MODE_FINGER
                        else COLOR_MODE_USB
                    ),
                )

        # announce writing ecg log
        if self.write > 0:
            t = time.time()
            if self.write > 0 and t % 2 == 0:
                self.disp.print(
                    "LOG", posx=0, posy=60, fg=COLOR_WRITE_FG, bg=COLOR_WRITE_BG
                )

        self.disp.update()
        self.update_screen = 0

    def main(self):
        # show button layout
        self.disp.clear(COLOR_BACKGROUND)
        self.disp.print(
            "  BUTTONS ", posx=0, posy=0, fg=COLOR_TEXT, font=display.FONT20
        )
        self.disp.line(0, 20, 159, 20, col=COLOR_LINE)
        self.disp.print(
            "       Pause >", posx=0, posy=28, fg=COLOR_MODE_FINGER, font=display.FONT16
        )
        self.disp.print(
            "    Settings >", posx=0, posy=44, fg=COLOR_MODE_USB, font=display.FONT16
        )
        self.disp.print(
            "< WriteLog    ", posx=0, posy=64, fg=COLOR_WRITE_BG, font=display.FONT16
        )
        self.disp.update()
        time.sleep(3)

        # start ecg
        self.open_sensor()
        while True:
            button_pressed = {"BOTTOM_LEFT": 0, "BOTTOM_RIGHT": 0, "TOP_RIGHT": 0}
            while True:
                v = buttons.read(
                    buttons.BOTTOM_LEFT | buttons.BOTTOM_RIGHT | buttons.TOP_RIGHT
                )

                # TOP RIGHT
                #
                # pause

                # down
                if button_pressed["TOP_RIGHT"] == 0 and v & buttons.TOP_RIGHT != 0:
                    button_pressed["TOP_RIGHT"] = 1
                    self.toggle_pause()

                # up
                if button_pressed["TOP_RIGHT"] > 0 and v & buttons.TOP_RIGHT == 0:
                    button_pressed["TOP_RIGHT"] = 0

                # BOTTOM LEFT
                #
                # on pause = shift view left
                # else = toggle write

                # down
                if button_pressed["BOTTOM_LEFT"] == 0 and v & buttons.BOTTOM_LEFT != 0:
                    button_pressed["BOTTOM_LEFT"] = 1
                    if self.pause_graph:
                        l = len(self.history)
                        self.graph_offset += config.get_option("Rate") / 2
                        if l - self.graph_offset < WIDTH * config.get_option("Window"):
                            self.graph_offset = l - WIDTH * config.get_option("Window")
                    else:
                        self.toggle_write()

                # up
                if button_pressed["BOTTOM_LEFT"] > 0 and v & buttons.BOTTOM_LEFT == 0:
                    button_pressed["BOTTOM_LEFT"] = 0

                # BOTTOM RIGHT
                #
                # on pause = shift view right
                # else = show settings

                # down
                if (
                    button_pressed["BOTTOM_RIGHT"] == 0
                    and v & buttons.BOTTOM_RIGHT != 0
                ):
                    button_pressed["BOTTOM_RIGHT"] = 1
                    if self.pause_graph:
                        self.graph_offset -= config.get_option("Rate") / 2
                        self.graph_offset -= self.graph_offset % (
                            config.get_option("Rate") / 2
                        )
                        if self.graph_offset < 0:
                            self.graph_offset = 0
                    else:
                        self.pause_screen = -1  # hide graph
                        leds.clear()  # disable all LEDs
                        if self.ble_streaming and not config.get_option("BLE Disp"):
                            self.disp.backlight(20)

                        config.run()  # show config menu

                        if self.ble_streaming and not config.get_option("BLE Disp"):
                            self.disp.backlight(0)
                        self.close_sensor()  # reset sensor in case mode or bias was changed TODO do not close sensor otherwise?
                        self.open_sensor()
                        self.pause_screen = 0  # start plotting graph again
                        # returning from menu was by pressing the TOP_RIGHT button
                        button_pressed["TOP_RIGHT"] = 1

                # up
                if button_pressed["BOTTOM_RIGHT"] > 0 and v & buttons.BOTTOM_RIGHT == 0:
                    button_pressed["BOTTOM_RIGHT"] = 0


if __name__ == "__main__":
    ecg = ECG()
    try:
        ecg.main()
    except KeyboardInterrupt as e:
        ecg.close_sensor()
        raise e
