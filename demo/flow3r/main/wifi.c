#include "ui.h"

static char *wifi_ssid     = NULL;
static char *wifi_password = NULL;
static bool wifi_connected = false;
void view_wifi (Ui *ui)
{
   if (ui->data == NULL)
   {
     if (!wifi_ssid) wifi_ssid = strdup (CTX_DEMO_WIFI_SSID);
     if (!wifi_password) wifi_password = strdup (CTX_DEMO_WIFI_PASSWORD);
     FILE *file = fopen("/sd/w1f1_config.json", "rb");
     fseek(file, 0, SEEK_END);
     long length = ftell(file);
     fseek(file, 0, SEEK_SET);
     ui->data = malloc(length + 1);
     fread(ui->data, length, 1, file);
     fclose(file);
     ((char*)ui->data)[length] = 0;
     ui->data_finalize = free;
   }
   ui_start (ui);
   ui->y += ui->height * 0.05;
   ui_text(ui,"wifi");

   ui_entry(ui,"essid", "wifi name", &wifi_ssid);
   ui_entry(ui,"password", "joshua", &wifi_password);

   if (ui->data)
   {
   uint8_t *p = ui->data;
    char ssid[128];
    int ssid_len = 0;
    char psk[128];
    int psk_len = 0;

   for (; *p && *p != '{'; p++);
   if (*p) p++;

   for (; *p && *p != '{'; p++);
   if (*p) p++;
      
     do {
        ssid_len = 0;
        psk_len = 0;
       for (; *p && (*p == ' '||*p== '\n'); p++);
       if (*p == '"'){
         p++;
         for (; *p && (*p != '"'); p++){
           if (ssid_len < 126) ssid[ssid_len++]=*p;
         }
         ssid[ssid_len] = 0;
         
         for (; *p && *p != ':'; p++);
         if (*p) p++;
         for (; *p && *p != ':'; p++);
         if (*p) p++;

         for (; *p && ((*p == ' ')||(*p== '\n')); p++);
         if (*p == '"'){
           p++;
           for (; *p && (*p != '"'); p++){
             if (psk_len < 126) psk[psk_len++]=*p;
           }
           
         }

         for (; *p && *p != '}'; p++);
         if (*p) p++;
       
         for (; *p && ((*p == ' ')||(*p== '\n')); p++);
         if (*p == ',') p++;
         for (; *p && ((*p == ' ')||(*p== '\n')); p++);

         psk[psk_len] = 0;
       
         if (ui_button (ui, ssid))
         {
            if (wifi_ssid) free (wifi_ssid);
            wifi_ssid = strdup (ssid);
            if (wifi_password) free (wifi_password);
            wifi_password= strdup (psk);
         }
       }
     } while (*p && *p != '}');
   }

   if (wifi_connected)
     ui_text(ui,"connected");
   else
   if (ui_button(ui,"connect"))
   {
#if CTX_ESP
     wifi_connected = !wifi_init_sta(wifi_ssid, wifi_password); 
#endif
   }

   ui_end (ui);
}

