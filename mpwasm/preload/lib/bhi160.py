import sys_bhi160
import interrupt
import ucollections

DataVector = ucollections.namedtuple("DataVector", ["x", "y", "z", "status"])


def disable_all_sensors():
    """
    Disable all sensor also if they are already deactivated.

    :returns: None
    """
    sys_bhi160.disable_all_sensors()


class BHI160:
    def enable_sensor(self):
        interrupt.disable_callback(self.interrupt_id)
        interrupt.set_callback(self.interrupt_id, self._interrupt)

        # If the sensor is already enabled, disable it first.
        # The call is allowed to silently fail.
        sys_bhi160.disable_sensor(self.sensor_id)

        self.stream_id = sys_bhi160.enable_sensor(
            self.sensor_id, self.sample_buffer_len, self.sample_rate, self.dynamic_range
        )

        if self.stream_id < 0:
            raise ValueError("Enable sensor returned %i", self.stream_id)

        self.active = True

        if self._callback:
            interrupt.enable_callback(self.interrupt_id)

    def __enter__(self):
        return self

    def __exit__(self, _et, _ev, _t):
        self.close()

    def close(self):
        """
        Close the connection to the sensor
        """
        if self.active:
            self.active = False
            ret = sys_bhi160.disable_sensor(self.sensor_id)

            if ret < 0:
                raise ValueError("Disable sensor returned %i", ret)

            interrupt.disable_callback(self.interrupt_id)
            interrupt.set_callback(self.interrupt_id, None)

    def read(self):
        """
        Read sensor values

        :returns: The recent collected sensor values as a list. If no data is
            available the list contains no elements. Maximum length of the list
            is ``sample_buffer_len``. The last element contains the most recent
            data. The elements contains a sensor specific named tuple. See the
            documentation of the sensor class for more information.

        .. warning::
            Weird behaviour ahead: If the internal buffer overflows, the new samples will be dropped.
        """
        result = []
        if self.active:
            for sample in sys_bhi160.read_sensor(self.stream_id):
                result.append(self.convert(sample))
        return result

    def _interrupt(self, _):
        if self.active:
            data = self.read()
            if self._callback:
                self._callback(data)

    def convert_data_vector(self, sample):
        return DataVector(
            self.convert_single(sample[0]),
            self.convert_single(sample[1]),
            self.convert_single(sample[2]),
            sample[3],
        )


class BHI160Accelerometer(BHI160):
    """
    Accelerometer of the BHI160.

    This sensors sample data named tuple contains the following fields:

    - ``x``: Acceleration along the x axis
    - ``y``: Acceleration along the y axis
    - ``z``: Acceleration along the z axis
    - ``status``: accuracy / "confidence" value of the sensor (0 being worst and 3 being best)

    .. todo::
        These values are not scaled correctly

    :param int sample_rate: Sample rate (optional, default is 4, range is 1 - 200 in *Hz*)
    :param int dynamic_range: Dynamic range (optional, default is 2)
    :param callback: Call this callback when enough data is collected (optional, default is None)

            .. todo::
               The callback functionality is untested, so do not be confused if it does not work.
    :param int sample_buffer_len: Length of sample buffer (optional, default is 200)
    """

    def __init__(
        self, sample_rate=4, dynamic_range=2, callback=None, sample_buffer_len=200
    ):
        self.sample_rate = sample_rate
        self.dynamic_range = dynamic_range
        self.callback = callback
        self.sample_buffer_len = sample_buffer_len
        self.sensor_id = 0
        self.interrupt_id = interrupt.BHI160_ACCELEROMETER
        self._callback = callback
        self.enable_sensor()

    def convert_single(self, value):
        return 2 * value / 32768.0

    def convert(self, sample):
        return self.convert_data_vector(sample)


class BHI160Gyroscope(BHI160):
    """
    Gyroscope of the BHI160.

    This sensors sample data named tuple contains the following fields:

    - ``x``: Rotation around the x axis
    - ``y``: Rotation around the y axis
    - ``z``: Rotation around the z axis
    - ``status``: accuracy / "confidence" value of the sensor (0 being worst and 3 being best)

    .. todo::
        These values are not scaled correctly

    :param int sample_rate: Sample rate (optional, default is 4, range is 1 - 200 in *Hz*)
    :param int dynamic_range: Dynamic range (optional, default is 2)
    :param callback: Call this callback when enough data is collected (optional, default is None)

            .. todo::
               The callback functionality is untested, so do not be confused if it does not work.
    :param int sample_buffer_len: Length of sample buffer (optional, default is 200)
    """

    def __init__(
        self, sample_rate=4, dynamic_range=2, callback=None, sample_buffer_len=200
    ):
        self.sample_rate = sample_rate
        self.dynamic_range = dynamic_range
        self.callback = callback
        self.sample_buffer_len = sample_buffer_len
        self.sensor_id = 3
        self.interrupt_id = interrupt.BHI160_GYROSCOPE
        self._callback = callback
        self.enable_sensor()

    def convert_single(self, value):
        return 360 * value / 32768.0

    def convert(self, sample):
        return self.convert_data_vector(sample)


class BHI160Orientation(BHI160):
    """
    Orientation of the BHI160. Orientation is a virtual sensor that combines
    Accelerometer, Magnetometer and Gyroscope using the IMU Algorithm to
    calculate an absolute orientation.

    This sensors sample data named tuple contains the following fields:

    - ``x``: azimuth
    - ``y``: pitch
    - ``z``: roll
    - ``status``: accuracy / "confidence" value of the sensor (0 being worst and 3 being best)

    .. todo::
        These values are not scaled correctly

    :param int sample_rate: Sample rate (optional, default is 4, range is 1 - 200 in *Hz*)
    :param int dynamic_range: This parameter is unused for the Orientation.
    :param callback: Call this callback when enough data is collected (optional, default is None)

            .. todo::
               The callback functionality is untested, so do not be confused if it does not work.
    :param int sample_buffer_len: Length of sample buffer (optional, default is 200)
    """

    def __init__(
        self, sample_rate=4, dynamic_range=2, callback=None, sample_buffer_len=200
    ):
        self.sample_rate = sample_rate
        self.dynamic_range = dynamic_range
        self.callback = callback
        self.sample_buffer_len = sample_buffer_len
        self.sensor_id = 2
        self.interrupt_id = interrupt.BHI160_ORIENTATION
        self._callback = callback
        self.enable_sensor()

    def convert_single(self, value):
        return 360 * value / 32768.0

    def convert(self, sample):
        return self.convert_data_vector(sample)


class BHI160Magnetometer(BHI160):
    """
    Magnetometer of the BHI160

    This sensors sample data named tuple contains the following fields:

    - ``x``: Magnetic field along the x axis
    - ``y``: Magnetic field along the y axis
    - ``z``: Magnetic field along the z axis
    - ``status``: accuracy / "confidence" value of the sensor (0 being worst and 3 being best)

    .. todo::
        These values are not scaled correctly

    :param int sample_rate: Sample rate (optional, default is 4, range is 1 - 200 in *Hz*)
    :param int dynamic_range: Dynamic range (optional, default is 1)
    :param callback: Call this callback when enough data is collected (optional, default is None)

            .. todo::
               The callback functionality is untested, so do not be confused if it does not work.
    :param int sample_buffer_len: Length of sample buffer (optional, default is 200)

   .. versionadded:: 1.11
    """

    def __init__(
        self, sample_rate=4, dynamic_range=1, callback=None, sample_buffer_len=200
    ):
        self.sample_rate = sample_rate
        self.dynamic_range = dynamic_range
        self.callback = callback
        self.sample_buffer_len = sample_buffer_len
        self.sensor_id = 1
        self.interrupt_id = interrupt.BHI160_MAGNETOMETER
        self._callback = callback
        self.enable_sensor()

    def convert_single(self, value):
        return 1000 * value / 32768.0

    def convert(self, sample):
        return self.convert_data_vector(sample)
