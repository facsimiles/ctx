#if EMSCRIPTEN
#include "ctx.h"
#include "emscripten.h"
Ctx *ctx_wasm_get_context(int flags);
#endif

#ifndef _EPICARDIUM_H
#define _EPICARDIUM_H

#include <stdint.h>
#include <errno.h>

#ifndef __SPHINX_DOC
/* Some headers are not recognized by hawkmoth for some odd reason */
#include <stddef.h>
#include <stdbool.h>
#else
typedef unsigned int size_t;
typedef _Bool bool;
#endif /* __SPHINX_DOC */

/*
 * These definitions are required for the code-generator.  Please don't touch!
 */
#ifndef API
#define API(id, def) def
#endif
#ifndef API_ISR
#define API_ISR(id, isr) void isr(void);
#endif

/*
 * IDs for all defined API calls.  These IDs should not be needed in application
 * code on any side.
 */

/* clang-format off */
#define API_SYSTEM_EXIT             0x1
#define API_SYSTEM_EXEC             0x2
#define API_SYSTEM_RESET            0x3
#define API_BATTERY_VOLTAGE         0x4
#define API_SLEEP                   0x5

#define API_INTERRUPT_ENABLE        0xA
#define API_INTERRUPT_DISABLE       0xB
#define API_INTERRUPT_IS_ENABLED    0xC

#define API_UART_WRITE_STR         0x10
#define API_UART_READ_CHAR         0x11
#define API_UART_READ_STR          0x12

#define API_STREAM_READ            0x1F

#define API_DISP_OPEN              0x20
#define API_DISP_CLOSE             0x21
#define API_DISP_PRINT             0x22
#define API_DISP_CLEAR             0x23
#define API_DISP_UPDATE            0x24
#define API_DISP_LINE              0x25
#define API_DISP_RECT              0x26
#define API_DISP_CIRC              0x27
#define API_DISP_PIXEL             0x28
#define API_DISP_FRAMEBUFFER       0x29
#define API_DISP_BACKLIGHT         0x2a
#define API_DISP_PRINT_ADV         0x2b
#define API_DISP_BLIT              0x2d
#define API_DISP_CTX               0x2e

/* API_BATTERY_VOLTAGE              0x30 */
#define API_BATTERY_CURRENT        0x31
#define API_CHARGEIN_VOLTAGE       0x32
#define API_CHARGEIN_CURRENT       0x33
#define API_SYSTEM_VOLTAGE         0x34
#define API_THERMISTOR_VOLTAGE     0x35

#define API_FILE_OPEN              0x40
#define API_FILE_CLOSE             0x41
#define API_FILE_READ              0x42
#define API_FILE_WRITE             0x44
#define API_FILE_FLUSH             0x45
#define API_FILE_SEEK              0x46
#define API_FILE_TELL              0x47
#define API_FILE_STAT              0x48
#define API_FILE_OPENDIR           0x49
#define API_FILE_READDIR           0x4a
#define API_FILE_UNLINK            0x4b
#define API_FILE_RENAME            0x4c
#define API_FILE_MKDIR             0x4d
#define API_FILE_FS_ATTACHED       0x4e

#define API_RTC_GET_SECONDS        0x50
#define API_RTC_SCHEDULE_ALARM     0x51
#define API_RTC_SET_MILLISECONDS   0x52
#define API_RTC_GET_MILLISECONDS   0x53
#define API_RTC_GET_MONOTONIC_SECONDS      0x54
#define API_RTC_GET_MONOTONIC_MILLISECONDS 0x55

#define API_LEDS_SET               0x60
#define API_LEDS_SET_HSV           0x61
#define API_LEDS_PREP              0x62
#define API_LEDS_PREP_HSV          0x63
#define API_LEDS_UPDATE            0x64
#define API_LEDS_SET_POWERSAVE     0x65
#define API_LEDS_SET_ROCKET        0x66
#define API_LEDS_SET_FLASHLIGHT    0x67
#define API_LEDS_DIM_TOP           0x68
#define API_LEDS_DIM_BOTTOM        0x69
#define API_LEDS_SET_ALL           0x6a
#define API_LEDS_SET_ALL_HSV       0x6b
#define API_LEDS_SET_GAMMA_TABLE   0x6c
#define API_LEDS_CLEAR_ALL         0x6d
#define API_LEDS_GET_ROCKET        0x6e
#define API_LEDS_GET               0x6f
#define API_LEDS_FLASH_ROCKET      0x72

#define API_VIBRA_SET              0x70
#define API_VIBRA_VIBRATE          0x71

#define API_LIGHT_SENSOR_RUN       0x80
#define API_LIGHT_SENSOR_GET       0x81
#define API_LIGHT_SENSOR_STOP      0x82
#define API_LIGHT_SENSOR_READ	   0x83

#define API_BUTTONS_READ           0x90

#define API_GPIO_SET_PIN_MODE      0xA0
#define API_GPIO_GET_PIN_MODE      0xA1
#define API_GPIO_WRITE_PIN         0xA2
#define API_GPIO_READ_PIN          0xA3

#define API_TRNG_READ              0xB0
#define API_CSPRNG_READ            0XB1

#define API_PERSONAL_STATE_SET     0xc0
#define API_PERSONAL_STATE_GET     0xc1
#define API_PERSONAL_STATE_IS_PERSISTENT 0xc2

#define API_BME680_INIT            0xD0
#define API_BME680_DEINIT          0xD1
#define API_BME680_GET_DATA        0xD2
#define API_BSEC_GET_DATA          0xD3

#define API_BHI160_ENABLE          0xe0
#define API_BHI160_DISABLE         0xe1
#define API_BHI160_DISABLE_ALL     0xe2

#define API_MAX30001_ENABLE        0xf0
#define API_MAX30001_DISABLE       0xf1

#define API_MAX86150_ENABLE        0x0100
#define API_MAX86150_DISABLE       0x0101

#define API_USB_SHUTDOWN           0x110
#define API_USB_STORAGE            0x111
#define API_USB_CDCACM             0x112

#define API_WS2812_WRITE           0x0120

#define API_CONFIG_GET_STRING      0x130
#define API_CONFIG_GET_INTEGER     0x131
#define API_CONFIG_GET_BOOLEAN     0x132
#define API_CONFIG_SET_STRING      0x133

#define API_BLE_GET_COMPARE_VALUE     0x140
#define API_BLE_COMPARE_RESPONSE      0x141
#define API_BLE_SET_MODE              0x142
#define API_BLE_GET_EVENT             0x143
#define API_BLE_GET_SCAN_REPORT       0x144
#define API_BLE_GET_LAST_PAIRING_NAME 0x145
#define API_BLE_GET_PEER_DEVICE_NAME  0x146
#define API_BLE_FREE_EVENT            0x147

#define API_BLE_HID_SEND_REPORT        0x150

#define API_BLE_ATTS_DYN_CREATE_GROUP         0x160
#define API_BLE_ATTS_DYN_DELETE_GROUP         0x161
#define API_BLE_ATTS_DYN_DELETE_GROUPS        0x169
#define API_BLE_ATTS_DYN_ADD_CHARACTERISTIC   0x16B
#define API_BLE_ATTS_DYN_ADD_DESCRIPTOR       0x163
#define API_BLE_ATTS_SET_BUFFER               0x16A
#define API_BLE_ATTS_SEND_SERVICE_CHANGED_IND 0x168

#define API_BLE_ATTS_SET_ATTR                 0x170
#define API_BLE_ATTS_HANDLE_VALUE_NTF         0x171
#define API_BLE_ATTS_HANDLE_VALUE_IND         0x172

#define API_BLE_CLOSE_CONNECTION              0x180
#define API_BLE_IS_CONNECTION_OPEN            0x181
#define API_BLE_SET_DEVICE_NAME               0x182
#define API_BLE_GET_DEVICE_NAME               0x183
#define API_BLE_GET_ADDRESS                   0x184
#define API_BLE_ADVERTISE                     0x185
#define API_BLE_ADVERTISE_STOP                0x186
#define API_BLE_DISCOVER_PRIMARY_SERVICES     0x187
#define API_BLE_DISCOVER_CHARACTERISTICS      0x188
#define API_BLE_DISCOVER_DESCRIPTORS          0x189
#define API_BLE_ATTC_READ                     0x18A
#define API_BLE_ATTC_WRITE_NO_RSP             0x18B
#define API_BLE_ATTC_WRITE                    0x18C

#define API_BLE_INIT                          0x190
#define API_BLE_DEINIT                        0x191

/* clang-format on */

typedef uint32_t api_int_id_t;

/**
 * Interrupts
 * ==========
 * Next to API calls, Epicardium API also has an interrupt mechanism to serve
 * the other direction.  These interrupts can be enabled/disabled
 * (masked/unmasked) using :c:func:`epic_interrupt_enable` and
 * :c:func:`epic_interrupt_disable`.
 *
 * These interrupts work similar to hardware interrupts:  You will only get a
 * single interrupt, even if multiple events occured since the ISR last ran
 * (*this behavior is new since version 1.16*).
 *
 * .. warning::
 *
 *    Never attempt to call the API from inside an ISR.  This might trigger an
 *    assertion if a call is already being made from thread context.  We plan to
 *    lift this restriction at some point, but for the time being, this is how
 *    it is.
 */

/**
 * Enable/unmask an API interrupt.
 *
 * :param int_id: The interrupt to be enabled
 */
API(API_INTERRUPT_ENABLE, int epic_interrupt_enable(api_int_id_t int_id));

/**
 * Disable/mask an API interrupt.
 *
 * :param int_id: The interrupt to be disabled
 */
API(API_INTERRUPT_DISABLE, int epic_interrupt_disable(api_int_id_t int_id));

/**
 * Check if an API interrupt is enabled.
 *
 * :param int int_id: The interrupt to be checked
 * :param bool* enabled: ``true`` will be stored here if the interrupt is enabled.
 *   ``false`` otherwise.
 *
 * :return: 0 on success, ``-EINVAL`` if the interrupt is unknown.
 *
 * .. versionadded:: 1.16
 */
API(API_INTERRUPT_IS_ENABLED, int epic_interrupt_is_enabled(api_int_id_t int_id, bool *enabled));

/**
 * The following interrupts are defined:
 */

/* clang-format off */
/** Reset Handler */
#define EPIC_INT_RESET                  0
/** ``^C`` interrupt. See :c:func:`epic_isr_ctrl_c` for details.  */
#define EPIC_INT_CTRL_C                 1
/** UART Receive interrupt.  See :c:func:`epic_isr_uart_rx`. */
#define EPIC_INT_UART_RX                2
/** RTC Alarm interrupt.  See :c:func:`epic_isr_rtc_alarm`. */
#define EPIC_INT_RTC_ALARM              3
/** BHI160 Accelerometer.  See :c:func:`epic_isr_bhi160_accelerometer`. */
#define EPIC_INT_BHI160_ACCELEROMETER	4
/** BHI160 Orientation Sensor.  See :c:func:`epic_isr_bhi160_orientation`. */
#define EPIC_INT_BHI160_ORIENTATION	5
/** BHI160 Gyroscope.  See :c:func:`epic_isr_bhi160_gyroscope`. */
#define EPIC_INT_BHI160_GYROSCOPE	6
/** MAX30001 ECG.  See :c:func:`epic_isr_max30001_ecg`. */
#define EPIC_INT_MAX30001_ECG		7
/** BHI160 Magnetometer.  See :c:func:`epic_isr_bhi160_magnetometer`. */
#define EPIC_INT_BHI160_MAGNETOMETER    8
/** MAX86150 ECG and PPG sensor.  See :c:func:`epic_isr_max86150`. */
#define EPIC_INT_MAX86150               9
/** Bluetooth Low Energy event.  See :c:func:`epic_isr_ble`. */
#define EPIC_INT_BLE                    10
#define EPIC_INT_MAX86150_PROX          11
/* Number of defined interrupts. */
#define EPIC_INT_NUM                    12
/* clang-format on */

/*
 * "Reset Handler*.  This isr is implemented by the API caller and is used to
 * reset the core for loading a new payload.
 *
 * Just listed here for completeness.  You don't need to implement this yourself.
 */
API_ISR(EPIC_INT_RESET, __epic_isr_reset);

/**
 * Core API
 * ========
 * The following functions control execution of code on core 1.
 */

/**
 * Stop execution of the current payload and return to the menu.
 *
 * :param int ret:  Return code.
 * :return: :c:func:`epic_exit` will never return.
 */
void epic_exit(int ret) __attribute__((noreturn));

/*
 * The actual epic_exit() function is not an API call because it needs special
 * behavior.  The underlying call is __epic_exit() which returns.  After calling
 * this API function, epic_exit() will enter the reset handler.
 */
API(API_SYSTEM_EXIT, void __epic_exit(int ret));

/**
 * Stop execution of the current payload and immediately start another payload.
 *
 * :param char* name: Name (path) of the new payload to start.  This can either
 *    be:
 *
 *    - A path to an ``.elf`` file (l0dable).
 *    - A path to a ``.py`` file (will be loaded using Pycardium).
 *    - A path to a directory (assumed to be a Python module, execution starts
 *      with ``__init__.py`` in this folder).
 *
 * :return: :c:func:`epic_exec` will only return in case loading went wrong.
 *    The following error codes can be returned:
 *
 *    - ``-ENOENT``: File not found.
 *    - ``-ENOEXEC``: File not a loadable format.
 */
int epic_exec(char *name);

/*
 * Underlying API call for epic_exec().  The function is not an API call itself
 * because it needs special behavior when loading a new payload.
 */
API(API_SYSTEM_EXEC, int __epic_exec(char *name));

/**
 * Reset/Restart card10
 */
API(API_SYSTEM_RESET, void epic_system_reset(void));

/**
 * Sleep for the specified amount of time.
 *
 * This call will block for at most the specified amount of time. It allows epicardium to
 * reduce clock speed of the system until this call is finished.
 *
 * This call returns early if an interrupt is signaled from epicardium.
 *
 * The clock source of epicardium has a limited amount of accuracy. Tolerances
 * of +- 10% have been observed. This means that the sleep time also has a
 * tolarance of at least +- 10%. The exact amount varies from device to device and
 * also with temperature. You should take this into consideration when selecting
 * the time you want to sleep.
 *
 * :param ms:  Time to wait in milliseconds
 * :returns: 0 if no interrupt happened, ``INT_MAX`` if an interrupt happened and the sleep ended early.
 */
API(API_SLEEP, int epic_sleep(uint32_t ms));

/**
 * PMIC API
 * ===============
 */


/**
 * Read the current battery voltage.
 */
API(API_BATTERY_VOLTAGE, int epic_read_battery_voltage(float *result));

/**
 * Read the current battery current.
 */
API(API_BATTERY_CURRENT, int epic_read_battery_current(float *result));

/**
 * Read the current charge voltage.
 */
API(API_CHARGEIN_VOLTAGE, int epic_read_chargein_voltage(float *result));

/**
 * Read the current charge current.
 */
API(API_CHARGEIN_CURRENT, int epic_read_chargein_current(float *result));

/**
 * Read the current system voltage.
 */
API(API_SYSTEM_VOLTAGE, int epic_read_system_voltage(float *result));

/**
 * Read the current thermistor voltage.
 */
API(API_THERMISTOR_VOLTAGE, int epic_read_thermistor_voltage(float *result));


/**
 * UART/Serial Interface
 * =====================
 */

/**
 * Write a string to all connected serial devices.  This includes:
 *
 * - Real UART, whose pins are mapped onto USB-C pins.  Accessible via the HW-debugger.
 * - A CDC-ACM device available via USB.
 * - Maybe, in the future, bluetooth serial?
 *
 * :param str:  String to write.  Does not necessarily have to be NULL-terminated.
 * :param length:  Amount of bytes to print.
 */
API(API_UART_WRITE_STR, void epic_uart_write_str(
	const char *str, size_t length
));

/**
 * Try reading a single character from any connected serial device.
 *
 * If nothing is available, :c:func:`epic_uart_read_char` returns ``(-1)``.
 *
 * :return:  The byte or ``(-1)`` if no byte was available.
 */
API(API_UART_READ_CHAR, int epic_uart_read_char(void));

/**
 * Read as many characters as possible from the UART queue.
 *
 * :c:func:`epic_uart_read_str` will not block if no new data is available.  For
 * an example, see :c:func:`epic_isr_uart_rx`.
 *
 * :param char* buf: Buffer to be filled with incoming data.
 * :param size_t cnt: Size of ``buf``.
 * :returns: Number of bytes read.  Can be ``0`` if no data was available.
 *    Might be a negative value if an error occured.
 */
API(API_UART_READ_STR, int epic_uart_read_str(char *buf, size_t cnt));

/**
 * **Interrupt Service Routine** for :c:data:`EPIC_INT_UART_RX`
 *
 * UART receive interrupt.  This interrupt is triggered whenever a new character
 * becomes available on any connected UART device.  This function is weakly
 * aliased to :c:func:`epic_isr_default` by default.
 *
 * **Example**:
 *
 * .. code-block:: cpp
 *
 *    void epic_isr_uart_rx(void)
 *    {
 *            char buffer[33];
 *            int n = epic_uart_read_str(&buffer, sizeof(buffer) - 1);
 *            buffer[n] = '\0';
 *            printf("Got: %s\n", buffer);
 *    }
 *
 *    int main(void)
 *    {
 *            epic_interrupt_enable(EPIC_INT_UART_RX);
 *
 *            while (1) {
 *                    __WFI();
 *            }
 *    }
 */
API_ISR(EPIC_INT_UART_RX, epic_isr_uart_rx);

/**
 * **Interrupt Service Routine** for :c:data:`EPIC_INT_CTRL_C`
 *
 * A user-defineable ISR which is triggered when a ``^C`` (``0x04``) is received
 * on any serial input device.  This function is weakly aliased to
 * :c:func:`epic_isr_default` by default.
 *
 * To enable this interrupt, you need to enable :c:data:`EPIC_INT_CTRL_C`:
 *
 * .. code-block:: cpp
 *
 *    epic_interrupt_enable(EPIC_INT_CTRL_C);
 */
API_ISR(EPIC_INT_CTRL_C, epic_isr_ctrl_c);

/**
 * Buttons
 * =======
 *
 */

/** Button IDs */
enum epic_button {
	/** ``1``, Bottom left button (bit 0). */
	BUTTON_LEFT_BOTTOM   = 1,
	/** ``2``, Bottom right button (bit 1). */
	BUTTON_RIGHT_BOTTOM  = 2,
	/** ``4``, Top right button (bit 2). */
	BUTTON_RIGHT_TOP     = 4,
	/** ``8``, Top left (power) button (bit 3). */
	BUTTON_LEFT_TOP      = 8,
	/** ``8``, Top left (power) button (bit 3). */
	BUTTON_RESET         = 8,
};

/**
 * Read buttons.
 *
 * :c:func:`epic_buttons_read` will read all buttons specified in ``mask`` and
 * return set bits for each button which was reported as pressed.
 *
 * .. note::
 *
 *    The reset button cannot be unmapped from reset functionality.  So, while
 *    you can read it, it cannot be used for app control.
 *
 * **Example**:
 *
 * .. code-block:: cpp
 *
 *    #include "epicardium.h"
 *
 *    uint8_t pressed = epic_buttons_read(BUTTON_LEFT_BOTTOM | BUTTON_RIGHT_BOTTOM);
 *
 *    if (pressed & BUTTON_LEFT_BOTTOM) {
 *            // Bottom left button is pressed
 *    }
 *
 *    if (pressed & BUTTON_RIGHT_BOTTOM) {
 *            // Bottom right button is pressed
 *    }
 *
 * :param uint8_t mask: Mask of buttons to read.  The 4 LSBs correspond to the 4
 *     buttons:
 *
 *     ===== ========= ============ ===========
 *     ``3`` ``2``     ``1``        ``0``
 *     ----- --------- ------------ -----------
 *     Reset Right Top Right Bottom Left Bottom
 *     ===== ========= ============ ===========
 *
 *     Use the values defined in :c:type:`epic_button` for masking, as shown in
 *     the example above.
 * :return: Returns nonzero value if unmasked buttons are pushed.
 */
API(API_BUTTONS_READ, uint8_t epic_buttons_read(uint8_t mask));

/**
 * Wristband GPIO
 * ==============
 */

/** GPIO pins IDs */
enum gpio_pin {
    /** ``1``, Wristband connector 1 */
    EPIC_GPIO_WRISTBAND_1 = 1,
    /** ``2``, Wristband connector 2 */
    EPIC_GPIO_WRISTBAND_2 = 2,
    /** ``3``, Wristband connector 3 */
    EPIC_GPIO_WRISTBAND_3 = 3,
    /** ``4``, Wristband connector 4 */
    EPIC_GPIO_WRISTBAND_4 = 4,
};

/** GPIO pin modes */
enum gpio_mode {
    /** Configure the pin as input */
    EPIC_GPIO_MODE_IN = (1<<0),
    /** Configure the pin as output */
    EPIC_GPIO_MODE_OUT = (1<<1),
    EPIC_GPIO_MODE_ADC = (1<<2),

    /** Enable the internal pull-up resistor */
    EPIC_GPIO_PULL_UP = (1<<6),
    /** Enable the internal pull-down resistor */
    EPIC_GPIO_PULL_DOWN = (1<<7),
};

/**
 * Set the mode of a card10 GPIO pin.
 *
 * :c:func:`epic_gpio_set_pin_mode` will set the pin specified by ``pin`` to the mode ``mode``.
 * If the specified pin ID is not valid this function will do nothing.
 *
 * **Example:**
 *
 * .. code-block:: cpp
 *
 *    #include "epicardium.h"
 *
 *    // Configure wristband pin 1 as output.
 *    if (epic_gpio_set_pin_mode(GPIO_WRISTBAND_1, GPIO_MODE_OUT)) {
 *        // Do your error handling here...
 *    }
 *
 * :param uint8_t pin: ID of the pin to configure. Use on of the IDs defined in :c:type:`gpio_pin`.
 * :param uint8_t mode: Mode to be configured. Use a combination of the :c:type:`gpio_mode` flags.
 * :returns: ``0`` if the mode was set, ``-EINVAL`` if ``pin`` is not valid or the mode could not be set.
 */
API(API_GPIO_SET_PIN_MODE, int epic_gpio_set_pin_mode(
	uint8_t pin, uint8_t mode
));

/**
 * Get the mode of a card10 GPIO pin.
 *
 * :c:func:`epic_gpio_get_pin_mode` will get the current mode of the GPIO pin specified by ``pin``.
 *
 * **Example:**
 *
 * .. code-block:: cpp
 *
 *    #include "epicardium.h"
 *
 *    // Get the mode of wristband pin 1.
 *    int mode = epic_gpio_get_pin_mode(GPIO_WRISTBAND_1);
 *    if (mode < 0) {
 *        // Do your error handling here...
 *    } else {
 *        // Do something with the queried mode information
 *    }
 *
 * :param uint8_t pin: ID of the pin to get the configuration of. Use on of the IDs defined in :c:type:`gpio_pin`.
 * :returns: Configuration byte for the specified pin or ``-EINVAL`` if the pin is not valid.
 */
API(API_GPIO_GET_PIN_MODE, int epic_gpio_get_pin_mode(uint8_t pin));

/**
 * Write value to a card10 GPIO pin,
 *
 * :c:func:`epic_gpio_write_pin` will set the value of the GPIO pin described by ``pin`` to either on or off depending on ``on``.
 *
 * **Example:**
 *
 * .. code-block:: cpp
 *
 *    #include "epicardium.h"
 *
 *    // Get the mode of wristband pin 1.
 *    int mode = epic_gpio_get_pin_mode(GPIO_WRISTBAND_1);
 *    if (mode < 0) {
 *        // Do your error handling here...
 *    } else {
 *        // Do something with the queried mode information
 *    }
 *
 * :param uint8_t pin: ID of the pin to get the configuration of. Use on of the IDs defined in :c:type:`gpio_pin`.
 * :param bool on: Sets the pin to either true (on/high) or false (off/low).
 * :returns: ``0`` on succcess, ``-EINVAL`` if ``pin`` is not valid or is not configured as an output.
 */
API(API_GPIO_WRITE_PIN, int epic_gpio_write_pin(uint8_t pin, bool on));

/**
 * Read value of a card10 GPIO pin.
 *
 * :c:func:`epic_gpio_read_pin` will get the value of the GPIO pin described by ``pin``.
 *
 * **Example:**
 *
 * .. code-block:: cpp
 *
 *    #include "epicardium.h"
 *
 *    // Get the current value of wristband pin 1.
 *    uint32_t value = epic_gpio_read_pin(GPIO_WRISTBAND_1);
 *    if (mode == -EINVAL) {
 *        // Do your error handling here...
 *    } else {
 *        // Do something with the current value
 *    }
 *
 * :param uint8_t pin: ID of the pin to get the configuration of. Use on of the IDs defined in :c:type:`gpio_pin`.
 * :returns: ``-EINVAL`` if ``pin`` is not valid, an integer value otherwise.
 */
API(API_GPIO_READ_PIN, int epic_gpio_read_pin(uint8_t pin));

/**
 * LEDs
 * ====
 */

/**
 * Set one of card10's RGB LEDs to a certain color in RGB format.
 *
 * This function is rather slow when setting multiple LEDs, use
 * :c:func:`leds_set_all` or :c:func:`leds_prep` + :c:func:`leds_update`
 * instead.
 *
 * :param int led:  Which LED to set.  0-10 are the LEDs on the top and 11-14
 *    are the 4 "ambient" LEDs.
 * :param uint8_t r:  Red component of the color.
 * :param uint8_t g:  Green component of the color.
 * :param uint8_t b:  Blue component of the color.
 */
API(API_LEDS_SET, void epic_leds_set(int led, uint8_t r, uint8_t g, uint8_t b));


/**
 * Get one of card10's RGB LEDs in format of RGB.
 *
 * :c:func:`epic_leds_get_rgb` will get the value of a RGB  LED described by ``led``.
 *
 * :param int led:  Which LED to get.  0-10 are the LEDs on the top and 11-14
 *    are the 4 "ambient" LEDs.
 * :param uint8_t * rgb:  need tree byte array to get the value of red, green and blue.
 * :returns: ``0`` on success or ``-EPERM`` if the LED is blocked by personal-state.
 *
 * .. versionadded:: 1.10
 */
API(API_LEDS_GET, int epic_leds_get_rgb(int led, uint8_t * rgb));

/**
 * Set one of the rockets to flash for a certain time.
 *
 * :c:func:`epic_leds_flash_rocket` will set a timer for the flash of a rocket.
 *
 * :param int led: Number of the rocket that sould flash 
 * :param uint8_t value:  brightness of the 'on'-state of this rocket ( 0 < value < 32)
 * :param int millis:  time in milliseconds defining the duration of the flash (i.e. how long is the rocket 'on')
 *
 * .. versionadded:: 1.16
 */
API(API_LEDS_FLASH_ROCKET, void epic_leds_flash_rocket(int led, uint8_t valiue, int millis));

/**
 * Set one of card10's RGB LEDs to a certain color in HSV format.
 *
 * This function is rather slow when setting multiple LEDs, use
 * :c:func:`leds_set_all_hsv` or :c:func:`leds_prep_hsv` + :c:func:`leds_update`
 * instead.
 *
 * :param int led:  Which LED to set.  0-10 are the LEDs on the top and 11-14 are the 4 "ambient" LEDs.
 * :param float h:  Hue component of the color. (0 <= h < 360)
 * :param float s:  Saturation component of the color. (0 <= s <= 1)
 * :param float v:  Value/Brightness component of the color. (0 <= v <= 0)
 */
API(API_LEDS_SET_HSV, void epic_leds_set_hsv(
	int led, float h, float s, float v
));

/**
 * Set multiple of card10's RGB LEDs to a certain color in RGB format.
 *
 * The first ``len`` leds are set, the remaining ones are not modified.
 *
 * :param uint8_t[len][r,g,b] pattern:  Array with RGB Values with 0 <= len <=
 *    15. 0-10 are the LEDs on the top and 11-14 are the 4 "ambient" LEDs.
 * :param uint8_t len: Length of 1st dimension of ``pattern``, see above.
 */
API(API_LEDS_SET_ALL, void epic_leds_set_all(uint8_t *pattern, uint8_t len));

/**
 * Set multiple of card10's RGB LEDs to a certain color in HSV format.
 *
 * The first ``len`` led are set, the remaining ones are not modified.
 *
 * :param uint8_t[len][h,s,v] pattern:  Array of format with HSV Values with 0
 *    <= len <= 15.  0-10 are the LEDs on the top and 11-14 are the 4 "ambient"
 *    LEDs. (0 <= h < 360, 0 <= s <= 1, 0 <= v <= 1)
 * :param uint8_t len: Length of 1st dimension of ``pattern``, see above.
 */
API(API_LEDS_SET_ALL_HSV, void epic_leds_set_all_hsv(
	float *pattern, uint8_t len
));

/**
 * Prepare one of card10's RGB LEDs to be set to a certain color in RGB format.
 *
 * Use :c:func:`leds_update` to apply changes.
 *
 * :param int led:  Which LED to set.  0-10 are the LEDs on the top and 11-14
 *    are the 4 "ambient" LEDs.
 * :param uint8_t r:  Red component of the color.
 * :param uint8_t g:  Green component of the color.
 * :param uint8_t b:  Blue component of the color.
 */
API(API_LEDS_PREP, void epic_leds_prep(
	int led, uint8_t r, uint8_t g, uint8_t b
));

/**
 * Prepare one of card10's RGB LEDs to be set to a certain color in HSV format.
 *
 * Use :c:func:`leds_update` to apply changes.
 *
 * :param int led:  Which LED to set.  0-10 are the LEDs on the top and 11-14
 *    are the 4 "ambient" LEDs.
 * :param uint8_t h:  Hue component of the color. (float, 0 <= h < 360)
 * :param uint8_t s:  Saturation component of the color. (float, 0 <= s <= 1)
 * :param uint8_t v:  Value/Brightness component of the color. (float, 0 <= v <= 0)
 */
API(API_LEDS_PREP_HSV, void epic_leds_prep_hsv(
	int led, float h, float s, float v
));

/**
 * Set global brightness for top RGB LEDs.
 *
 * Aside from PWM, the RGB LEDs' overall brightness can be controlled with a
 * current limiter independently to achieve a higher resolution at low
 * brightness which can be set with this function.
 *
 * :param uint8_t value:  Global brightness of top LEDs. (1 <= value <= 8, default = 1)
 */
API(API_LEDS_DIM_BOTTOM, void epic_leds_dim_bottom(uint8_t value));

/**
 * Set global brightness for bottom RGB LEDs.
 *
 * Aside from PWM, the RGB LEDs' overall brightness can be controlled with a
 * current limiter independently to achieve a higher resolution at low
 * brightness which can be set with this function.
 *
 * :param uint8_t value:  Global brightness of bottom LEDs. (1 <= value <= 8, default = 8)
 */
API(API_LEDS_DIM_TOP, void epic_leds_dim_top(uint8_t value));

/**
 * Enables or disables powersave mode.
 *
 * Even when set to zero, the RGB LEDs still individually consume ~1mA.
 * Powersave intelligently switches the supply power in groups. This introduces
 * delays in the magnitude of ~10µs, so it can be disabled for high speed
 * applications such as POV.
 *
 * :param bool eco:  Activates powersave if true, disables it when false. (default = True)
 */
API(API_LEDS_SET_POWERSAVE, void epic_leds_set_powersave(bool eco));

/**
 * Updates the RGB LEDs with changes that have been set with :c:func:`leds_prep`
 * or :c:func:`leds_prep_hsv`.
 *
 * The LEDs can be only updated in bulk, so using this approach instead of
 * :c:func:`leds_set` or :c:func:`leds_set_hsv` significantly reduces the load
 * on the corresponding hardware bus.
 */
API(API_LEDS_UPDATE, void epic_leds_update(void));

/**
 * Set the brightness of one of the rocket LEDs.
 *
 * :param int led:  Which LED to set.
 *
 *    +-------+--------+----------+
 *    |   ID  | Color  | Location |
 *    +=======+========+==========+
 *    | ``0`` | Blue   | Left     |
 *    +-------+--------+----------+
 *    | ``1`` | Yellow | Top      |
 *    +-------+--------+----------+
 *    | ``2`` | Green  | Right    |
 *    +-------+--------+----------+
 * :param uint8_t value:  Brightness of LED (value between 0 and 31).
 */
API(API_LEDS_SET_ROCKET, void epic_leds_set_rocket(int led, uint8_t value));

/**
 * Get the brightness of one of the rocket LEDs.
 *
 * :param int led:  Which LED to get.
 *
 *    +-------+--------+----------+
 *    |   ID  | Color  | Location |
 *    +=======+========+==========+
 *    | ``0`` | Blue   | Left     |
 *    +-------+--------+----------+
 *    | ``1`` | Yellow | Top      |
 *    +-------+--------+----------+
 *    | ``2`` | Green  | Right    |
 *    +-------+--------+----------+
 * :returns value:  Brightness of LED (value between 0 and 31)  or ``-EINVAL`` if the LED/rocket does not exists.
 *
 * .. versionadded:: 1.10
 */
API(API_LEDS_GET_ROCKET, int epic_leds_get_rocket(int led));

/**
 * Turn on the bright side LED which can serve as a flashlight if worn on the left wrist or as a rad tattoo illuminator if worn on the right wrist.
 *
 *:param bool power:  Side LED on if true.
 */
API(API_LEDS_SET_FLASHLIGHT, void epic_set_flashlight(bool power));

/**
 * Set gamma lookup table for individual rgb channels.
 *
 * Since the RGB LEDs' subcolor LEDs have different peak brightness and the
 * linear scaling introduced by PWM is not desireable for color accurate work,
 * custom lookup tables for each individual color channel can be loaded into the
 * Epicardium's memory with this function.
 *
 * :param uint8_t rgb_channel:  Color whose gamma table is to be updated, 0->Red, 1->Green, 2->Blue.
 * :param uint8_t[256] gamma_table: Gamma lookup table. (default = 4th order power function rounded up)
 */
API(API_LEDS_SET_GAMMA_TABLE, void epic_leds_set_gamma_table(
	uint8_t rgb_channel, uint8_t *gamma_table
));

/**
 * Set all LEDs to a certain RGB color.
 *
 * :param uint8_t r: Value for the red color channel.
 * :param uint8_t g: Value for the green color channel.
 * :param uint8_t b: Value for the blue color channel.
 */
API(API_LEDS_CLEAR_ALL, void epic_leds_clear_all(
	uint8_t r, uint8_t g, uint8_t b
));

/**
 * BME680
 * ======
 *
 * .. versionadded:: 1.4
 */

/**
 * BME680 Sensor Data
 */
struct bme680_sensor_data {
	/** Temperature in degree celsius */
	float temperature;
	/** Humidity in % relative humidity */
	float humidity;
	/** Pressure in hPa */
	float pressure;
	/** Gas resistance in Ohms */
	float gas_resistance;
};

/**
 * Initialize the BM680 sensor.
 *
 * .. versionadded:: 1.4
 *
 * :return: 0 on success or ``-Exxx`` on error.  The following
 *     errors might occur:
 *
 *     - ``-EFAULT``:  On NULL-pointer.
 *     - ``-EINVAL``:  Invalid configuration.
 *     - ``-EIO``:  Communication with the device failed.
 *     - ``-ENODEV``:  Device was not found.
 */
API(API_BME680_INIT, int epic_bme680_init());

/**
 * De-Initialize the BM680 sensor.
 *
 * .. versionadded:: 1.4
 *
 * :return: 0 on success or ``-Exxx`` on error.  The following
 *     errors might occur:
 *
 *     - ``-EFAULT``:  On NULL-pointer.
 *     - ``-EINVAL``:  Invalid configuration.
 *     - ``-EIO``:  Communication with the device failed.
 *     - ``-ENODEV``:  Device was not found.
 */
API(API_BME680_DEINIT, int epic_bme680_deinit());

/**
 * Get the current BME680 data.
 *
 * .. versionadded:: 1.4
 *
 * :param data: Where to store the environmental data.
 * :return: 0 on success or ``-Exxx`` on error.  The following
 *     errors might occur:
 *
 *     - ``-EFAULT``:  On NULL-pointer.
 *     - ``-EINVAL``:  Sensor not initialized.
 *     - ``-EIO``:  Communication with the device failed.
 *     - ``-ENODEV``:  Device was not found.
 */
API(API_BME680_GET_DATA, int epic_bme680_read_sensors(
	struct bme680_sensor_data *data
));

/**
 * .. _bsec_api:
 *
 * BSEC
 * ----
 * The Bosch Sensortec Environmental Cluster (BSEC) library
 * allows to estimate an indoor air qualtiy (IAQ) metric as
 * well as CO2 and VOC content equivalents using the gas sensor
 * of the BME680.
 *
 * As it is a proprietary binary blob, it has to be enabled using
 * the ``bsec_enable`` configuration option (see :ref:`card10_cfg`).
 *
 * Please also have a look at the BME680 datasheet and some of
 * the BSEC documentation:
 *
 * https://git.card10.badge.events.ccc.de/card10/hardware/-/blob/master/datasheets/bosch/BST-BME680-DS001.pdf
 *
 * https://git.card10.badge.events.ccc.de/card10/firmware/-/blob/master/lib/vendor/Bosch/BSEC/integration_guide/BST-BME680-Integration-Guide-AN008-48.pdf
 */

/**
 * BSEC Sensor Data
 */
struct bsec_sensor_data {
	/** Compensated temperature in degree celsius */
	float temperature;
	/** Compensated humidity in % relative humidity */
	float humidity;
	/** Pressure in hPa */
	float pressure;
	/** Gas resistance in Ohms */
	float gas_resistance;
	/** Timestamp in of the measurement in UNIX time (seconds since
	 * 1970-01-01 00:00:00 UTC)*/
	uint32_t timestamp;
	/** Accuracy of IAQ, CO2 equivalent and breath VOC equivalent:
	 *
	 * 0: Stabilization / run-in ongoing:
	 *    This means that the sensor still needs to warm up. Takes about
	 *    5 min after activation of BSEC / reboot.
	 *
	 * 1: Low accuracy:
	 *    The sensor has not yet been calibrated. BSEC needs to collect
	 *    more data to calibrate the sensor. This can take multiple
	 *    hours.
	 *
	 *    BSEC documentation: To reach high accuracy(3) please expose
	 *    sensor once to good air (e.g. outdoor air) and bad air (e.g.
	 *    box with exhaled breath) for auto-trimming
	 *
	 * 2: Medium accuracy: auto-trimming ongoing
	 *    BSEC has detected that it needs to recalibrate the sensor.
	 *    This is an automatic process and usally finishes after tens
	 *    of minutes. Can happen every now and then.
	 *
	 * 3: High accuracy:
	 *    The sensor has warmed up and is calibrated.
	 *
	 * From BSEC documentation:
	 * IAQ accuracy indicator will notify the user when they should
	 * initiate a calibration process. Calibration is performed automatically
	 * in the background if the sensor is exposed to clean and polluted air
	 * for approximately 30 minutes each.
	 *
	 * See also:
	 * https://community.bosch-sensortec.com/t5/MEMS-sensors-forum/BME680-IAQ-accuracy-definition/m-p/5931/highlight/true#M10
	 */
	uint8_t accuracy;
	/** Indoor Air Quality with range 0 to 500
	 *
	 * Statement from the Bosch BSEC library:
	 *
	 * Indoor-air-quality (IAQ) gives an indication of the relative change
	 * in ambient TVOCs detected by BME680.
	 *
	 * The IAQ scale ranges from 0 (clean air) to 500 (heavily polluted air).
	 * During operation, algorithms automatically calibrate and adapt
	 * themselves to the typical environments where the sensor is operated
	 * (e.g., home, workplace, inside a car, etc.).This automatic background
	 * calibration ensures that users experience consistent IAQ performance.
	 * The calibration process considers the recent measurement history (typ.
	 * up to four days) to ensure that IAQ=25 corresponds to typical good air
	 * and IAQ=250 indicates typical polluted air.
	 *
	 * Please also consult the BME680 datsheet (pages 9 and 21) as well:
	 * https://git.card10.badge.events.ccc.de/card10/hardware/-/blob/master/datasheets/bosch/BST-BME680-DS001.pdf
	 *
	 */
	int32_t indoor_air_quality;
	/** Unscaled IAQ value.
	 *
	 * See this post for details:
	 * https://community.bosch-sensortec.com/t5/MEMS-sensors-forum/BME680-strange-IAQ-and-CO2-values/m-p/9667/highlight/true#M1505
	 */
	int32_t static_indoor_air_quality;
	/** Estimation of equivalant CO2 content in the air in ppm. */
	float co2_equivalent;
	/** Estimation of equivalant breath VOC content in the air in ppm. */
	float breath_voc_equivalent;
};

/**
 *
 * Get the current BME680 data filtered by Bosch BSEC library
 *
 * As it is a proprietary binary blob, it has to be enabled using
 * the ``bsec_enable`` configuration option (see :ref:`card10_cfg`).
 *
 * The sample rate is currently fixed to one sample every 3 seconds.
 * Querying the sensor more often will return cached data.
 *
 * After the libary has been activated it starts to calibrate the
 * sensor. This can take multiple hours.
 * After a reset/power on it takes about 5 minutes to stabilize
 * the sensor if it was calibrated before.
 *
 * The BSEC library regularly recalibrates the sensor during operation.
 * The ``accuracy`` field of the return data indicates the calibration
 * status of the sensor. Please take it into consideration when
 * using / displaying the IAQ.
 *
 * Please refer to the description of :c:type:`bsec_sensor_data` for more
 * information about how to interpret its content.
 *
 * .. versionadded:: 1.x
 *
 * :param data: Where to store the environmental data.
 * :return: 0 on success or ``-Exxx`` on error.  The following
 *     errors might occur:
 *
 *     - ``-EFAULT``:  On NULL-pointer.
 *     - ``-EINVAL``:  No data available from the sensor.
 *     - ``-ENODEV``:  BSEC libray is not running.
 */
API(API_BSEC_GET_DATA, int epic_bsec_read_sensors(
	struct bsec_sensor_data *data
));
/**
 * MAX86150
 * ========
 */

/**
 * Configuration for a MAX86150 sensor.
 *
 * This struct is used when enabling a sensor using
 * :c:func:`epic_max86150_enable_sensor`.
 */
struct max86150_sensor_config {
    /**
     * Number of samples Epicardium should keep for this sensor.  Do not set
     * this number too high as the sample buffer will eat RAM.
     */
    size_t sample_buffer_len;
    /**
     * Sample rate for PPG from the sensor in Hz.  Maximum data rate is limited
     * to 200 Hz for all sensors though some might be limited at a lower
     * rate.
     *
     * Possible values are 10, 20, 50, 84, 100, 200.
     */
    uint16_t ppg_sample_rate;
};

/**
 * MAX86150 Sensor Data
 */
struct max86150_sensor_data {
	/** Red LED data */
	uint32_t red;
	/** IR LED data */
	uint32_t ir;
	/** ECG data */
	int32_t ecg;
};

/**
 * Enable a MAX86150 PPG and ECG sensor.
 *
 * Calling this function will instruct the MAX86150 to collect a
 * data from the sensor.  You can then retrieve the samples using
 * :c:func:`epic_stream_read`.
 *
 * :param max86150_sensor_config* config: Configuration for this sensor.
 * :param size_t config_size: Size of ``config``.
 * :returns: A sensor descriptor which can be used with
 *    :c:func:`epic_stream_read` or a negative error value:
 *
 *    - ``-ENOMEM``:  The MAX86150 driver failed to create a stream queue.
 *    - ``-ENODEV``:  The MAX86150 driver failed due to physical connectivity problem
 *      (broken wire, unpowered, etc).
 *    - ``-EINVAL``:  config->ppg_sample_rate is not one of 10, 20, 50, 84, 100, 200
 *      or config_size is not size of config.
 *
 * .. versionadded:: 1.16
 */
API(API_MAX86150_ENABLE, int epic_max86150_enable_sensor(struct max86150_sensor_config *config, size_t config_size));

/**
 * Disable the MAX86150 sensor.
 *
 * :returns: 0 in case of success or forward negative error value from stream_deregister.
 *
 * .. versionadded:: 1.16
 */
API(API_MAX86150_DISABLE, int epic_max86150_disable_sensor());

/**
 * **Interrupt Service Routine** for :c:data:`EPIC_INT_MAX86150`
 *
 * :c:func:`epic_isr_max86150` is called whenever the MAX86150
 * PPG sensor has new data available.
 */
API_ISR(EPIC_INT_MAX86150, epic_isr_max86150);

/**
 * Personal State
 * ==============
 * Card10 can display your personal state.
 *
 * If a personal state is set the top-left LED on the bottom side of the
 * harmonics board is directly controlled by epicardium and it can't be
 * controlled by pycardium.
 *
 * To re-enable pycardium control the personal state has to be cleared. To do
 * that simply set it to ``STATE_NONE``.
 *
 * The personal state can be set to be persistent which means it won't get reset
 * on pycardium application change/restart.
 */

/** Possible personal states. */
enum personal_state {
    /** ``0``, No personal state - LED is under regular application control. */
    STATE_NONE = 0,
    /** ``1``, "no contact, please!" - I am overloaded. Please leave me be - red led, continuously on. */
    STATE_NO_CONTACT = 1,
    /** ``2``, "chaos" - Adventure time - blue led, short blink, long blink. */
    STATE_CHAOS = 2,
    /** ``3``, "communication" - want to learn something or have a nice conversation - yellow led, long blinks. */
    STATE_COMMUNICATION = 3,
    /** ``4``, "camp" - I am focussed on self-, camp-, or community maintenance - green led, fade on and off. */
    STATE_CAMP = 4,
    /** STATE_MAX gives latest value and count of possible STATEs**/
    STATE_MAX = 5,
};

/**
 * Set the users personal state.
 *
 * Using :c:func:`epic_personal_state_set` an application can set the users personal state.
 *
 * :param uint8_t state: The users personal state. Must be one of :c:type:`personal_state`.
 * :param bool persistent: Indicates whether the configured personal state will remain set and active on pycardium application restart/change.
 * :returns: ``0`` on success, ``-EINVAL`` if an invalid state was requested.
 */
API(API_PERSONAL_STATE_SET, int epic_personal_state_set(
	uint8_t state, bool persistent
));

/**
 * Get the users personal state.
 *
 * Using :c:func:`epic_personal_state_get` an application can get the currently set personal state of the user.
 *
 * :returns: A value with exactly one value of :c:type:`personal_state` set.
 */
API(API_PERSONAL_STATE_GET, int epic_personal_state_get());

/**
 * Get whether the users personal state is persistent.
 *
 * Using :c:func:`epic_personal_state_is_persistent` an app can find out whether the users personal state is persistent or transient.
 *
 * :returns: ``1`` if the state is persistent, ``0`` otherwise.
 */
API(API_PERSONAL_STATE_IS_PERSISTENT, int epic_personal_state_is_persistent());

/**
 * Sensor Data Streams
 * ===================
 * A few of card10's sensors can do continuous measurements.  To allow
 * performant access to their data, the following function is made for generic
 * access to streams.
 */

/**
 * Read sensor data into a buffer.  ``epic_stream_read()`` will read as many
 * sensor samples into the provided buffer as possible and return the number of
 * samples written.  If no samples are available, ``epic_stream_read()`` will
 * return ``0`` immediately.
 *
 * ``epic_stream_read()`` expects the provided buffer to have a size which is a
 * multiple of the sample size for the given stream.  For the sample-format and
 * size, please consult the sensors documentation.
 *
 * Before reading the internal sensor sample queue, ``epic_stream_read()`` will
 * call a sensor specific *poll* function to allow the sensor driver to fetch
 * new samples from its hardware.  This should, however, never take a long
 * amount of time.
 *
 * :param int sd: Sensor Descriptor.  You get sensor descriptors as return
 *    values when activating the respective sensors.
 * :param void* buf: Buffer where sensor data should be read into.
 * :param size_t count: How many bytes to read at max.  Note that fewer bytes
 *    might be read.  In most cases, this should be ``sizeof(buf)``.
 * :return: Number of data packets read (**not** number of bytes) or a negative
 *    error value.  Possible errors:
 *
 *    - ``-ENODEV``: Sensor is not currently available.
 *    - ``-EBADF``: The given sensor descriptor is unknown.
 *    - ``-EINVAL``:  ``count`` is not a multiple of the sensor's sample size.
 *    - ``-EBUSY``: The descriptor table lock could not be acquired.
 *
 * **Example**:
 *
 * .. code-block:: cpp
 *
 *    #include "epicardium.h"
 *
 *    struct foo_measurement sensor_data[16];
 *    int foo_sd, n;
 *
 *    foo_sd = epic_foo_sensor_enable(9001);
 *
 *    while (1) {
 *            n = epic_stream_read(
 *                    foo_sd,
 *                    &sensor_data,
 *                    sizeof(sensor_data)
 *            );
 *
 *            // Print out the measured sensor samples
 *            for (int i = 0; i < n; i++) {
 *                    printf("Measured: %?\n", sensor_data[i]);
 *            }
 *    }
 */
API(API_STREAM_READ, int epic_stream_read(int sd, void *buf, size_t count));

/**
 * BHI160 Sensor Fusion
 * ====================
 * card10 has a BHI160 onboard which is used as an IMU.  BHI160 exposes a few
 * different sensors which can be accessed using Epicardium API.
 *
 * .. versionadded:: 1.4
 *
 * **Example**:
 *
 * .. code-block:: cpp
 *
 *    #include "epicardium.h"
 *
 *    // Configure a sensor & enable it
 *    struct bhi160_sensor_config cfg = {0};
 *    cfg.sample_buffer_len = 40;
 *    cfg.sample_rate = 4;   // Hz
 *    cfg.dynamic_range = 2; // g
 *
 *    int sd = epic_bhi160_enable_sensor(BHI160_ACCELEROMETER, &cfg);
 *
 *    // Read sensor data
 *    while (1) {
 *            struct bhi160_data_vector buf[10];
 *
 *            int n = epic_stream_read(sd, buf, sizeof(buf));
 *
 *            for (int i = 0; i < n; i++) {
 *                    printf("X: %6d Y: %6d Z: %6d\n",
 *                           buf[i].x,
 *                           buf[i].y,
 *                           buf[i].z);
 *            }
 *    }
 *
 *    // Disable the sensor
 *    epic_bhi160_disable_sensor(BHI160_ACCELEROMETER);
 */

/**
 * BHI160 Sensor Types
 * -------------------
 */

/**
 * BHI160 virtual sensor type.
 */
enum bhi160_sensor_type {
	/**
	 * Accelerometer
	 *
	 * - Data type: :c:type:`bhi160_data_vector`
	 * - Dynamic range: g's (1x Earth Gravity, ~9.81m*s^-2)
	 */
	BHI160_ACCELEROMETER               = 0,
	/**
	 * Magnetometer
	 *
	 * - Data type: :c:type:`bhi160_data_vector`
	 * - Dynamic range: -1000 to 1000 microtesla
	 */
	BHI160_MAGNETOMETER                = 1,
	/** Orientation */
	BHI160_ORIENTATION                 = 2,
	/** Gyroscope */
	BHI160_GYROSCOPE                   = 3,
	/** Gravity (**Unimplemented**) */
	BHI160_GRAVITY                     = 4,
	/** Linear acceleration (**Unimplemented**) */
	BHI160_LINEAR_ACCELERATION         = 5,
	/** Rotation vector (**Unimplemented**) */
	BHI160_ROTATION_VECTOR             = 6,
	/** Uncalibrated magnetometer (**Unimplemented**) */
	BHI160_UNCALIBRATED_MAGNETOMETER   = 7,
	/** Game rotation vector (whatever that is supposed to be) */
	BHI160_GAME_ROTATION_VECTOR        = 8,
	/** Uncalibrated gyroscrope (**Unimplemented**) */
	BHI160_UNCALIBRATED_GYROSCOPE      = 9,
	/** Geomagnetic rotation vector (**Unimplemented**) */
	BHI160_GEOMAGNETIC_ROTATION_VECTOR = 10,
};

enum bhi160_data_type {
	BHI160_DATA_TYPE_VECTOR
};

/**
 * BHI160 Sensor Data Types
 * ------------------------
 */

/**
 * Vector Data.  The scaling of these values is dependent on the chosen dynamic
 * range.  See the individual sensor's documentation for details.
 */
struct bhi160_data_vector {
	enum bhi160_data_type data_type;

	/** X */
	int16_t x;
	/** Y */
	int16_t y;
	/** Z */
	int16_t z;
	/** Status */
	uint8_t status;
};

/**
 * BHI160 API
 * ----------
 */

/**
 * Configuration for a BHI160 sensor.
 *
 * This struct is used when enabling a sensor using
 * :c:func:`epic_bhi160_enable_sensor`.
 */
struct bhi160_sensor_config {
	/**
	 * Number of samples Epicardium should keep for this sensor.  Do not set
	 * this number too high as the sample buffer will eat RAM.
	 */
	size_t sample_buffer_len;
	/**
	 * Sample rate for the sensor in Hz.  Maximum data rate is limited
	 * to 200 Hz for all sensors though some might be limited at a lower
	 * rate.
	 */
	uint16_t sample_rate;
	/**
	 * Dynamic range.  Interpretation of this value depends on
	 * the sensor type.  Please refer to the specific sensor in
	 * :c:type:`bhi160_sensor_type` for details.
	 */
	uint16_t dynamic_range;
	/** Always zero. Reserved for future parameters. */
	uint8_t _padding[8];
};

/**
 * Enable a BHI160 virtual sensor.  Calling this function will instruct the
 * BHI160 to collect data for this specific virtual sensor.  You can then
 * retrieve the samples using :c:func:`epic_stream_read`.
 *
 * :param bhi160_sensor_type sensor_type: Which sensor to enable.
 * :param bhi160_sensor_config* config: Configuration for this sensor.
 * :returns: A sensor descriptor which can be used with
 *    :c:func:`epic_stream_read` or a negative error value:
 *
 *    - ``-EBUSY``:  The BHI160 driver is currently busy with other tasks and
 *      could not be acquired for enabling a sensor.
 *
 * .. versionadded:: 1.4
 */
API(API_BHI160_ENABLE, int epic_bhi160_enable_sensor(
	enum bhi160_sensor_type sensor_type,
	struct bhi160_sensor_config *config
));

/**
 * Disable a BHI160 sensor.
 *
 * :param bhi160_sensor_type sensor_type: Which sensor to disable.
 *
 * .. versionadded:: 1.4
 */
API(API_BHI160_DISABLE, int epic_bhi160_disable_sensor(
	enum bhi160_sensor_type sensor_type
));

/**
 * Disable all BHI160 sensors.
 *
 * .. versionadded:: 1.4
 */
API(API_BHI160_DISABLE_ALL, void epic_bhi160_disable_all_sensors());

/**
 * BHI160 Interrupt Handlers
 * -------------------------
 */

/**
 * **Interrupt Service Routine** for :c:data:`EPIC_INT_BHI160_ACCELEROMETER`
 *
 * :c:func:`epic_isr_bhi160_accelerometer` is called whenever the BHI160
 * accelerometer has new data available.
 */
API_ISR(EPIC_INT_BHI160_ACCELEROMETER, epic_isr_bhi160_accelerometer);

/**
 * **Interrupt Service Routine** for :c:data:`EPIC_INT_BHI160_MAGNETOMETER`
 *
 * :c:func:`epic_isr_bhi160_magnetometer` is called whenever the BHI160
 * magnetometer has new data available.
 */
API_ISR(EPIC_INT_BHI160_MAGNETOMETER, epic_isr_bhi160_magnetometer);

/**
 * **Interrupt Service Routine** for :c:data:`EPIC_INT_BHI160_ORIENTATION`
 *
 * :c:func:`epic_isr_bhi160_orientation` is called whenever the BHI160
 * orientation sensor has new data available.
 */
API_ISR(EPIC_INT_BHI160_ORIENTATION, epic_isr_bhi160_orientation);

/**
 * **Interrupt Service Routine** for :c:data:`EPIC_INT_BHI160_GYROSCOPE`
 *
 * :c:func:`epic_isr_bhi160_orientation` is called whenever the BHI160
 * gyroscrope has new data available.
 */
API_ISR(EPIC_INT_BHI160_GYROSCOPE, epic_isr_bhi160_gyroscope);


/**
 * Vibration Motor
 * ===============
 */

/**
 * Turn vibration motor on or off
 *
 * :param status: 1 to turn on, 0 to turn off.
 */
API(API_VIBRA_SET, void epic_vibra_set(int status));

/**
 * Turn vibration motor on for a given time
 *
 * :param millis: number of milliseconds to run the vibration motor.
 */
API(API_VIBRA_VIBRATE, void epic_vibra_vibrate(int millis));

/**
 * Display
 * =======
 * The card10 has an LCD screen that can be accessed from user code.
 *
 * There are two ways to access the display:
 *
 *  - *immediate mode*, where you ask Epicardium to draw shapes and text for
 *    you.  Most functions in this subsection are related to *immediate mode*.
 *  - *framebuffer mode*, where you provide Epicardium with a memory range where
 *    you already drew graphics whichever way you wanted and Epicardium will
 *    copy them to the display.  To use *framebuffer mode*, use the
 *    :c:func:`epic_disp_framebuffer` function.
 */

/** Line-Style */
enum disp_linestyle {
  /** */
  LINESTYLE_FULL = 0,
  /** */
  LINESTYLE_DOTTED = 1
};

/** Fill-Style */
enum disp_fillstyle {
  /** */
  FILLSTYLE_EMPTY = 0,
  /** */
  FILLSTYLE_FILLED = 1
};

/** Width of display in pixels */
#define DISP_WIDTH 160

/** Height of display in pixels */
#define DISP_HEIGHT 80

/**
 * Framebuffer
 *
 * The frambuffer stores pixels as RGB565, but byte swapped.  That is, for every ``(x, y)`` coordinate, there are two ``uint8_t``\ s storing 16 bits of pixel data.
 *
 * .. todo::
 *
 *    Document (x, y) in relation to chirality.
 *
 * **Example**: Fill framebuffer with red
 *
 * .. code-block:: cpp
 *
 * 	union disp_framebuffer fb;
 * 	uint16_t red = 0b1111100000000000;
 * 	for (int y = 0; y < DISP_HEIGHT; y++) {
 * 		for (int x = 0; x < DISP_WIDTH; x++) {
 * 			fb.fb[y][x][0] = red >> 8;
 * 			fb.fb[y][x][1] = red & 0xFF;
 * 		}
 * 	}
 * 	epic_disp_framebuffer(&fb);
 */
union disp_framebuffer {
  /** Coordinate based access (as shown in the example above). */
  uint8_t fb[DISP_HEIGHT][DISP_WIDTH][2];
  /** Raw byte-indexed access. */
  uint8_t raw[DISP_HEIGHT*DISP_WIDTH*2];
};

/**
 * Locks the display.
 *
 * :return: ``0`` on success or a negative value in case of an error:
 *
 *    - ``-EBUSY``: Display was already locked from another task.
 */
API(API_DISP_OPEN, int epic_disp_open());

/**
 * Unlocks the display again.
 *
 * :return: ``0`` on success or a negative value in case of an error:
 *
 *    - ``-EBUSY``: Display was already locked from another task.
 */
API(API_DISP_CLOSE, int epic_disp_close());

/**
 * Causes the changes that have been written to the framebuffer
 * to be shown on the display
 * :return: ``0`` on success or a negative value in case of an error:
 *
 *    - ``-EBUSY``: Display was already locked from another task.
 */
API(API_DISP_UPDATE, int epic_disp_update());

/**
 * Prints a string into the display framebuffer
 *
 * :param posx: x position to print to.
 * :param posy: y position to print to.
 * :param pString: string to print
 * :param fg: foreground color in rgb565
 * :param bg: background color in rgb565
 * :return: ``0`` on success or a negative value in case of an error:
 *
 *    - ``-EBUSY``: Display was already locked from another task.
 */
API(API_DISP_PRINT,
    int epic_disp_print(
	    int16_t posx,
	    int16_t posy,
	    const char *pString,
	    uint16_t fg,
	    uint16_t bg)
    );

/*
 * Font Selection
 */
enum disp_font_name {
	DISP_FONT8  = 0,
	DISP_FONT12 = 1,
	DISP_FONT16 = 2,
	DISP_FONT20 = 3,
	DISP_FONT24 = 4,
};

/*
 * Image data type
 */
enum epic_rgb_format {
	EPIC_RGB8     = 0,
	EPIC_RGBA8    = 1,
	EPIC_RGB565   = 2,
	EPIC_RGBA5551 = 3,
 };

/**
 * Prints a string into the display framebuffer with font type selectable
 *
 * :param fontName: number of font, use FontName enum
 * :param posx: x position to print to.
 * :param posy: y position to print to.
 * :param pString: string to print
 * :param fg: foreground color in rgb565
 * :param bg: background color in rgb565, no background is drawn if bg==fg
 * :return: ``0`` on success or a negative value in case of an error:
 *
 *    - ``-EBUSY``: Display was already locked from another task.
 */
API(API_DISP_PRINT_ADV, int epic_disp_print_adv(
	uint8_t font,
	int16_t posx,
	int16_t posy,
	const char *pString,
	uint16_t fg,
	uint16_t bg
));

/**
 * Fills the whole screen with one color
 *
 * :param color: fill color in rgb565
 * :return: ``0`` on success or a negative value in case of an error:
 *
 *    - ``-EBUSY``: Display was already locked from another task.
 */
API(API_DISP_CLEAR, int epic_disp_clear(uint16_t color));

/**
 * Draws a pixel on the display
 *
 * :param x: x position;
 * :param y: y position;
 * :param color: pixel color in rgb565
 * :return: ``0`` on success or a negative value in case of an error:
 *
 *    - ``-EBUSY``: Display was already locked from another task.
 */
API(API_DISP_PIXEL, int epic_disp_pixel(
	int16_t x, int16_t y, uint16_t color
));

/**
 * Blits an image buffer to the display
 *
 * :param x: x position
 * :param y: y position
 * :param w: Image width
 * :param h: Image height
 * :param img: Image data
 * :param format: Format of the image data. One of :c:type:`epic_rgb_format`.
 */
API(API_DISP_BLIT, int epic_disp_blit(
	int16_t x,
	int16_t y,
	int16_t w,
	int16_t h,
	void *img,
	enum epic_rgb_format format
));

/**
 * Draws a line on the display
 *
 * :param xstart: x starting position
 * :param ystart: y starting position
 * :param xend: x ending position
 * :param yend: y ending position
 * :param color: line color in rgb565
 * :param linestyle: 0 for solid, 1 for dottet (almost no visual difference)
 * :param pixelsize: thickness of the line; 1 <= pixelsize <= 8
 * :return: ``0`` on success or a negative value in case of an error:
 *
 *    - ``-EBUSY``: Display was already locked from another task.
 */
API(API_DISP_LINE, int epic_disp_line(
	int16_t xstart,
	int16_t ystart,
	int16_t xend,
	int16_t yend,
	uint16_t color,
	enum disp_linestyle linestyle,
	uint16_t pixelsize
));

/**
 * Draws a rectangle on the display
 *
 * :param xstart: x coordinate of top left corner
 * :param ystart: y coordinate of top left corner
 * :param xend: x coordinate of bottom right corner
 * :param yend: y coordinate of bottom right corner
 * :param color: line color in rgb565
 * :param fillstyle: 0 for empty, 1 for filled
 * :param pixelsize: thickness of the rectangle outline; 1 <= pixelsize <= 8
 * :return: ``0`` on success or a negative value in case of an error:
 *
 *    - ``-EBUSY``: Display was already locked from another task.
 */
API(API_DISP_RECT, int epic_disp_rect(
	int16_t xstart,
	int16_t ystart,
	int16_t xend,
	int16_t yend,
	uint16_t color,
	enum disp_fillstyle fillstyle,
	uint16_t pixelsize
));

/**
 * Draws a circle on the display
 *
 * :param x: x coordinate of the center; 0 <= x <= 160
 * :param y: y coordinate of the center; 0 <= y <= 80
 * :param rad: radius of the circle
 * :param color: fill and outline color of the circle (rgb565)
 * :param fillstyle: 0 for empty, 1 for filled
 * :param pixelsize: thickness of the circle outline; 1 <= pixelsize <= 8
 * :return: ``0`` on success or a negative value in case of an error:
 *
 *    - ``-EBUSY``: Display was already locked from another task.
 */
API(API_DISP_CIRC, int epic_disp_circ(
	int16_t x,
	int16_t y,
	uint16_t rad,
	uint16_t color,
	enum disp_fillstyle fillstyle,
	uint16_t pixelsize
));

/**
 * Draw out the entries from a prefilled ctx object.
 *
 * .. todo::
 *
 *    Document this more...
 */
API(API_DISP_CTX, int epic_disp_ctx(Ctx *ctx));

/**
 * Immediately send the contents of a framebuffer to the display. This overrides
 * anything drawn by immediate mode graphics and displayed using ``epic_disp_update``.
 *
 * :param fb: framebuffer to display
 * :return: ``0`` on success or negative value in case of an error:
 *
 *    - ``-EBUSY``: Display was already locked from another task.
 */
API(API_DISP_FRAMEBUFFER, int epic_disp_framebuffer(
	union disp_framebuffer *fb
));


/**
 * Light Sensor
 * ============
 */


/**
 * Set the backlight brightness.
 *
 * Note that this function does not require acquiring the display.
 *
 * :param brightness: brightness from 0 - 100
 * :return: ``0`` on success or negative value in case of an error
 */
API(API_DISP_BACKLIGHT, int epic_disp_backlight(uint16_t brightness));


/**
 * Start continuous readout of the light sensor. Will read light level
 * at preconfigured interval and make it available via :c:func:`epic_light_sensor_get`.
 *
 * If the continuous readout was already running, this function will silently pass.
 *
 *
 * :return: ``0`` if the start was successful or a negative error value
 *      if an error occured. Possible errors:
 *
 *      - ``-EBUSY``: The timer could not be scheduled.
 */
API(API_LIGHT_SENSOR_RUN, int epic_light_sensor_run());

/**
 * Get the last light level measured by the continuous readout.
 *
 * :param uint16_t* value: where the last light level should be written.
 * :return: ``0`` if the readout was successful or a negative error
 *      value. Possible errors:
 *
 *      - ``-ENODATA``: Continuous readout not currently running.
 */
API(API_LIGHT_SENSOR_GET, int epic_light_sensor_get(uint16_t* value));


/**
 * Stop continuous readout of the light sensor.
 *
 * If the continuous readout wasn't running, this function will silently pass.
 *
 * :return: ``0`` if the stop was sucessful or a negative error value
 *      if an error occured. Possible errors:
 *
 *      - ``-EBUSY``: The timer stop could not be scheduled.
 */
API(API_LIGHT_SENSOR_STOP, int epic_light_sensor_stop());

/**
 * Get the light level directly.
 *
 * Each call has an intrinsic delay of about 240us, I recommend another
 * 100-300us delay  between calls. Whether or not the IR LED is fast enough is
 * another issue.
 *
 * :return: Light level
 *
 * .. versionadded:: 1.8
 */
API(API_LIGHT_SENSOR_READ, uint16_t epic_light_sensor_read(void));


/**
 * File
 * ====
 * Except for :c:func:`epic_file_open`, which models C stdio's ``fopen``
 * function, ``close``, ``read`` and ``write`` model `close(2)`_, `read(2)`_ and
 * `write(2)`_.  All file-related functions return >= ``0`` on success and
 * ``-Exyz`` on failure, with error codes from errno.h (``EIO``, ``EINVAL``
 * etc.)
 *
 * .. _close(2): http://man7.org/linux/man-pages/man2/close.2.html
 * .. _read(2): http://man7.org/linux/man-pages/man2/read.2.html
 * .. _write(2): http://man7.org/linux/man-pages/man2/write.2.html
 */

/** */
API(API_FILE_OPEN, int epic_file_open(
	const char* filename, const char* modeString
));

/** */
API(API_FILE_CLOSE, int epic_file_close(int fd));

/** */
API(API_FILE_READ, int epic_file_read(int fd, void* buf, size_t nbytes));

/**
 * Write bytes to a file.
 *
 * :param int fd: Descriptor returned by :c:func:`epic_file_open`.
 * :param void* buf: Data to write.
 * :param size_t nbytes: Number of bytes to write.
 *
 * :return: ``< 0`` on error, ``nbytes`` on success. (Partial writes don't occur on success!)
 *
*/
API(API_FILE_WRITE, int epic_file_write(
	int fd, const void* buf, size_t nbytes
));

/** */
API(API_FILE_FLUSH, int epic_file_flush(int fd));

/** */
API(API_FILE_SEEK, int epic_file_seek(int fd, long offset, int whence));

/** */
API(API_FILE_TELL, int epic_file_tell(int fd));

/** */
enum epic_stat_type {
	/**
	 * Basically ``ENOENT``. Although :c:func:`epic_file_stat` returns an
	 * error for 'none', the type will still be set to none additionally.
	 *
	 * This is also used internally to track open FS objects, where we use
	 * ``EPICSTAT_NONE`` to mark free objects.
	 */
	EPICSTAT_NONE,
	/** normal file */
	EPICSTAT_FILE,
	/** directory */
	EPICSTAT_DIR,
};

/**
 * Maximum length of a path string (=255).
 */
#define EPICSTAT_MAX_PATH        255
/* conveniently the same as FF_MAX_LFN */

/** */
struct epic_stat {
	/** Entity Type: file, directory or none */
	enum epic_stat_type type;

	/*
	 * Note about padding & placement of uint32_t size:
	 *
	 *   To accomodate for future expansion, we want padding at the end of
	 *   this struct. Since sizeof(enum epic_stat_type) can not be assumed
	 *   to be have a certain size, we're placing uint32_t size here so we
	 *   can be sure it will be at offset 4, and therefore the layout of the
	 *   other fields is predictable.
	 */

	/** Size in bytes. */
	uint32_t size;

	/** File Name. */
	char name[EPICSTAT_MAX_PATH + 1];
	uint8_t _reserved[12];
};

/**
 * stat path
 *
 * :param char* filename: path to stat
 * :param epic_stat* stat: pointer to result
 *
 * :return: ``0`` on success, negative on error
 */
API(API_FILE_STAT, int epic_file_stat(
	const char* path, struct epic_stat* stat
));

/**
 * Open a directory, for enumerating its contents.
 *
 * Use :c:func:`epic_file_readdir` to iterate over the directories entries.
 *
 * **Example**:
 *
 * .. code-block:: cpp
 *
 *    #include "epicardium.h"
 *
 *    int fd = epic_file_opendir("/path/to/dir");
 *
 *    struct epic_stat entry;
 *    for (;;) {
 *            epic_file_readdir(fd, &entry);
 *
 *            if (entry.type == EPICSTAT_NONE) {
 *                    // End
 *                    break;
 *            }
 *
 *            printf("%s\n", entry.name);
 *    }
 *
 *    epic_file_close(fd);
 *
 * :param char* path: Directory to open.
 *
 * :return: ``> 0`` on success, negative on error
 */
API(API_FILE_OPENDIR, int epic_file_opendir(const char* path));

/**
 * Read one entry from a directory.
 *
 * Call :c:func:`epic_file_readdir` multiple times to iterate over all entries
 * of a directory.  The end of the entry list is marked by returning
 * :c:data:`EPICSTAT_NONE` as the :c:member:`epic_stat.type`.
 *
 * :param int fd: Descriptor returned by :c:func:`epic_file_opendir`.
 * :param epic_stat* stat: Pointer where to store the result.  Pass NULL to
 *    reset iteration offset of ``fd`` back to the beginning.
 *
 * :return: ``0`` on success, negative on error
 */
API(API_FILE_READDIR, int epic_file_readdir(int fd, struct epic_stat* stat));

/**
 * Unlink (remove) a file.
 *
 * :param char* path: file to delete
 *
 * :return: ``0`` on success, negative on error
 */
API(API_FILE_UNLINK, int epic_file_unlink(const char* path));

/**
 * Rename a file or directory.
 *
 * :param char* oldp: old name
 * :param char* newp: new name
 *
 * :return: ``0`` on success, negative on error
 */
API(API_FILE_RENAME, int epic_file_rename(const char *oldp, const char* newp));

/**
 * Create directory in CWD
 *
 * :param char* dirname: directory name
 *
 * :return: ``0`` on success, negative on error
 */
API(API_FILE_MKDIR, int epic_file_mkdir(const char *dirname));

/**
 * Check whether the filesystem is currently attached and a call like
 * :c:func:`epic_file_open` has a chance to succeed.
 *
 * :return: ``true`` if the filesystem is attached and ``false`` if the
 *     filesystem is not attached.
 */
API(API_FILE_FS_ATTACHED, bool epic_fs_is_attached(void));

/**
 * RTC
 * ===
 */

/**
 * Get the monotonic time in seconds.
 *
 * :return: monotonic time in seconds
 *
 * .. versionadded:: 1.11
 */
API(API_RTC_GET_MONOTONIC_SECONDS,
	uint32_t epic_rtc_get_monotonic_seconds(void)
);

/**
 * Get the monotonic time in ms.
 *
 * :return: monotonic time in milliseconds
 *
 * .. versionadded:: 1.11
 */
API(API_RTC_GET_MONOTONIC_MILLISECONDS,
	uint64_t epic_rtc_get_monotonic_milliseconds(void)
);

/**
 * Read the current RTC value.
 *
 * :return: Unix time in seconds
 */
API(API_RTC_GET_SECONDS, uint32_t epic_rtc_get_seconds(void));

/**
 * Read the current RTC value in ms.
 *
 * :return: Unix time in milliseconds
 */
API(API_RTC_GET_MILLISECONDS, uint64_t epic_rtc_get_milliseconds(void));

/**
 * Sets the current RTC time in milliseconds
 */
API(API_RTC_SET_MILLISECONDS, void epic_rtc_set_milliseconds(
	uint64_t milliseconds
));

/**
 * Schedule the RTC alarm for the given timestamp.
 *
 * :param uint32_t timestamp: When to schedule the IRQ
 * :return: ``0`` on success or a negative value if an error occured. Possible
 *    errors:
 *
 *    - ``-EINVAL``: RTC is in a bad state
 */
API(API_RTC_SCHEDULE_ALARM, int epic_rtc_schedule_alarm(uint32_t timestamp));

/**
 * **Interrupt Service Routine** for :c:data:`EPIC_INT_RTC_ALARM`
 *
 * ``epic_isr_rtc_alarm()`` is called when the RTC alarm triggers.  The RTC alarm
 * can be scheduled using :c:func:`epic_rtc_schedule_alarm`.
 */
API_ISR(EPIC_INT_RTC_ALARM, epic_isr_rtc_alarm);

/**
 * RNG
 * ====
 */

/**
 * Read random bytes from the TRNG.
 *
 * Be aware that this function returns raw unprocessed bytes from
 * the TRNG. They might be biased or have other kinds of imperfections.
 *
 * Use :c:func:`epic_csprng_read` for cryptographically safe random
 * numbers instead.
 *
 * .. warning::
 *
 *    The exact behaviour of the TRNG is not well understood. Its
 *    distribution and other parameters are unknown. Only use this
 *    function if you really want the unmodified values from the
 *    hardware TRNG to experiment with it.
 *
 * :param uint8_t * dest: Destination buffer
 * :param size: Number of bytes to read.
 * :return: ``0`` on success or a negative value if an error occured. Possible
 *    errors:
 *
 *    - ``-EFAULT``: Invalid destination address.
 */
API(API_TRNG_READ, int epic_trng_read(uint8_t *dest, size_t size));

/**
 * Read random bytes from the CSPRNG.
 *
 * The random bytes returned are safe to be used for cryptography.
 *
 * :param uint8_t * dest: Destination buffer
 * :param size: Number of bytes to read.
 * :return: ``0`` on success or a negative value if an error occured. Possible
 *    errors:
 *
 *    - ``-EFAULT``: Invalid destination address.
 */
API(API_CSPRNG_READ, int epic_csprng_read(uint8_t *dest, size_t size));

/**
 * MAX30001
 * ========
 */

/**
 * Configuration for a MAX30001 sensor.
 *
 * This struct is used when enabling the sensor using
 * :c:func:`epic_max30001_enable_sensor`.
 */
struct max30001_sensor_config {
	/**
	 * Number of samples Epicardium should keep for this sensor.  Do not set
	 * this number too high as the sample buffer will eat RAM.
	 */
	size_t sample_buffer_len;
	/**
	 * Sample rate for the sensor in Hz.
	 */
	uint16_t sample_rate;

	/**
	 * Set to true if the second lead comes from USB-C
	 */
	bool usb;

	/**
	 * Set to true if the interal lead bias of the MAX30001 is to be used.
	 */
	bool bias;

	/** Always zero. Reserved for future parameters. */
	uint8_t _padding[8];
};

/**
 * Enable a MAX30001 ECG sensor.
 *
 * Calling this function will instruct the MAX30001 to collect data for this
 * sensor.  You can then retrieve the samples using :c:func:`epic_stream_read`.
 *
 * :param max30001_sensor_config* config: Configuration for this sensor.
 * :returns: A sensor descriptor which can be used with
 *    :c:func:`epic_stream_read` or a negative error value:
 *
 *    - ``-EBUSY``:  The MAX30001 driver is currently busy with other tasks and
 *      could not be acquired for enabling a sensor.
 *
 * .. versionadded:: 1.6
 */
API(API_MAX30001_ENABLE, int epic_max30001_enable_sensor(
	struct max30001_sensor_config *config
));

/**
 * Disable MAX30001
 *
 * .. versionadded:: 1.6
 */
API(API_MAX30001_DISABLE, int epic_max30001_disable_sensor());

/**
 * **Interrupt Service Routine** for :c:data:`EPIC_INT_MAX30001_ECG`
 *
 * This interrupt handler is called whenever the MAX30001 ECG has new data
 * available.
 */
API_ISR(EPIC_INT_MAX30001_ECG, epic_isr_max30001_ecg);

/**
 * USB
 * ===
 */

/**
 * De-initialize the currently configured USB device (if any)
 *
 */
API(API_USB_SHUTDOWN, int epic_usb_shutdown(void));

/**
 * Configure the USB peripheral to export the internal FLASH
 * as a Mass Storage device.
 */
API(API_USB_STORAGE, int epic_usb_storage(void));

/**
 * Configure the USB peripheral to provide card10's stdin/stdout
 * on a USB CDC-ACM device.
 */
API(API_USB_CDCACM, int epic_usb_cdcacm(void));

/**
 * WS2812
 * ======
 */

/**
 * Takes a gpio pin specified with the gpio module and transmits
 * the led data. The format ``GG:RR:BB`` is expected.
 *
 * :param uint8_t pin: The gpio pin to be used for data.
 * :param uint8_t * pixels: The buffer, in which the pixel data is stored.
 * :param uint32_t n_bytes: The size of the buffer.
 *
 * .. versionadded:: 1.10
 */
API(API_WS2812_WRITE, void epic_ws2812_write(uint8_t pin, uint8_t *pixels, uint32_t n_bytes));


/**
 * Configuration
 * =============
 */

/**
 * Read an integer from the configuration file
 *
 * :param char* key: Name of the option to read
 * :param int* value: Place to read the value into
 * :return: ``0`` on success or a negative value if an error occured. Possible
 *    errors:
 *
 *    - ``-ENOENT``: Value can not be read
 *
 * .. versionadded:: 1.13
 */
API(API_CONFIG_GET_INTEGER, int epic_config_get_integer(const char *key, int *value));

/**
 * Read a boolean from the configuration file
 *
 * :param char* key: Name of the option to read
 * :param bool* value: Place to read the value into
 * :return: ``0`` on success or a negative value if an error occured. Possible
 *    errors:
 *
 *    - ``-ENOENT``: Value can not be read
 *
 * .. versionadded:: 1.13
 */
API(API_CONFIG_GET_BOOLEAN, int epic_config_get_boolean(const char *key, bool *value));

/**
 * Read a string from the configuration file.
 *
 * If the buffer supplied is too small for the config option,
 * no error is reported and the first ``buf_len - 1`` characters
 * are returned (0 terminated).
 *
 * :param char* key: Name of the option to read
 * :param char* buf: Place to read the string into
 * :param size_t buf_len: Size of the provided buffer
 * :return: ``0`` on success or a negative value if an error occured. Possible
 *    errors:
 *
 *    - ``-ENOENT``: Value can not be read
 *
 * .. versionadded:: 1.13
 */
API(API_CONFIG_GET_STRING, int epic_config_get_string(const char *key, char *buf, size_t buf_len));


/**
 * Write a string to the configuration file.
 *
 * :param char* key: Name of the option to write
 * :param char* value: The value to write
 * :return: ``0`` on success or a negative value if an error occured. Possivle
 *    errors:
 *
 *    - ``-EINVAL``: Parameters out of range
 *    - ``-ENOENT``: Key already exists but can not be read
 *    - ``-EIO``   : Unspecified I/O error
 *    - Any fopen/fread/fwrite/fclose related error code
 *
 * .. versionadded:: 1.16
 */
API(API_CONFIG_SET_STRING, int epic_config_set_string(const char *key, const char *value));


/**
 * Bluetooth Low Energy (BLE)
 * ==========================
 */

/**
 * BLE event type
 */
enum epic_ble_event_type {
	/** No event pending */
	BLE_EVENT_NONE                                    = 0,
	/** Numeric comparison requested */
	BLE_EVENT_HANDLE_NUMERIC_COMPARISON               = 1,
	/** A pairing procedure has failed */
	BLE_EVENT_PAIRING_FAILED                          = 2,
	/** A pairing procedure has successfully completed */
	BLE_EVENT_PAIRING_COMPLETE                        = 3,
	/** New scan data is available  */
	BLE_EVENT_SCAN_REPORT                             = 4,
	BLE_EVENT_ATT_EVENT                               = 5,
	BLE_EVENT_ATT_WRITE                               = 6,
	BLE_EVENT_DM_EVENT                                = 7,
};

/**
 * MicroPython Bluetooth support data types. Please
 * do not use them until they are stabilized.
 */
typedef uint8_t bdAddr_t[6];

struct epic_wsf_header
{
	/** General purpose parameter passed to event handler */
	uint16_t param;
	/** General purpose event value passed to event handler */
	uint8_t event;
	/** General purpose status value passed to event handler */
	uint8_t status;
};

struct epic_att_event
{
	/** Header structure */
	struct epic_wsf_header hdr;
	/** Value */
	uint8_t *pValue;
	/** Value length */
	uint16_t valueLen;
	/** Attribute handle */
	uint16_t handle;
	/** TRUE if more response packets expected */
	uint8_t continuing;
	/** Negotiated MTU value */
	uint16_t mtu;
};

struct epic_hciLeConnCmpl_event
{        /** Event header */
	struct epic_wsf_header hdr;
	/** Status. */
	uint8_t status;
	/** Connection handle. */
	uint16_t handle;
	/** Local connection role. */
	uint8_t role;
	/** Peer address type. */
	uint8_t addrType;
	/** Peer address. */
	bdAddr_t peerAddr;
	/** Connection interval */
	uint16_t connInterval;
	/** Connection latency. */
	uint16_t connLatency;
	/** Supervision timeout. */
	uint16_t supTimeout;
	/** Clock accuracy. */
	uint8_t clockAccuracy;

	/** enhanced fields */
	/** Local RPA. */
	bdAddr_t localRpa;
	/** Peer RPA. */
	bdAddr_t peerRpa;
};

/*! \brief Disconnect complete event */
struct epic_hciDisconnectCmpl_event
{
	/** Event header */
	struct epic_wsf_header hdr;
	/** Disconnect complete status. */
	uint8_t status;
	/** Connect handle. */
	uint16_t handle;
	/** Reason. */
	uint8_t reason;
};

struct epic_dm_event
{
	union {
		/** LE connection complete. */
		struct epic_hciLeConnCmpl_event leConnCmpl;
		/** Disconnect complete. */
		struct epic_hciDisconnectCmpl_event disconnectCmpl;
	};
};

struct epic_att_write
{
	/** Header structure */
	struct epic_wsf_header hdr;
	/** Value length */
	uint16_t valueLen;
	/** Attribute handle */
	uint16_t handle;

	uint8_t operation;
	uint16_t offset;
	void *buffer;
};

struct epic_ble_event {
	enum epic_ble_event_type type;
	union {
		void *data;
		struct epic_att_event *att_event;
		struct epic_dm_event *dm_event;
		struct epic_att_write *att_write;
	};
};

/**
 * Scan report data. Based on ``hciLeAdvReportEvt_t`` from BLE stack.
 *
 * TODO: 64 bytes for data is an arbitrary number ATM */
struct epic_scan_report
{
	/** advertising or scan response data. */
	uint8_t data[64];
	/** length of advertising or scan response data. */
	uint8_t len;
	/** RSSI. */
	int8_t rssi;
	/** Advertising event type. */
	uint8_t eventType;
	/** Address type. */
	uint8_t addrType;
	/** Device address. */
	uint8_t addr[6];

	/** direct fields */
	/** Direct advertising address type. */
	uint8_t directAddrType;
	/** Direct advertising address. */
	uint8_t directAddr[6];
};

/**
 * **Interrupt Service Routine** for :c:data:`EPIC_INT_BLE`
 *
 * :c:func:`epic_isr_ble` is called when the BLE stack wants to signal an
 * event to the application. You can use :c:func:`epic_ble_get_event` to obtain
 * the event which triggered this interrupt.
 *
 * Currently supported events:
 *
 * :c:data:`BLE_EVENT_HANDLE_NUMERIC_COMPARISON`:
 *    An ongoing pairing procedure requires a numeric comparison to complete.
 *    The compare value can be retreived using :c:func:`epic_ble_get_compare_value`.
 *
 * :c:data:`BLE_EVENT_PAIRING_FAILED`:
 *    A pairing procedure failed. The stack automatically went back advertising
 *    and accepting new pairings.
 *
 * :c:data:`BLE_EVENT_PAIRING_COMPLETE`:
 *    A pairing procedure has completed sucessfully.
 *    The stack automatically persists the pairing information, creating a bond.
 *
 * .. versionadded:: 1.16
 */
API_ISR(EPIC_INT_BLE, epic_isr_ble);

/**
 * Retreive the event which triggered :c:func:`epic_isr_ble`
 *
 * .. versionadded:: 1.16
 * .. versionchanged:: 1.17
 */
API(API_BLE_GET_EVENT, int epic_ble_get_event(struct epic_ble_event *e));

/**
 * Retrieve the compare value of an ongoing pairing procedure.
 *
 * If no pairing procedure is ongoing, the returned value is undefined.
 *
 * :return: 6 digit long compare value
 *
 * .. versionadded:: 1.16
 */
API(API_BLE_GET_COMPARE_VALUE, uint32_t epic_ble_get_compare_value(void));

/**
 * Retrieve the (file) name of the last pairing which was successful.
 *
 * :return: ``0`` on success or a negative value if an error occured. Possible
 *    errors:
 *
 *    - ``-ENOENT``: There was no successful pairing yet.
 *
 * .. versionadded:: 1.16
 */
API(API_BLE_GET_LAST_PAIRING_NAME, int epic_ble_get_last_pairing_name(char *buf, size_t buf_size));

/**
 * Retrieve the name of the peer to which we are connected
 *
 * The name might be empty if the peer device does not expose it or
 * if it has not yet been read from it.
 *
 * :return: ``0`` on success or a negative value if an error occured. Possible
 *    errors:
 *
 *    - ``-ENOENT``: There is no active connection at the moment.
 *
 * .. versionadded:: 1.16
 */
API(API_BLE_GET_PEER_DEVICE_NAME, int epic_ble_get_peer_device_name(char *buf, size_t buf_size));

/**
 * Indicate wether the user confirmed the compare value.
 *
 * If a pariring procedure involving a compare value is ongoing and this
 * function is called with confirmed set to ``true``, it will try to
 * proceed and complete the pairing process. If called with ``false``, the
 * pairing procedure will be aborted.
 *
 * :param bool confirmed: ``true`` if the user confirmed the compare value.
 *
 * .. versionadded:: 1.16
 */
API(API_BLE_COMPARE_RESPONSE, void epic_ble_compare_response(bool confirmed));

/**
 * Set the desired mode of the BLE stack.
 *
 * There are three allowed modes:
 *
 *  - Peripheral which is not bondable (bondable = ``false``, scanner = ``false``).
 *  - Peripheral which is bondable (bondable = ``true``, scanner = ``false``).
 *  - Observer which scans for advertisements (bondable = ``false``, scanner = ``true``).
 *
 * By default the card10 will not allow new bondings to be made. New
 * bondings have to explicitly allowed by calling this function.
 *
 * While bondable the card10 will change its advertisements to
 * indicate to scanning hosts that it is available for discovery.
 *
 * When scanning is active, :c:data:`BLE_EVENT_SCAN_REPORT` events will be sent
 * and the scan reports can be fetched using :c:func:`epic_ble_get_scan_report`.
 *
 * When switching applications new bondings are automatically
 * disallowed and scanning is stopped.
 *
 * :param bool bondable: ``true`` if new bondings should be allowed.
 * :param bool scanner: ``true`` if scanning should be turned on.
 *
 * .. versionadded:: 1.16
 */
API(API_BLE_SET_MODE, void epic_ble_set_mode(bool bondable, bool scanner));

/**
 * Retrieve a scan report from the queue of scan reports.
 *
 * :param struct\ epic_scan_report* rpt: Pointer where the report will be stored.
 *
 * :return: ``0`` on success or a negative value if an error occured. Possible
 *    errors:
 *
 *    - ``-ENOENT``: No scan report available
 *
 */
API(API_BLE_GET_SCAN_REPORT, int epic_ble_get_scan_report(struct epic_scan_report *rpt));

/**
 * Send an input report to the host.
 *
 * :param uint8_t report_id: The id of the report to use. 1: keyboard, 2: mouse, 3: consumer control
 * :param uint8_t* data: Data to be reported.
 * :param uint8_t len: Length in bytes of the data to be reported. Maximum length is 8 bytes.
 *
 * :return: ``0`` on success, ``1`` if the report is queued or a negative value
 *    if an error occured. Possible errors:
 *
 *    - ``-EIO``: There is no host device connected or BLE HID is not enabled.
 *    - ``-EAGAIN``: There is no space in the queue available. Try again later.
 *    - ``-EINVAL``: Either the report_id is out of range or the data is too long.
 *
 */
API(API_BLE_HID_SEND_REPORT, int epic_ble_hid_send_report(uint8_t report_id, uint8_t *data, uint8_t len));


/**
 * MicroPython BLE Support API
 * ---------------------------
 * The following API calls are to be used for MicroPython BLE support.
 *
 * .. warning::
 *
 *    The following epic-calls are **not** part of the stable public API and
 *    thus **no** guarantee about stability or behavior is made.  Do not use
 *    these outside of Pycardium unless you can live with sudden breakage!!
 *
 *    They are only documented here for completeness and as a reference for
 *    firmware hackers, not for common usage.
 */

/** Private API call for Pycardium BLE support. */
API(API_BLE_INIT, int epic_ble_init(void));
/** Private API call for Pycardium BLE support. */
API(API_BLE_DEINIT, int epic_ble_deinit(void));
/** Private API call for Pycardium BLE support. */
API(API_BLE_ATTS_DYN_CREATE_GROUP, int epic_atts_dyn_create_service(const uint8_t *uuid, uint8_t uuid_len, uint16_t group_size, void **pSvcHandle));
//API(API_BLE_ATTS_DYN_DELETE_GROUP, void AttsDynDeleteGroup(void *pSvcHandle));
/** Private API call for Pycardium BLE support. */
API(API_BLE_ATTS_DYN_DELETE_GROUPS, int epic_ble_atts_dyn_delete_groups(void));

/** Private API call for Pycardium BLE support. */
API(API_BLE_ATTS_DYN_ADD_CHARACTERISTIC, int epic_atts_dyn_add_characteristic(void *pSvcHandle, const uint8_t *uuid, uint8_t uuid_len, uint8_t flags, uint16_t maxLen, uint16_t *value_handle));
/** Private API call for Pycardium BLE support. */
API(API_BLE_ATTS_DYN_ADD_DESCRIPTOR, int epic_ble_atts_dyn_add_descriptor(void *pSvcHandle, const uint8_t *uuid, uint8_t uuid_len, uint8_t flags, const uint8_t *value, uint16_t value_len, uint16_t maxLen, uint16_t *descriptor_handle));

/** Private API call for Pycardium BLE support. */
API(API_BLE_ATTS_SEND_SERVICE_CHANGED_IND, int epic_atts_dyn_send_service_changed_ind(void));

/** Private API call for Pycardium BLE support. */
API(API_BLE_ATTS_SET_ATTR, int epic_ble_atts_set_attr(uint16_t handle, const uint8_t *value, uint16_t value_len));
/** Private API call for Pycardium BLE support. */
API(API_BLE_ATTS_HANDLE_VALUE_NTF, int epic_ble_atts_handle_value_ntf(uint8_t connId, uint16_t handle, uint16_t valueLen, uint8_t *pValue));
/** Private API call for Pycardium BLE support. */
API(API_BLE_ATTS_HANDLE_VALUE_IND, int epic_ble_atts_handle_value_ind(uint8_t connId, uint16_t handle, uint16_t valueLen, uint8_t *pValue));
/** Private API call for Pycardium BLE support. */
API(API_BLE_ATTS_SET_BUFFER, int epic_ble_atts_set_buffer(uint16_t value_handle, size_t len, bool append));

/** Private API call for Pycardium BLE support. */
API(API_BLE_FREE_EVENT, int epic_ble_free_event(struct epic_ble_event *e));
/** Private API call for Pycardium BLE support. */
API(API_BLE_CLOSE_CONNECTION, void epic_ble_close_connection(uint8_t connId));
/** Private API call for Pycardium BLE support. */
API(API_BLE_IS_CONNECTION_OPEN, int epic_ble_is_connection_open(void));
/** Private API call for Pycardium BLE support. */
API(API_BLE_SET_DEVICE_NAME, int epic_ble_set_device_name(const uint8_t *buf, uint16_t len));
/** Private API call for Pycardium BLE support. */
API(API_BLE_GET_DEVICE_NAME, int epic_ble_get_device_name(uint8_t **buf, uint16_t *len));
/** Private API call for Pycardium BLE support. */
API(API_BLE_GET_ADDRESS, void epic_ble_get_address(uint8_t *addr));

/** Private API call for Pycardium BLE support. */
API(API_BLE_ADVERTISE, int epic_ble_advertise(int interval_us, const uint8_t *adv_data, size_t adv_data_len, const uint8_t *sr_data, size_t sr_data_len, bool connectable));
/** Private API call for Pycardium BLE support. */
API(API_BLE_ADVERTISE_STOP, int epic_ble_advertise_stop(void));

/** Private API call for Pycardium BLE support. */
API(API_BLE_DISCOVER_PRIMARY_SERVICES, int epic_ble_attc_discover_primary_services(uint8_t connId, const uint8_t *uuid, uint8_t uuid_len));
/** Private API call for Pycardium BLE support. */
API(API_BLE_DISCOVER_CHARACTERISTICS, int epic_ble_attc_discover_characteristics(uint8_t connId, uint16_t start_handle, uint16_t end_handle));
/** Private API call for Pycardium BLE support. */
API(API_BLE_DISCOVER_DESCRIPTORS, int epic_ble_attc_discover_descriptors(uint8_t connId, uint16_t start_handle, uint16_t end_handle));
/** Private API call for Pycardium BLE support. */
API(API_BLE_ATTC_READ, int epic_ble_attc_read(uint8_t connId, uint16_t value_handle));
/** Private API call for Pycardium BLE support. */
API(API_BLE_ATTC_WRITE_NO_RSP, int epic_ble_attc_write_no_rsp(uint8_t connId, uint16_t value_handle, const uint8_t *value, uint16_t value_len));
/** Private API call for Pycardium BLE support. */
API(API_BLE_ATTC_WRITE, int epic_ble_attc_write(uint8_t connId, uint16_t value_handle, const uint8_t *value, uint16_t value_len));

#endif /* _EPICARDIUM_H */

