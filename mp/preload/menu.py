"""
Menu Script
===========
You can customize this script however you want :)  If you want to go back to
the default version, just delete this file; the firmware will recreate it on
next run.
"""
import buttons
import collections
import color
import display
import os
import simple_menu
import sys
import json
import time

App = collections.namedtuple("App", ["name", "path"])

# Favorite apps which are shown at the very top of the app list
FAVORITE_APPS = ["personal_state", "ecg"]


def enumerate_entries():
    for f in os.listdir("/"):
        if f == "main.py":
            yield App("Home", f)

    yield App("USB Storage", "USB_STORAGE_FLAG")

    yield from enumerate_apps(FAVORITE_APPS)

    yield from sorted(enumerate_apps(), key=lambda b: b.name.lower())


def enumerate_apps(apps=None):
    """List all installed apps."""
    for app in apps or os.listdir("/apps"):
        if app.startswith("."):
            continue

        # Skip special apps when enumerating from filesystem
        if apps is None and app in FAVORITE_APPS:
            continue

        if app.endswith(".py") or app.endswith(".elf"):
            yield App(app, "/apps/" + app)
            continue

        try:
            with open("/apps/" + app + "/metadata.json") as f:
                info = json.load(f)

            yield App(
                info["name"], "/apps/{}/{}".format(app, info.get("bin", "__init__.py"))
            )
        except Exception as e:
            print("'{}': metadata.json is invalid ... hoping for the best.".format(app))
            sys.print_exception(e)
            pyfile = "/apps/{}/__init__.py".format(app)
            try:
                open(pyfile).close()
                yield App(app, pyfile)
            except OSError:
                print(pyfile, "does not even exist :(")
                pass


def usb_mode(disp):
    os.usbconfig(os.USB_FLASH)

    if disp:
        disp.clear(color.CAMPGREEN)
        disp.print("USB Storage", posx=3, posy=20, fg=color.CAMPGREEN_DARK)
        disp.print("open", posx=52, posy=40, fg=color.CAMPGREEN_DARK)
        disp.update()

    time.sleep_ms(200)

    # Wait for select button to be released
    while buttons.read() == buttons.TOP_RIGHT:
        pass

    # Wait for any button to be pressed and disable USB storage again
    while buttons.read() == 0 and not os.fs_is_attached():
        pass

    os.usbconfig(os.USB_SERIAL)

    # Exit after USB-Mode to reload menu
    os.exit(0)


class SubMenu(simple_menu.Menu):
    timeout = 30.0

    def on_select(self, name, index):
        print(name)
        if name == "Make Default":
            self.disp.clear(color.CHAOSBLUE)
            self.disp.print("Making", posx=0, posy=20)
            self.disp.print(self.app.name, posx=0, posy=40)
            self.disp.print("default", posx=0, posy=60)
            self.disp.update()
            import config

            config.set_string("default_app", self.app.path)
            time.sleep(1)
        self.exit()


class MainMenu(simple_menu.Menu):
    timeout = 30.0

    def entry2name(self, app):
        return app.name

    def on_select(self, app, index):
        self.disp.clear().update()
        try:
            if app.path == "USB_STORAGE_FLAG":
                usb_mode(self.disp)
                return

            print("Trying to load " + app.path)
            os.exec(app.path)
        except OSError as e:
            print("Loading failed: ")
            sys.print_exception(e)
            self.error("Loading", "failed")
            time.sleep(1.0)
            os.exit(1)

    def on_long_select(self, app, index):
        print("Long press\n")
        if app.path not in ["main.py", "USB_STORAGE_FLAG"]:
            # sm = SubMenu(("Make Default", "Fav", "Unfav", "Delete", "Exit"))
            sm = SubMenu(("Make Default", "Exit", "", ""))
            sm.app = app
            sm.run()

    def on_timeout(self):
        try:
            f = open("main.py")
            f.close()
            os.exec("main.py")
        except OSError:
            pass


def loading_message():
    with display.open() as disp:
        disp.clear(color.CHAOSBLUE)
        disp.print("Loading", posx=31, posy=20)
        disp.print("menu ...", posx=24, posy=40)
        disp.update()


def no_apps_message():
    """Display a warning if no apps are installed."""
    with display.open() as disp:
        disp.clear(color.COMMYELLOW)
        disp.print(
            " No apps ", posx=17, posy=0, fg=color.COMMYELLOW_DARK, bg=color.COMMYELLOW
        )
        disp.print(
            "available", posx=17, posy=20, fg=color.COMMYELLOW_DARK, bg=color.COMMYELLOW
        )

        disp.print("USB Active", posx=9, posy=60, fg=color.BLACK, bg=color.COMMYELLOW)
        disp.update()
        usb_mode(None)

    while True:
        time.sleep(0.5)


if __name__ == "__main__":
    loading_message()

    try:
        apps = list(enumerate_entries())
    except OSError:
        apps = []

    if not apps:
        no_apps_message()

    MainMenu(apps).run()
