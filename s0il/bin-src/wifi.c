#ifndef PORT_CONFIG
#include "port_config.h"
#endif
#include "s0il.h"

static char *wifi_ssid = NULL;
static char *wifi_password = NULL;
static bool wifi_connected = false;

static char *wifi_contents = NULL;
#if CTX_ESP
int wifi_init_sta(const char *ssid_arg, const char *password_arg);
char **wifi_scan(void);
#endif

static char **wifis = NULL;

void view_wifi(Ui *ui) {
  ui_start_frame(ui);
  ui_text(ui, "wifi");

  if (wifi_connected)
    ui_text(ui, "connected");
  else if (ui_button(ui, "connect")) {
#if CTX_ESP
    wifi_connected = !wifi_init_sta(wifi_ssid, wifi_password);
#endif
  }

  ui_seperator(ui);

  ui_entry_realloc(ui, "essid", "wifi name", &wifi_ssid);
  ui_entry_realloc(ui, "password", "joshua", &wifi_password);

  for (int i = 0; wifis[i]; i++) {
    if (ui_button(ui, wifis[i])) {
      if (wifi_ssid)
        free(wifi_ssid);
      wifi_ssid = strdup(wifis[i]);
      if (wifi_password)
        free(wifi_password);
      wifi_password = NULL;

      // XXX - what about password
    }
  }

  ui_end_frame(ui);
}

const char *_wifis[] = {"mutus", "nomen", "dedit", "cocis", NULL};

MAIN(wifi) {
  Ui *ui = ui_host(NULL);
  Ctx *ctx = ui_ctx(ui);
  wifi_contents = NULL;

  if (wifi_contents == NULL) {
    if (!wifi_ssid)
      wifi_ssid = strdup("");
    if (!wifi_password)
      wifi_password = strdup("");
    FILE *file = fopen("/sd/w1f1_config.json", "rb");
    if (file) {
      fseek(file, 0, SEEK_END);
      long length = ftell(file);
      fseek(file, 0, SEEK_SET);
      wifi_contents = malloc(length + 1);
      fread(wifi_contents, length, 1, file);
      fclose(file);
      ((char *)wifi_contents)[length] = 0;
    }
  }

#if CTX_ESP
  // printf ("scanning wifis\n");
  wifis = wifi_scan();
#else
  wifis = (char **)_wifis;
#endif

  if (wifi_contents) {
    char *p = wifi_contents;
    char ssid[128];
    int ssid_len = 0;
    char psk[128];
    int psk_len = 0;

    for (; *p && *p != '{'; p++)
      ;
    if (*p)
      p++;

    for (; *p && *p != '{'; p++)
      ;
    if (*p)
      p++;

    do {
      ssid_len = 0;
      psk_len = 0;
      for (; *p && (*p == ' ' || *p == '\n'); p++)
        ;
      if (*p == '"') {
        p++;
        for (; *p && (*p != '"'); p++) {
          if (ssid_len < 126)
            ssid[ssid_len++] = *p;
        }
        ssid[ssid_len] = 0;

        for (; *p && *p != ':'; p++)
          ;
        if (*p)
          p++;
        for (; *p && *p != ':'; p++)
          ;
        if (*p)
          p++;

        for (; *p && ((*p == ' ') || (*p == '\n')); p++)
          ;
        if (*p == '"') {
          p++;
          for (; *p && (*p != '"'); p++) {
            if (psk_len < 126)
              psk[psk_len++] = *p;
          }
        }

        for (; *p && *p != '}'; p++)
          ;
        if (*p)
          p++;

        for (; *p && ((*p == ' ') || (*p == '\n')); p++)
          ;
        if (*p == ',')
          p++;
        for (; *p && ((*p == ' ') || (*p == '\n')); p++)
          ;

        psk[psk_len] = 0;

        if (wifis)
          for (int i = 0; wifis[i]; i++) {
            if (!strcmp(ssid, wifis[i])) {
              if (wifi_ssid == NULL || strcmp(wifi_ssid, wifis[i])) {
                if (wifi_ssid)
                  free(wifi_ssid);
                wifi_ssid = strdup(wifis[i]);
                if (wifi_password)
                  free(wifi_password);
                wifi_password = strdup(psk);
              }
            }
          }
      }
    } while (*p && *p != '}');
  }
#if CTX_ESP
  if (wifi_ssid && wifi_ssid[0])
    wifi_connected = !wifi_init_sta(wifi_ssid, wifi_password);
#endif

  if (argv[1] && !strcmp(argv[1], "--auto")) {
    sleep(1);
    return 0;
  }

  do {
    ctx_start_frame(ctx);
    view_wifi(ui);
    ui_add_key_binding(ui, "escape", "exit", "leave view");
    ui_add_key_binding(ui, "backspace", "exit", "leave view");
    ui_keyboard(ui);
    ctx_end_frame(ctx);
  } while (!ctx_has_exited(ctx));
  if (wifis != (char **)_wifis)
    free(wifis);

  if (wifi_contents)
    free(wifi_contents);
  wifi_contents = NULL;

  ctx_reset_has_exited(ctx);

  return 0;
}
