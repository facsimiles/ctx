# this script reads a BMP file and displays it as POV. For optimal results use a BMP that is 11 pixels wide.

import leds

# default filename. Depending on how you want to run this, change it
FILENAME = "Smiley.bmp"
# Determines the Frame Rate, currently not used.
DELAY = 0
# Determine Brightness Level. You might not wanna use the full values from the BMP
BRIGHTNESS = 1.0


class BMPError(Exception):
    pass


class POV:
    def __init__(self, filename=FILENAME):
        self.filename = filename
        f = open("/" + self.filename, "rb")
        if f.read(2) != b"BM":  # check signature
            raise BMPError("Not BitMap file")
        # turn into int.from_bytes
        bmpFileSize = int.from_bytes(f.read(4), "little")
        f.read(4)  # Read & ignore creator bytes

        self.bmpImageOffset = self.read_le(f.read(4))  # Start of image data
        headerSize = self.read_le(f.read(4))
        self.bmpWidth = self.read_le(f.read(4))
        self.bmpHeight = self.read_le(f.read(4))
        flip = False

        print(
            "Size: %d\nImage offset: %d\nHeader size: %d"
            % (bmpFileSize, self.bmpImageOffset, headerSize)
        )
        print("Width: %d\nHeight: %d" % (self.bmpWidth, self.bmpHeight))

        if self.read_le(f.read(2)) != 1:
            raise BMPError("Not singleplane")
        bmpDepth = self.read_le(f.read(2))  # bits per pixel
        print("Bit depth: %d" % (bmpDepth))
        if bmpDepth != 24:
            raise BMPError("Not 24-bit")
        if self.read_le(f.read(2)) != 0:
            raise BMPError("Compressed file")

        print("Image OK!")
        self.rowSize = (self.bmpWidth * 3 + 3) & ~3  # 32-bit line boundary
        self.fileOffset = f.tell()
        print("File Offset" + str(self.fileOffset))
        f.close()
        self.read_image()

    def correct_brightness(self, color):
        return int(pow((color * BRIGHTNESS) / 255, 2.7) * 255 + 0.5)

    def getWidth(self):
        return self.bmpWidth

    def setFileOffset(self, fileOffset):
        self.fileOffset = fileOffset

    def getFileOffset(self):
        return self.fileOffset

    def read_le(self, bytes):
        return int.from_bytes(bytes, "little")

    def read_image(self):
        self.imageArray = []
        flip = True
        rowSize = (self.bmpWidth * 3 + 3) & ~3
        f = open("/" + self.filename, "rb")
        for row in range(self.bmpHeight):
            rowArray = []
            if flip:  # Bitmap is stored bottom-to-top order (normal BMP)
                pos = self.bmpImageOffset + (self.bmpHeight - 1 - row) * rowSize
            else:  # Bitmap is stored top-to-bottom
                pos = self.bmpImageOffset + row * rowSize
            f.seek(pos)
            for col in range(self.bmpWidth):
                rgb = [f.read(1), f.read(1), f.read(1)]
                rgb = [int.from_bytes(x, "little") for x in rgb]
                # rgb = [self.correct_brightness(x) for x in rgb]
                rowArray += [rgb]
            self.imageArray += [rowArray]

    def getImageArray(self):
        return self.imageArray

    def playMe(self):
        for i in self.imageArray:
            leds.set_all(i)


def play(imagename):
    myPov = POV(imagename)
    while True:
        myPov.playMe()


play("Smiley.bmp")
