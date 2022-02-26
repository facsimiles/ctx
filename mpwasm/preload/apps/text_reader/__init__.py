"""
Text Reader Script
=================
This script will list and display text files
"""
import buttons
import color
import display
import os
import time

STATE_LIST = "List"
STATE_SHOW = "Show"
SPECIAL_NO_FILES = "# no txt files"
SPECIAL_EXIT = "[ exit ]"
SPECIAL_EMPTY = "# empty file"
BUTTON_TIMER_POPPED = -1


def list_files():
    """Create a list of available text files."""
    files = sorted(os.listdir("/"))

    # Filter for text files
    files = [txt for txt in files if txt.endswith(".txt")]

    return files


def triangle(disp, x, y, left):
    """Draw a triangle to show there's more text in this line"""
    yf = 1 if left else -1
    scale = 6
    disp.line(x - scale * yf, int(y + scale / 2), x, y, col=[255, 0, 0])
    disp.line(x, y, x, y + scale, col=[255, 0, 0])
    disp.line(x, y + scale, x - scale * yf, y + int(scale / 2), col=[255, 0, 0])


def button_events(timeout=0):
    """Iterate over button presses (event-loop)."""
    yield 0
    button_pressed = False
    count = 0
    while True:
        v = buttons.read(buttons.BOTTOM_LEFT | buttons.BOTTOM_RIGHT | buttons.TOP_RIGHT)
        if timeout > 0 and count > 0 and count % timeout == 0:
            yield BUTTON_TIMER_POPPED

        if timeout > 0:
            count += 1

        if v == 0:
            button_pressed = False

        if not button_pressed and v & buttons.BOTTOM_LEFT != 0:
            button_pressed = True
            yield buttons.BOTTOM_LEFT

        if not button_pressed and v & buttons.BOTTOM_RIGHT != 0:
            button_pressed = True
            yield buttons.BOTTOM_RIGHT

        if not button_pressed and v & buttons.TOP_RIGHT != 0:
            button_pressed = True
            yield buttons.TOP_RIGHT

        time.sleep_ms(10)


COLOR1, COLOR2 = (color.CHAOSBLUE_DARK, color.CHAOSBLUE)


def file_len(filename):
    i = -1
    with open(filename) as fh:
        for i, l in enumerate(fh):
            pass
    return i + 1


def draw_filecontent(disp, filename, pos, linecount, lineoffset=0):
    disp.clear()
    with open(filename) as fh:

        # stop if file is empty
        if linecount <= 0:
            disp.print(SPECIAL_EMPTY, posy=20, bg=color.BLACK)
            return

        # calc start position
        start = 0
        if pos > 0:
            start = pos - 1
        if start + 4 > linecount:
            start = linecount - 4
        if start < 0:
            start = 0

        # loop throuhg all lines
        for i, line in enumerate(fh):
            if i >= start + 4 or i >= linecount:
                break
            if i >= start:
                disp.rect(
                    0,
                    (i - start) * 20,
                    159,
                    (i - start) * 20 + 20,
                    col=COLOR1 if i == pos else COLOR2,
                )

                off = 0
                linelength = len(line)
                if i == pos and linelength > 11 and lineoffset > 0:
                    off = (
                        lineoffset if lineoffset + 11 < linelength else linelength - 11
                    )
                if lineoffset > linelength:
                    off = 0

                disp.print(
                    line[off : (off + 11)],
                    posy=(i - start) * 20,
                    bg=COLOR1 if i == pos else COLOR2,
                )
                if linelength > 11 and off < linelength - 11:
                    triangle(disp, 153, (i - start) * 20 + 6, False)
                if off > 0:
                    triangle(disp, 6, (i - start) * 20 + 6, True)

        disp.update()


def draw_filelist(disp, filelist, pos, filecount, lineoffset):
    disp.clear()

    start = 0
    if pos > 0:
        start = pos - 1
    if start + 4 > filecount:
        start = filecount - 4
    if start < 0:
        start = 0

    for i, line in enumerate(filelist):
        if i >= start + 4 or i >= filecount:
            break
        if i >= start:
            disp.rect(
                0,
                (i - start) * 20,
                159,
                (i - start) * 20 + 20,
                col=COLOR1 if i == pos else COLOR2,
            )

            off = 0
            linelength = len(line)
            if i == pos and linelength > 10 and lineoffset > 0:
                off = lineoffset if lineoffset + 10 < linelength else linelength - 10
            if lineoffset > linelength:
                off = 0

            disp.print(
                " " + line[off : (off + 10)],
                posy=(i - start) * 20,
                bg=COLOR1 if i == pos else COLOR2,
            )
            if i == pos:
                disp.print(">", posy=(i - start) * 20, fg=color.COMMYELLOW, bg=COLOR1)

            if linelength > 10 and off < linelength - 10:
                triangle(disp, 153, (i - start) * 20 + 6, False)
            if off > 0:
                triangle(disp, 24, (i - start) * 20 + 6, True)

    disp.update()


def main():
    disp = display.open()
    current_state = STATE_LIST

    # list files variables

    filelist = list_files()
    if len(filelist) == 0:
        filelist.append(SPECIAL_NO_FILES)
    filelist.append(SPECIAL_EXIT)
    numfiles = len(filelist)
    current_file = 0

    # show files variables

    filename = ""
    linecount = 0
    linepos = 0
    lineoffset = 0
    lineoffdir = 0
    timerscrollspeed = 1
    timerstartscroll = 5
    timercountpopped = 0

    for ev in button_events(10):

        # list files

        if current_state == STATE_LIST:
            if ev == buttons.BOTTOM_RIGHT:
                # Scroll down
                current_file = (current_file + 1) % numfiles
                lineoffset = 0
                timercountpopped = 0

            elif ev == buttons.BOTTOM_LEFT:
                # Scroll up
                current_file = (current_file + numfiles - 1) % numfiles
                lineoffset = 0
                timercountpopped = 0

            elif ev == BUTTON_TIMER_POPPED:
                timercountpopped += 1
                if (
                    timercountpopped >= timerstartscroll
                    and (timercountpopped - timerstartscroll) % timerscrollspeed == 0
                ):
                    lineoffset += 1

            elif ev == buttons.TOP_RIGHT:
                filename = filelist[current_file % numfiles]

                # exit or ignore
                if filename == SPECIAL_EXIT:
                    os.exit()
                elif filename == SPECIAL_NO_FILES:
                    continue

                # show file, switch state and draw
                current_state = STATE_SHOW
                disp.clear().update()

                # reset variables
                linepos = 0
                lineoffset = 0
                timercountpopped = 0
                linecount = file_len(filename)

                # draw
                draw_filecontent(disp, filename, linepos, linecount, lineoffset)
                continue

            draw_filelist(disp, filelist, current_file, numfiles, lineoffset)

        # show files

        elif current_state == STATE_SHOW:
            if ev == buttons.BOTTOM_RIGHT:
                if linepos < (linecount - 1):
                    # Scroll down
                    linepos += 1
                else:
                    # goto first line
                    linepos = 0
                lineoffset = 0
                timercountpopped = 0

            elif ev == buttons.BOTTOM_LEFT:
                if linepos > 0:
                    # Scroll up
                    linepos -= 1
                else:
                    # got to last line
                    linepos = linecount - 1
                lineoffset = 0
                timercountpopped = 0

            elif ev == BUTTON_TIMER_POPPED:
                timercountpopped += 1
                if (
                    timercountpopped >= timerstartscroll
                    and (timercountpopped - timerstartscroll) % timerscrollspeed == 0
                ):
                    lineoffset += 1

            elif ev == buttons.TOP_RIGHT:
                # go back to file menu
                current_state = STATE_LIST
                lineoffset = 0
                timercountpopped = 0
                draw_filelist(disp, filelist, current_file, numfiles, 0)
                continue

            draw_filecontent(disp, filename, linepos, linecount, lineoffset)


if __name__ == "__main__":
    main()
