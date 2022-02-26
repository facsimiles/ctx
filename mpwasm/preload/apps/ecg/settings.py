import color
import simple_menu
import itertools


class Settings(simple_menu.Menu):
    color_1 = color.CAMPGREEN
    color_2 = color.CAMPGREEN_DARK

    selected_options = {}

    def __init__(self):
        super().__init__([("return", False)])

    def on_select(self, value, index):
        if index == 0:
            self.exit()
        else:
            self.selected_options[value[0]] = next(value[1])
            self.write_to_file()

    def entry2name(self, value):
        if value[0] == "return":
            return value[0]
        else:
            return "{}: {}".format(value[0], self.selected_options[value[0]][0])

    def add_option(self, option):
        self.entries.append(option)
        self.selected_options[option[0]] = next(option[1])

    def get_option(self, name):
        return self.selected_options[name][1]

    def load_from_file(self):
        config_path = "/".join(__file__.split("/")[0:-1])
        try:
            f = open("{}/config.cfg".format(config_path), "r")
            for line in f:
                parts = [x.strip() for x in line.split(":")]
                if parts[0] in self.selected_options:
                    # find corresponding entry from menu to get access to the corresponding itertools.cycle
                    option_cycle = next(x for x in self.entries if x[0] == parts[0])[1]
                    if self.selected_options[parts[0]][0] != parts[1]:
                        previous = self.selected_options[parts[0]][0]
                        self.selected_options[parts[0]] = next(option_cycle)
                        while self.selected_options[parts[0]][0] not in {
                            parts[1],
                            previous,
                        }:
                            self.selected_options[parts[0]] = next(option_cycle)

                        if self.selected_options[parts[0]][0] == previous:
                            print(
                                "Settings: unknown option '{}' for key '{}'".format(
                                    parts[1], parts[0]
                                )
                            )
                else:
                    print("Settings: unknown key '{}'".format(parts[0]))
            f.close()
        except OSError:
            print("Settings could not be loaded from file. Maybe it did not exist yet?")

    def write_to_file(self):
        config_path = "/".join(__file__.split("/")[0:-1])
        try:
            f = open("{}/config.cfg".format(config_path), "w")
            for option_name in self.selected_options:
                f.write(
                    "{}:{}\n".format(option_name, self.selected_options[option_name][0])
                )
            f.close()
        except OSError as e:
            print("Settings could not be written to file! Error: {}".format(e))


def ecg_settings():
    config = Settings()
    config.add_option(
        (
            "LEDs",
            itertools.cycle(
                [
                    ("off", {}),
                    ("Pulse", {"pulse"}),
                    ("Bar", {"bar"}),
                    ("Full", {"pulse", "bar"}),
                ]
            ),
        )
    )
    config.add_option(("Mode", itertools.cycle([("Finger", "Finger"), ("USB", "USB")])))
    config.add_option(("Bias", itertools.cycle([("on", True), ("off", False)])))
    config.add_option(("Filter", itertools.cycle([("HP", {"HP"}), ("off", {})])))
    config.add_option(("Rate", itertools.cycle([("128Hz", 128), ("256Hz", 256)])))
    config.add_option(("Window", itertools.cycle([("1x", 1), ("2x", 2), ("3x", 3)])))
    config.add_option(("BLE Disp", itertools.cycle([("Off", False), ("On", True)])))
    config.add_option(
        (
            "Log",
            itertools.cycle(
                [
                    ("graph", {"graph"}),
                    ("pulse", {"pulse"}),
                    ("full", {"graph", "pulse"}),
                ]
            ),
        )
    )

    config.load_from_file()

    return config
