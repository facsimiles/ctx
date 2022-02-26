import os


def main():
    # Try loading analog clock
    try:
        import config

        default_app = config.get_string("default_app")
    except OSError:
        default_app = "apps/g_watch/__init__.py"

    try:
        with open(default_app, "r"):
            pass

        print("main.py: Loading " + default_app)
        os.exec(default_app)
    finally:
        True # os.exit(1)  # this is a diff from card10 upstream


if __name__ == "__main__":
    main()
