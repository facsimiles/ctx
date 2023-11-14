#include "port_config.h"
#include "ui.h"
#ifndef CTX_DEMO_SSH_USER
#define CTX_DEMO_SSH_USER "pi"
#endif
#ifndef CTX_DEMO_SSH_PASSWORD
#define CTX_DEMO_SSH_PASSWORD "raspberry"
#endif
#ifndef CTX_DEMO_SSH_HOST
#define CTX_DEMO_SSH_HOST "raspberrypi"
#endif

static CtxClient *term_client = NULL;

static void term_handle_event (Ctx        *ctx,
                               CtxEvent   *ctx_event,
                               const char *event)
{
  ctx_client_feed_keystring (term_client, ctx_event, event);
}
static void terminal_key_any (CtxEvent *event, void *userdata, void *userdata2)  
{
  Ui *ui = userdata;
  switch (event->type)
  {
    case CTX_KEY_PRESS:
      term_handle_event (event->ctx, event, event->string);
      break;
    case CTX_KEY_UP:
      { char buf[1024];
        snprintf (buf, sizeof(buf)-1, "keyup %i %i", event->unicode, event->state);
        term_handle_event (event->ctx, event, buf);
      }
      break;
    case CTX_KEY_DOWN:
      { char buf[1024];
      if (!strcmp (event->string, "escape")){ ui_do(ui, "back"); return;}
        snprintf (buf, sizeof(buf)-1, "keydown %i %i", event->unicode, event->state);
        term_handle_event (event->ctx, event, buf);
      }
      break;
    default:
      break;
  }
}

int ssh_connect(Ctx *ctx, const char *host, int port, const char *user, const char *password);
static char *ssh_host = NULL;
static char *ssh_user = NULL;
static char *ssh_port = NULL;
static char *ssh_password = NULL;
static int ssh_connected = 0;

static void view_term (Ui *ui)
{
   Ctx *ctx = ui->ctx;
   ui_start (ui);
   float font_size = ctx_height(ctx)/17;
   if (!ssh_host)
   {
     ssh_host = strdup("192.168.92.98");
     ssh_user = strdup(CTX_DEMO_SSH_USER);
     ssh_port = strdup("22");
     ssh_password = strdup(CTX_DEMO_SSH_PASSWORD);
   }
   if (!term_client)
   {
      int flags = 0;
      term_client = ctx_client_new_argv (ctx, NULL, 0,0,ctx_width(ctx),
                                  ctx_height(ctx), font_size,flags, NULL, NULL);
    
      ctx_client_maximize(ctx, ctx_client_id(term_client));
      ctx_client_resize (ctx, ctx_client_id(term_client), ctx_width(ctx)*216/240, ctx_height(ctx)*180/240);
#
      ctx_osk_mode = 2;
   }
   else if (!ctx_osk_mode) ctx_osk_mode = 1;
   
   switch (ssh_connected)
   {
     case 2:
{
     ctx_save(ctx);
     ctx_gray(ctx,0);
     ctx_listen (ctx, CTX_KEY_PRESS, terminal_key_any, ui, NULL);
     ctx_listen (ctx, CTX_KEY_DOWN,  terminal_key_any, ui, NULL);
     ctx_listen (ctx, CTX_KEY_UP,    terminal_key_any, ui, NULL);
     if (ctx_osk_mode > 1)
     {
       int y = ctx_vt_cursor_y (term_client) * font_size;
       ctx_translate (ctx, ctx_width(ctx) * 12/240, ctx_height(ctx)*80/240-y);
     }
     else
       ctx_translate (ctx, ctx_width(ctx) * 12/240, ctx_height(ctx)*35/240);
     ctx_clients_draw (ctx, 0);

     ctx_restore(ctx);
     }break;
     case 0:
       if (ui_button(ui,"connect"))
       {
         if (ssh_connect(ctx, ssh_host, atoi(ssh_port), ssh_user, ssh_password))
         {
           ssh_connected = 2;
           ui_set_scroll_offset (ui, 0);
           ui->focused_id = NULL;
         }
       }
       ui_entry(ui,"host", "ip hostname", &ssh_host);
       ui_entry(ui,"port", "22", &ssh_port);
       ui_entry(ui,"user", "joe", &ssh_user);
       ui_entry(ui,"password", "password", &ssh_password);
       break;
     case 1:
        ui_text(ui,"unused view");
        ui_text(ui,"for conn error?");
       break;
   }

   ui_end (ui);
   ctx_clients_handle_events (ctx);
}
