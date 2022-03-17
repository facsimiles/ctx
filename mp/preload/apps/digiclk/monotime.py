import time as _time

_offset_ms = 0


def time_monotonic():
    return _time.time() + _offset_ms // 1000


def time_monotonic_ms():
    return _time.time_ms() + _offset_ms


def sleep(s):
    return _time.sleep(s)


def sleep_ms(ms):
    return _time.sleep_ms(ms)


def sleep_us(us):
    return _time.sleep_us(us)


def time():
    return _time.time()


def time_ms():
    return _time.time_ms()


def set_time(t):
    global _offset_ms

    cur_t = _time.time_ms()
    _time.set_time(t)
    new_t = _time.time_ms()

    diff = cur_t - new_t
    _offset_ms += diff


def set_unix_time(t):
    global _offset_ms

    cur_t = _time.time_ms()
    _time.set_unix_time(t)
    new_t = _time.time_ms()

    diff = cur_t - new_t
    _offset_ms += diff


def localtime(s=None):
    if s != None:
        return _time.localtime(s)
    else:
        return _time.localtime()


def mktime(t):
    return _time.mktime(t)


def alarm(s, cb=None):
    if cb != None:
        return _time.alarm(s, cb)
    else:
        return _time.alarm(s)
