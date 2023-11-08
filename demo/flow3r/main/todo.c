#include "ui.h"

void view_todo (Ui *ui)
{
   ui_start (ui);
   ui_text(ui,"animated slider steps");
   ui_text(ui,"espnow-chat");
   ui_text(ui,"scrolling/clipping/different positioning of text");
   ui_text(ui,"selection in text entry");
   ui_text(ui,"micropython");
   ui_text(ui,"text editor");
   ui_end(ui);
}
