#include "ui.h"

void view_todo (Ui *ui)
{
   ui_start (ui);
   ui->y += ui->line_height;
   ui_text(ui,"last and first item for wrap");
   ui_text(ui,"scaling/scrolling/clipping/different positioning of text");
   ui_text(ui,"selection in text entry");
   ui->y += ui->line_height;
   ui_text(ui,"espnow-chat");
   ui_text(ui,"micropython");
   ui_text(ui,"redo textview");
   ui_text(ui,"text editor");
   ui_text(ui,"commandline");
   ui_text(ui,"console");
   ui_text(ui,"view transitions");
   ui_text(ui,"css");
   ui_end(ui);
}
