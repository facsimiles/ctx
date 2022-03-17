import sys_config


def set_string(key, value):
    """
    Write a string to the configuration file :ref:`card10_cfg`.

    Both ``key`` and ``value`` must be strings or must be
    convertible to a string using the :py:func:`str()` function.

    ``key`` must not contain spaces, control characters (including tabs),
    number signs ans equal signs.
    ``value`` must not contain control characters (including tabs).
    Neither is allowed to contain the sub-string ``"execute_elf"``.

    The key/value pair is immediately written to the configuration
    file (:ref:`card10_cfg`). After the file is written, configuration
    is read again and the new value is available via :py:func:`config.get_string`.

    :param str key:     Name of the configuration option.
    :param str value:   Value to write.
    :raises OSError: If writing to the configuration file failed.
    :raises OSError: If key or value contain illegal characters.
    :raises ValueError: If key or value contain the sub-string ``"execute_elf"``.

    .. versionadded:: 1.16
    """

    sys_config.set_string(str(key), str(value))


def get_string(key):
    """
    Read a string from the configuration file :ref:`card10_cfg`.

    ``key`` must be a string or must be convertible to a string using
    the :py:func:`str()` function.


    :param str key: Name of the configuration option.
    :rtype: str
    :returns: Value of the configuration option.
    :raises OSError: if the key is not present in the configuration.

    .. versionadded:: 1.16
    """

    return sys_config.get_string(str(key))
