# Stripped down version of the CPython uuid.py module
import os

int_ = int  # The built-in int type
bytes_ = bytes  # The built-in bytes type


class UUID:
    def __init__(self, hex=None, bytes=None, int=None, version=None):
        if [hex, bytes, int].count(None) != 2:
            raise TypeError("one of the hex, bytes, or int arguments must be given")

        if hex is not None:
            hex = hex.replace("urn:", "").replace("uuid:", "")
            hex = hex.strip("{}").replace("-", "")
            if len(hex) != 32:
                raise ValueError("badly formed hexadecimal UUID string")
            int = int_(hex, 16)
        if bytes is not None:
            if len(bytes) != 16:
                raise ValueError("bytes is not a 16-char string")
            assert isinstance(bytes, bytes_), repr(bytes)
            int = int_.from_bytes(bytes, "big")
        if int is not None:
            if not 0 <= int < 1 << 128:
                raise ValueError("int is out of range (need a 128-bit value)")
        if version is not None:
            if not 1 <= version <= 5:
                raise ValueError("illegal version number")
            # Set the variant to RFC 4122.
            int &= ~(0xC000 << 48)
            int |= 0x8000 << 48
            # Set the version number.
            int &= ~(0xF000 << 64)
            int |= version << 76

        setattr(self, "int", int)

    def __eq__(self, other):
        if isinstance(other, UUID):
            return self.int == other.int
        return NotImplemented

    def __lt__(self, other):
        if isinstance(other, UUID):
            return self.int < other.int
        return NotImplemented

    def __gt__(self, other):
        if isinstance(other, UUID):
            return self.int > other.int
        return NotImplemented

    def __le__(self, other):
        if isinstance(other, UUID):
            return self.int <= other.int
        return NotImplemented

    def __ge__(self, other):
        if isinstance(other, UUID):
            return self.int >= other.int
        return NotImplemented

    def __hash__(self):
        return hash(self.int)

    def __int__(self):
        return self.int

    def __repr__(self):
        return "{}({!r})".format(self.__class__.__name__, str(self))

    def __setattr__(self, name, value):
        raise TypeError("UUID objects are immutable")

    def __str__(self):
        hex = "{:032x}".format(self.int)
        return "{}-{}-{}-{}-{}".format(
            hex[:8], hex[8:12], hex[12:16], hex[16:20], hex[20:]
        )

    @property
    def bytes(self):
        return self.int.to_bytes(16, "big")

    @property
    def node(self):
        return self.int & 0xFFFFFFFFFFFF

    @property
    def hex(self):
        return "{:032x}".format(self.int)

    @property
    def urn(self):
        return "urn:uuid:" + str(self)

    @property
    def version(self):
        return int((self.int >> 76) & 0xF)


def uuid4():
    """Generate a random UUID."""
    return UUID(bytes=os.urandom(16), version=4)
