 /*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021, Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "bsp/board.h"
#include "hardware/uart.h"
#include "tusb.h"
#include "ctx.h"

extern Ctx *pico_ctx;

//--------------------------------------------------------------------+
// MACRO TYPEDEF CONSTANT ENUM DECLARATION
//--------------------------------------------------------------------+

// If your host terminal support ansi escape code such as TeraTerm
// it can be use to simulate mouse cursor movement within terminal
#define USE_ANSI_ESCAPE   0

#define MAX_REPORT  4

//static uint8_t const keycode2ascii[128][2] =  { HID_KEYCODE_TO_ASCII };

// Each HID instance can has multiple reports
static struct
{
  uint8_t report_count;
  tuh_hid_report_info_t report_info[MAX_REPORT];
}hid_info[CFG_TUH_HID];

static void process_kbd_report(hid_keyboard_report_t const *report);
static void process_mouse_report(hid_mouse_report_t const * report);
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);

void hid_app_task(void)
{
  // nothing to do
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

  char buf[512];
// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
  //sprintf(buf, "HID device address = %d, instance = %d is mounted\r\n", dev_addr, instance);
  //buffer_add_str(buf);

  // Interface protocol (hid_interface_protocol_enum_t)
  //const char* protocol_str[] = { "None", "Keyboard", "Mouse" };
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  //sprintf(buf, "HID Interface Protocol = %s\r\n", protocol_str[itf_protocol]);
  //buffer_add_str(buf);

  // By default host stack will use activate boot protocol on supported interface.
  // Therefore for this simple example, we only need to parse generic report descriptor (with built-in parser)
  if ( itf_protocol == HID_ITF_PROTOCOL_NONE )
  {
    hid_info[instance].report_count = tuh_hid_parse_report_descriptor(hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);
    //sprintf(buf, "HID has %u reports \r\n", hid_info[instance].report_count);
    //buffer_add_str(buf);
  }

  // request to receive report
  // tuh_hid_report_received_cb() will be invoked when report is available
  if ( !tuh_hid_receive_report(dev_addr, instance) )
  {
    //sprintf(buf, "Error: cannot request to receive report\r\n");
    //buffer_add_str(buf);
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  //sprintf(buf, "HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
  //buffer_add_str(buf);
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
  switch (itf_protocol)
  {
    case HID_ITF_PROTOCOL_KEYBOARD:
      TU_LOG2("HID receive boot keyboard report\r\n");
      process_kbd_report( (hid_keyboard_report_t const*) report );
    break;

    case HID_ITF_PROTOCOL_MOUSE:
      TU_LOG2("HID receive boot mouse report\r\n");
      process_mouse_report( (hid_mouse_report_t const*) report );
    break;

    default:
      // Generic report requires matching ReportID and contents with previous parsed report info
      process_generic_report(dev_addr, instance, report, len);
    break;
  }

  // continue to request to receive report
  if ( !tuh_hid_receive_report(dev_addr, instance) )
  {
   // buffer_add_str("Error: cannot request to receive report\r\n");
  }
}

//--------------------------------------------------------------------+
// Keyboard
//--------------------------------------------------------------------+

// look up new key in previous keys
static inline bool find_key_in_report(hid_keyboard_report_t const *report, uint8_t keycode)
{
  for(uint8_t i=0; i<6; i++)
  {
    if (report->keycode[i] == keycode)  return true;
  }

  return false;
}

void buffer_add_byte (const char byte);

static int translate_key (int key)
{
  switch (key)
  {
   case HID_KEY_A: return 'A';
   case HID_KEY_B: return 'B';
   case HID_KEY_C: return 'C';
   case HID_KEY_D: return 'D';
   case HID_KEY_E: return 'E';
   case HID_KEY_F: return 'F';
   case HID_KEY_G: return 'G';
   case HID_KEY_H: return 'H';
   case HID_KEY_I: return 'I';
   case HID_KEY_J: return 'J';
   case HID_KEY_K: return 'K';
   case HID_KEY_L: return 'L';
   case HID_KEY_M: return 'M';
   case HID_KEY_N: return 'N';
   case HID_KEY_O: return 'O';
   case HID_KEY_P: return 'P';
   case HID_KEY_Q: return 'Q';
   case HID_KEY_R: return 'R';
   case HID_KEY_S: return 'S';
   case HID_KEY_T: return 'T';
   case HID_KEY_U: return 'U';
   case HID_KEY_V: return 'V';
   case HID_KEY_W: return 'W';
   case HID_KEY_X: return 'X';
   case HID_KEY_Y: return 'Y';
   case HID_KEY_Z: return 'Z';
   case HID_KEY_1: return '1';
   case HID_KEY_2: return '2';
   case HID_KEY_3: return '3';
   case HID_KEY_4: return '4';
   case HID_KEY_5: return '5';
   case HID_KEY_6: return '6';
   case HID_KEY_7: return '7';
   case HID_KEY_8: return '8';
   case HID_KEY_9: return '9';
   case HID_KEY_0: return '0';
   case HID_KEY_ENTER: return '\r';
   case HID_KEY_ESCAPE: return 27;
   case HID_KEY_BACKSPACE: return '\b';
   case HID_KEY_TAB: return '\t';
   case HID_KEY_SPACE: return ' ';
   case HID_KEY_MINUS: return 189;
   case HID_KEY_EQUAL: return '=';
   case HID_KEY_BRACKET_LEFT: return 219;
   case HID_KEY_BRACKET_RIGHT: return 221;
   case HID_KEY_BACKSLASH: return 220;
   //case HID_KEY_EUROPE_1: return ;
   case HID_KEY_SEMICOLON: return 59;
   case HID_KEY_APOSTROPHE: return 222;
   case HID_KEY_GRAVE: return 192;
   case HID_KEY_COMMA: return 188;
   case HID_KEY_PERIOD: return 190;
   case HID_KEY_SLASH: return 191;
   case HID_KEY_CAPS_LOCK: return '\a'; // bell on caps-lock..
   case HID_KEY_F1: return 112;
   case HID_KEY_F2: return 113;
   case HID_KEY_F3: return 114;
   case HID_KEY_F4: return 115;
   case HID_KEY_F5: return 116;
   case HID_KEY_F6: return 117;
   case HID_KEY_F7: return 118;
   case HID_KEY_F8: return 119;
   case HID_KEY_F9: return 120;
   case HID_KEY_F10: return 121;
   case HID_KEY_F11: return 122;
   case HID_KEY_F12: return 123;
   //case HID_KEY_PRINT_SCREEN: return ;
   //case HID_KEY_SCROLL_LOCK: return ;
   //case HID_KEY_PAUSE: return ;
   case HID_KEY_INSERT: return 45;
   case HID_KEY_HOME: return 36;
   case HID_KEY_PAGE_UP: return 33;
   case HID_KEY_DELETE: return 46;
   case HID_KEY_END: return 35;
   case HID_KEY_PAGE_DOWN: return 34;
   case HID_KEY_ARROW_RIGHT: return 39;
   case HID_KEY_ARROW_LEFT: return 37;
   case HID_KEY_ARROW_DOWN: return 40;
   case HID_KEY_ARROW_UP: return 38;
   //case HID_KEY_NUM_LOCK: return ;
#if 0
   case HID_KEY_EUROPE_2: return ;
   case HID_KEY_APPLICATION: return ;
   case HID_KEY_POWER: return ;
   case HID_KEY_F13: return ;
   case HID_KEY_F14: return ;
   case HID_KEY_F15: return ;
   case HID_KEY_F16: return ;
   case HID_KEY_F17: return ;
   case HID_KEY_F18: return ;
#endif

// wrong - but aliasing makes more things work as expected for now
   //case HID_KEY_KEYPAD_MULTIPLY: return ; // XXX inexpressible with aliasing
   //case HID_KEY_KEYPAD_ADD: return ; // XXX
   case HID_KEY_KEYPAD_DIVIDE: return 191;
   case HID_KEY_KEYPAD_SUBTRACT: return 189;
   case HID_KEY_KEYPAD_ENTER: return '\r';
   case HID_KEY_KEYPAD_DECIMAL: return 190;
   case HID_KEY_KEYPAD_EQUAL: return '=';
   case HID_KEY_KEYPAD_1: return '1';
   case HID_KEY_KEYPAD_2: return '2';
   case HID_KEY_KEYPAD_3: return '3';
   case HID_KEY_KEYPAD_4: return '4';
   case HID_KEY_KEYPAD_5: return '5';
   case HID_KEY_KEYPAD_6: return '6';
   case HID_KEY_KEYPAD_7: return '7';
   case HID_KEY_KEYPAD_8: return '8';
   case HID_KEY_KEYPAD_9: return '9';
   case HID_KEY_KEYPAD_0: return '0';


   case HID_KEY_RETURN: return 13;
   case HID_KEY_CONTROL_LEFT: return 17;
   case HID_KEY_SHIFT_LEFT: return 16;
   case HID_KEY_ALT_LEFT: return 18;
   case HID_KEY_GUI_LEFT: return 19; // extrapolated guess of value
   case HID_KEY_CONTROL_RIGHT: return 17;
   case HID_KEY_SHIFT_RIGHT: return 16;
   case HID_KEY_ALT_RIGHT: return 18;
   case HID_KEY_GUI_RIGHT: return 19; // extrapolated guess of value
   default: return -1;
  }
}

static void process_kbd_report(hid_keyboard_report_t const *report)
{
  static hid_keyboard_report_t prev_report = { 0, 0, {0} };
  static uint8_t prev_modifier = 0;

  // handle modifiers, we treat left and right shift/alt/ctrl the same
  if (  ((prev_modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT))!=0) &&
       !((report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT))!=0))
    ctx_key_up(pico_ctx, 16, NULL, 0);
  if ( !((prev_modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT))!=0) &&
        ((report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT))!=0))
    ctx_key_down(pico_ctx, 16, NULL, 0);
  if ( !((prev_modifier & (KEYBOARD_MODIFIER_LEFTCTRL| KEYBOARD_MODIFIER_RIGHTCTRL))!=0) &&
        ((report->modifier & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL))!=0))
    ctx_key_down(pico_ctx, 17, NULL, 0);
  if (  ((prev_modifier & (KEYBOARD_MODIFIER_LEFTCTRL| KEYBOARD_MODIFIER_RIGHTCTRL))!=0) &&
       !((report->modifier & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL))!=0))
    ctx_key_up(pico_ctx, 17, NULL, 0);

  if ( !((prev_modifier & (KEYBOARD_MODIFIER_LEFTALT| KEYBOARD_MODIFIER_RIGHTALT))!=0) &&
        ((report->modifier & (KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_RIGHTALT))!=0))
    ctx_key_down(pico_ctx, 18, NULL, 0);
  if (  ((prev_modifier & (KEYBOARD_MODIFIER_LEFTALT| KEYBOARD_MODIFIER_RIGHTALT))!=0) &&
       !((report->modifier & (KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_RIGHTALT))!=0))
    ctx_key_up(pico_ctx, 18, NULL, 0);
  prev_modifier = report->modifier;

  // handle other keys presses
  for(uint8_t i=0; i<6; i++)
  {
    if ( report->keycode[i] )
    {
      if (!find_key_in_report(&prev_report, report->keycode[i]) )
      {
        int translated_code = translate_key(report->keycode[i]);
        ctx_key_down(pico_ctx, translated_code, NULL, 0);
        ctx_key_press(pico_ctx, translated_code, NULL, 0);
      }
    }
  }

  // handle other keys releases
  for(uint8_t i=0; i<6; i++)
  {
    if (prev_report.keycode[i])
    {
      if (!find_key_in_report(report, prev_report.keycode[i]))
      {
        ctx_key_up(pico_ctx, translate_key(prev_report.keycode[i]), NULL, 0);
      }
    }
  }

  prev_report = *report;
}

//--------------------------------------------------------------------+
// Mouse
//--------------------------------------------------------------------+

void cursor_movement(int8_t x, int8_t y, int8_t wheel)
{
#if USE_ANSI_ESCAPE
  // Move X using ansi escape
  if ( x < 0)
  {
    printf(ANSI_CURSOR_BACKWARD(%d), (-x)); // move left
  }else if ( x > 0)
  {
    printf(ANSI_CURSOR_FORWARD(%d), x); // move right
  }

  // Move Y using ansi escape
  if ( y < 0)
  {
    printf(ANSI_CURSOR_UP(%d), (-y)); // move up
  }else if ( y > 0)
  {
    printf(ANSI_CURSOR_DOWN(%d), y); // move down
  }

  // Scroll using ansi escape
  if (wheel < 0)
  {
    printf(ANSI_SCROLL_UP(%d), (-wheel)); // scroll up
  }else if (wheel > 0)
  {
    printf(ANSI_SCROLL_DOWN(%d), wheel); // scroll down
  }

  printf("\r\n");
#else
  printf("(%d %d %d)\r\n", x, y, wheel);
#endif
}

static void process_mouse_report(hid_mouse_report_t const * report)
{
  static hid_mouse_report_t prev_report = { 0 };

  //------------- button state  -------------//
  uint8_t button_changed_mask = report->buttons ^ prev_report.buttons;
  if ( button_changed_mask & report->buttons)
  {
    printf(" %c%c%c ",
       report->buttons & MOUSE_BUTTON_LEFT   ? 'L' : '-',
       report->buttons & MOUSE_BUTTON_MIDDLE ? 'M' : '-',
       report->buttons & MOUSE_BUTTON_RIGHT  ? 'R' : '-');
  }

  //------------- cursor movement -------------//
  cursor_movement(report->x, report->y, report->wheel);
}

//--------------------------------------------------------------------+
// Generic Report
//--------------------------------------------------------------------+
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  (void) dev_addr;

  uint8_t const rpt_count = hid_info[instance].report_count;
  tuh_hid_report_info_t* rpt_info_arr = hid_info[instance].report_info;
  tuh_hid_report_info_t* rpt_info = NULL;

  if ( rpt_count == 1 && rpt_info_arr[0].report_id == 0)
  {
    // Simple report without report ID as 1st byte
    rpt_info = &rpt_info_arr[0];
  }else
  {
    // Composite report, 1st byte is report ID, data starts from 2nd byte
    uint8_t const rpt_id = report[0];

    // Find report id in the arrray
    for(uint8_t i=0; i<rpt_count; i++)
    {
      if (rpt_id == rpt_info_arr[i].report_id )
      {
        rpt_info = &rpt_info_arr[i];
        break;
      }
    }

    report++;
    len--;
  }

  if (!rpt_info)
  {
    //buffer_add_str ("Couldn't find the report info for this report !\r\n");
    return;
  }

  // For complete list of Usage Page & Usage checkout src/class/hid/hid.h. For examples:
  // - Keyboard                     : Desktop, Keyboard
  // - Mouse                        : Desktop, Mouse
  // - Gamepad                      : Desktop, Gamepad
  // - Consumer Control (Media Key) : Consumer, Consumer Control
  // - System Control (Power key)   : Desktop, System Control
  // - Generic (vendor)             : 0xFFxx, xx
  if ( rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP )
  {
    switch (rpt_info->usage)
    {
      case HID_USAGE_DESKTOP_KEYBOARD:
        TU_LOG1("HID receive keyboard report\r\n");
        // Assume keyboard follow boot report layout
        process_kbd_report( (hid_keyboard_report_t const*) report );
      break;

      case HID_USAGE_DESKTOP_MOUSE:
        TU_LOG1("HID receive mouse report\r\n");
        // Assume mouse follow boot report layout
        process_mouse_report( (hid_mouse_report_t const*) report );
      break;

      default: break;
    }
  }
}
