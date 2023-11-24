#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#if CTX_FLOW3R

#include "lwip/sockets.h"
#include "lwip/igmp.h"
#include "lwip/ip4.h"
#include "lwip/netdb.h"

#else

#include <sys/socket.h>
#include <netinet/in.h>

#endif

#include <sys/types.h>
#include <dirent.h>

#include "ui.h"

#define OUTS(s)      ctx_string_append_str(req->body, s)
#define OUTF(f, ...) ctx_string_append_printf(req->body, f, __VA_ARGS__)

typedef struct {
  char     *method;         /* will be GET */
  char     *path;           /* path requested from the webserver */

  const char *mime_type;      /* NULL to skip */
  int         content_length; /* -1 to skip */
  time_t      last_modified;  /* -1 to skip */
  char       *extra_headers;  /* NULL to skip */
                              // is not freed - we do not support concurrent
                              // requests and use static bufs for this

  CtxString *body;          /* the responsibility of the request handler
                               is to fill in the body, the preassumed
                               status is 200 OK - and if no further changes
                               are done - on POST requests this contains the 
                               received post body, and should be cleared
                               for the response 
                             */

  char     *protocol;       /* HTTP/1.x */
  int       status;         /* http status code to give */

  char     *status_string;  /* OK/not found/internal foobar */
  
  void     *user_data;
  FILE     *f;
  int       emitted;
} HttpdRequest;

typedef struct filemapping {
  const char *path;
  const char *fs_path;
} filemapping;

static filemapping filemappings[]={
  {"/","/sd/webassets/index.html"},
  {"/favicon.ico","/sd/webassets/favicon.ico"},
  {NULL, NULL},
};

static char httpd_buf2[1024 * 32];
static char httpd_buf[2048];

static char *httpd_css = 
"body { background:black; color:white;}\n"
"a { color:white;}\n"
"a:visited { color:white;}\n"
".dir_listing { list-style: none; background: #123; margin-top:0.5em;margin-left:auto;margin-right:auto; }\n"
".mime_type { float: color: #777; }\n"
".size { float: right; color: #aaa;}\n"
".dir_listing td { padding-left: 0.5em; padding-right: 0.5em;}\n"
".path { border-bottom: 0.05em solid gray;}\n"
".view textarea{ width: 100%; }\n"
".view img{ max-width: 100%; max-height : 100%; }\n"
"\n";

typedef struct dir_entry_t
{
  char       *name;
  char       *path;
  const char *mime_type;
  size_t      size;
} dir_entry_t;

typedef struct dir_listing_t
{
  dir_entry_t *entries;
  char *path;
  int count;
  int capacity;
} dir_listing_t;

static void dir_listing_destroy (dir_listing_t *di)
{
  if (!di) return;
  free (di->path);
  if (di->entries)
  {
  for (int i = 0; i < di->count; i++)
  {
    free (di->entries[i].name);
    free (di->entries[i].path);
  }
  }
  free (di->entries);
  free (di); 
}

static int
cmp_dir_entry (const void *p1, const void *p2)
{
  const dir_entry_t *e1 = p1;
  const dir_entry_t *e2 = p2;

  int e1_is_dir = e1->mime_type && !strcmp (e1->mime_type, "inode/directory");
  int e2_is_dir = e2->mime_type && !strcmp (e2->mime_type, "inode/directory");

  if (e2_is_dir - e1_is_dir)
    return (e2_is_dir - e1_is_dir);
  if (!e1->name) return 0;
  if (!e2->name) return 0;
  return strcmp (e1->name, e2->name);
}

dir_listing_t *dir_listing_read (HttpdRequest *req, const char *path)
{
  dir_listing_t *di = calloc (sizeof (dir_listing_t), 1);
      DIR *dir = opendir(path);

        di->path = strdup (path);
      if (dir)
      {
        struct dirent *ent;

        while ((ent = readdir(dir)))
        {
          const char *base = ent->d_name;

          if (base[0] == '.')
            continue;

          if (di->count + 1 >= di->capacity)
          {
            di->capacity += 16;
            di->entries = realloc (di->entries, sizeof (dir_entry_t) * di->capacity);
          }
          dir_entry_t *de = &di->entries[di->count++];
          de->name = strdup (base);
          de->path = malloc (strlen (path) + 3 + strlen (base));
          if (path[strlen(path)-1]=='/')
            sprintf(de->path, "%s%s", path, base);
          else
            sprintf(de->path, "%s/%s", path, base);
          de->mime_type = ui_get_mime_type (NULL, de->path);
          struct stat info;
          stat (de->path, &info);
          de->size = info.st_size;
        }
        closedir(dir);
        qsort(di->entries, di->count, sizeof(dir_entry_t), cmp_dir_entry);
      }

  return di;
}

void append_dir_listing (HttpdRequest *req, dir_listing_t *di, const char *prefix)
{
  if (!prefix) prefix = "";
  OUTS( "<table class='dir_listing'>\n");
  OUTF("<tr><td colspan='3' class='path'>%s</td></tr>\n", di->path);
  for (int i = 0; i < di->count; i++)
   {
     dir_entry_t *de = &di->entries[i];
     const char *mime_type = de->mime_type;

     OUTS("<tr><td>");

     if (mime_type && !strcmp (mime_type, "inode/directory"))
       OUTF( "<a href='%s%s/'><span class='name'>%s</span></a>/\n", prefix, de->path, de->name);
     else
       OUTF( "<a href='%s%s'><span class='name'>%s</span></a>\n", prefix, de->path, de->name);
     OUTS("</td><td class='size'>");

     if (mime_type && strcmp (mime_type, "inode/directory"))
     {
       OUTF( "%i", de->size);
     }
     OUTS( "</td><td>");

     if (mime_type)
     OUTF("<span class='mime_type'>%s</span></td>", mime_type);
     OUTS( "</tr>\n");
   }
  OUTS( "</table>\n");
}

static inline int decode_hex_digit(char digit)
{
 if (digit >='A' && digit <='F') return ((digit-'A')+10);
 if (digit >='a' && digit <='f') return ((digit-'a')+10);
 if (digit >='0' && digit <='9') return (digit-'0');
 return 0;
}

char *decode_uri (const char *uri)
{
  const char *p = uri;
  CtxString *str = ctx_string_new ("");

  while (*p)
  {
    switch (*p)
    {
      case '%':
        if (p[1] && p[2]) {
          ctx_string_append_byte (str,
             decode_hex_digit(p[1])*16+decode_hex_digit(p[2]));
          p+=2;
        }
        break;
      default:
        ctx_string_append_byte (str, *p);
    }
    p++;
  }
  return ctx_string_dissolve (str);
}

static bool mime_is_text(const char *mime)
{
  if (!mime) return false;
  return !strcmp (mime, "text/plain") ||
         !strcmp (mime, "text/css") ||
         !strcmp (mime, "text/html") ||
         !strcmp (mime, "text/markdown") ||
         !strcmp (mime, "application/javascript");

}



static void httpd_browse_handler (HttpdRequest *req)
{
      dir_listing_t *di = NULL;

      char *decoded_path = decode_uri (req->path + 2);
      const char *path = decoded_path;
      const char *item_path = path;
      const char *mime_type = ui_get_mime_type(NULL, path);

      char *dirnam = NULL;
      if (strcmp (mime_type, "inode/directory"))
      {
        dirnam = strdup (path);
        if (strrchr (dirnam, '/'))
          *(strrchr (dirnam, '/')+1)=0;
        path = dirnam;
      }

  req->mime_type = "text/html";
  OUTS( "<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Strict//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd'>\n");
  OUTF( "<html><head><title>%s</title>\n<style type='text/css'>\n%s</style>\n", req->path, httpd_css);
    OUTS( "<link rel='stylesheet' href='/sd/webassets/codemirror.css'/>\n"
          "<link rel='stylesheet' href='/sd/webassets/codemirror-cobalt.css'/>\n"
          "<script src='/sd/webassets/codemirror.js'></script>\n"
          "<script src='/sd/webassets/codemirror-python.js'></script>\n"
          "<script>"
   "window.unblock_savemod = false;"
   "window.onkeypress= function(event) { }"
   "window.onbeforeunload = function(event) {"
      "if (!window.unblock_savemod &&  content.value != content.defaultValue) {"
          " event.preventDefault();"
   "}}</script>\n"
    "</head>\n"
   );

   ctx_string_append_printf(req->body, "<body>");

  {

    if (!strcmp (mime_type, "inode/directory"))
    {
      di = dir_listing_read(req, req->path + 2);
     }

    OUTS(
      
      "<form enctype='text/plain' name='fo' id='fo' method='POST'><div class='toolbar'>"

      "<input value='parent' type='submit' name='action'/>"
    );

    if (!strcmp (mime_type, "inode/directory"))
    {
      if (di->count == 0)
         OUTS(
            " <input type='submit' value='rmdir' name='action'/>");
      OUTS(
         " <input type='submit' value='new dir' name='action'/>"
         " <input type='submit' value='new file' name='action'/>"
         " <input value='' name='param'/>");
    }
    else
    {

      OUTF(" <input value='%s' name='param'/>", ui_basename(item_path));

      if (mime_is_text(mime_type))
      {
         OUTS(
  " <input value='save' type='submit' name='action' onclick='window.unblock_savemod=true;'/>"
  " <input value='run' type='submit' name='action' onclick='window.unblock_savemod=true;'/>"
  " <input value='reload' style='float:right' type='submit' name='action' onclick='window.unblock_savemod=true;'/>"

);
      }

    OUTS( " <input value='rename' type='submit' name='action' />");
    OUTF( " <a href='%s'>download</a>", item_path);
    OUTF( " <span class='mime_type'>%s</span>", mime_type);

    OUTS( " <input value='remove' style='float:right' type='submit' name='action' onclick='window.unblock_savemod=true;'/>");
    }
    OUTS( " <input value='stop' style='float:right' type='submit' name='action' onclick='window.unblock_savemod=true;'/>");

    OUTF( "<input type='hidden' value='%s' name='origname'/>", ui_basename(item_path));
    OUTF( "<input type='hidden' value='%s' name='path'/>", path);

    OUTS(
      "</div>\n"); // toolbar

    OUTS( "<div class='view'>\n");

    if (mime_is_text (mime_type))
    {
    OUTS("<textarea name='content' id='content'>");
    FILE *f = fopen (item_path, "rb");
    if (f){
        char buf[512];
        int read = 0;
        do {
          read = fread(buf, 1, 512, f);
          if (read > 0)
          for (int i = 0; i < read; i++)
          {
            switch (buf[i])
            {
              case '<': ctx_string_append_data (req->body, "&lt;", 4); break;
              case '>': ctx_string_append_data (req->body, "&gt;", 4); break;
              case '&': ctx_string_append_data (req->body, "&amp;", 4); break;
              default:
                ctx_string_append_data (req->body, &buf[i], 1);
            }
          }
        } while (read > 0);
        fclose (f);
    }
    OUTS( "</textarea>");
     }
    else
    {
      OUTS("<input type='hidden' value='' name='content' id='content'/>");
    }

    if (mime_is_text (mime_type)) {}
    else if (!strcmp (mime_type, "image/jpeg")||
             !strcmp (mime_type, "image/gif")||
             !strcmp (mime_type, "image/png"))
     {
        OUTF("<a href='%s'><img src='%s'/></a>", item_path, item_path);
     }
    else if (!strcmp (mime_type, "inode/directory"))
    {


  append_dir_listing(req, di, "/_");
  dir_listing_destroy (di);

    }
    else
     {
        
     }

    OUTS( "</div>\n"); // view

    OUTS( "</form>\n");
  }

  if(dirnam) free (dirnam);
  if (decoded_path)
  free (decoded_path);

  if (mime_is_text (mime_type))
    OUTS( "<script>editor = "
  "CodeMirror.fromTextArea(document.getElementById('content'), {"
  " lineNumbers: true,"
  "theme:'cobalt'});</script>");

  OUTS( "</body></html>\n");
}

static void httpd_dir_handler (HttpdRequest *req)
{
  dir_listing_t *di = dir_listing_read(req, req->path);

  req->mime_type = "text/html";

  OUTS( "<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Strict//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd'>\n");
  OUTF( "<html><head><title>%s</title>\n<style type='text/css'>\n%s</style>\n", req->path, httpd_css);

  OUTS( "</head><body>");

  append_dir_listing (req, di, "");
#if 0
  OUTS( "<ul class='dir_listing'>");

  for (int i = 0; i < di->count; i++)
   {
     dir_entry_t *de = &di->entries[i];
     const char *mime_type = ui_get_mime_type (NULL, de->path);

     if (!strcmp (mime_type, "inode/directory"))
     {
       OUTF( "<li><a href='%s/'><span class='name'>%s</span></a>/</li>", de->path, de->name, mime_type);
     }
     else
     OUTF( "<li><a href='%s'><span class='name'>%s</span></a>  <span class='mime_type'>%s</span></li>", de->path, de->name, mime_type);
   }
  OUTS( "</ul>");
#endif
  dir_listing_destroy (di);

  OUTS( "</body></html>");

  //free (path);
}




typedef enum action_t {
  action_nop = 0,
  action_parent,
  action_remove,
  action_stop,
  action_confirm_remove,
  action_rename,
  action_reload,
  action_rmdir,
  action_run,
  action_save,
  action_mkdir,
  action_mkfile,
} action_t;

static int httpd_stop = 0;
static int rno = 0;
#include "ui.h"
#include <fcntl.h>

static void
ctx_string_append_time (CtxString *str, time_t time)
{
  char timebuf[128];
  struct tm tmp;
  char *rfc1123fmt = "%a, %d %b %Y %H:%M:%S GMT";
  memset (timebuf, 0, sizeof (timebuf));
  strftime (timebuf, sizeof (timebuf), rfc1123fmt, gmtime_r (&time, &tmp));
  ctx_string_append_str (str, timebuf);
}


static char *http_headers (HttpdRequest *req, int content_length)
{
  CtxString *str = ctx_string_new ("");
  ctx_string_append_printf (str, "%s %d %s\r\n", "HTTP/1.0", req->status, req->status_string);
  ctx_string_append_printf (str, "Server: ctx\r\n");

  //ctx_string_append_str (str, "Date: ");
  //ctx_string_append_time (str, time (NULL));
  //ctx_string_append_str (str, "\r\n");

  if (req->mime_type)
  {
    if (strstr (req->mime_type, "text/"))
    ctx_string_append_printf (str, "Content-Type: %s; charset=UTF-8\r\n", req->mime_type);
    else
    ctx_string_append_printf (str, "Content-Type: %s\r\n", req->mime_type);
  }
  if (req->content_length >= 0)
    ctx_string_append_printf (str, "Content-Length: %d\r\n", content_length);
  if (req->extra_headers)
    ctx_string_append_printf (str, "%s\r\n", req->extra_headers);

  if (req->last_modified != -1)
  {
    ctx_string_append_str (str, "Last-Modified: "); 
    ctx_string_append_time (str, req->last_modified);
    ctx_string_append_str (str, "\r\n");
  }
  ctx_string_append_printf (str, "Connection: close\r\n\r\n");

  return ctx_string_dissolve (str);
}

static void
httpd_serve_file (HttpdRequest *req, const char *path)
{
      const char *mime_type = ui_get_mime_type(NULL, path);
      req->mime_type = mime_type;
      int content_length = 0;
      FILE *file = fopen (path, "rb");
      printf ("%s\n", req->mime_type);
      req->status = 404;
      if (file)
      {
        fseek(file, 0, SEEK_END);
        req->content_length = content_length = ftell(file);
        fseek(file, 0, SEEK_SET);
        req->extra_headers = "Cache-Control: public, max-age=1734480000";
        req->status = 200;
        char *header = http_headers (req, req->content_length);
        fprintf (req->f, "%s", header);
        free (header);
        ctx_string_set (req->body, "");
        int read_total = 0;
        char buf[2048];
        int read = 0;
        
        do {
           int to_read = content_length - read_total;
           if (to_read > (int)sizeof(buf))to_read = sizeof(buf);
           read = fread (buf, 1, to_read, file);
           if (read>0) {
             printf ("%i of %i to %p\n", read_total, content_length, req->f);
           fwrite (buf, 1, read, req->f);
           read_total += read;
           }
        } while ((read > 0) && (read_total < content_length));
        fclose (file);

  req->emitted = 1;
  printf("logemitF:%i %s %s %i %s\n", req->status, req->method,
        req->status_string, req->content_length, req->path);
      }

  if (req->body)
  {
    ctx_string_free (req->body, 1);
    req->body = NULL;
  }
}

static void httpd_post_handler (HttpdRequest *req)
{
  req->status = 200;
  action_t action = 0;

  char *post_param = NULL;
  char *post_path = NULL;
  char *post_origname = NULL;
  char *post_content = NULL;

  char *p;
#define get_arg(var, name)\
  if ((p=strstr(req->body->str, name)))\
  {\
     p += strlen (name);\
     int len = strchr (p, '\r')-p;\
     var= malloc (len + 3);\
     memcpy (var, p, len);\
     var[len]=0;\
  }
  get_arg(post_param, "param=")
  get_arg(post_origname, "origname=")
  get_arg(post_path, "path=")
  post_content = strstr (req->body->str, "content=");
  if (post_content) post_content += strlen ("content=");

  if (strstr(req->body->str, "action=save") ==
      strstr(req->body->str, "action=")) action = action_save;
  else if (strstr(req->body->str, "action=parent") ==
           strstr(req->body->str, "action=")) action = action_parent;
  else if (strstr(req->body->str, "action=stop") ==
           strstr(req->body->str, "action=")) action = action_stop;
  else if (strstr(req->body->str, "action=remove") ==
           strstr(req->body->str, "action=")) action = action_remove;
  else if (strstr(req->body->str, "action=confirm remove") ==
           strstr(req->body->str, "action=")) action = action_confirm_remove;
  else if (strstr(req->body->str, "action=rename") ==
           strstr(req->body->str, "action=")) action = action_rename;
  else if (strstr(req->body->str, "action=rmdir") ==
           strstr(req->body->str, "action=")) action = action_rmdir;
  else if (strstr(req->body->str, "action=run") ==
           strstr(req->body->str, "action=")) action = action_run;
  else if (strstr(req->body->str, "action=reload") == 
           strstr(req->body->str, "action=")) action = action_reload;
  else if (strstr(req->body->str, "action=new dir") ==
           strstr(req->body->str, "action=")) action = action_mkdir;
  else if (strstr(req->body->str, "action=new file") ==
           strstr(req->body->str, "action=")) action = action_mkfile;

  static char temp[256];
  switch (action)
  {
    case action_stop:
      httpd_stop = 1; // XXX : perhaps with a delay - permit , a few redirected
                      // requests to succeed - use a counter?
      break;

    case action_rmdir:
        snprintf (temp, sizeof(temp), "%s%s", post_path, post_param);
        rmdir(temp);
      /* FALLTHROUGH */
    case action_remove:
      if (action == action_remove)
      {
        snprintf (temp, sizeof(temp), "%s%s", post_path, post_param);
        unlink(temp);
      }
      /* FALLTHROUGH */
    case action_parent:
    case action_save:
    case action_run:
      req->status = 301;

      if ((action == action_save || action==action_run)
           && post_content && strlen(post_content)>1)
      {
      snprintf (temp, sizeof(temp), "Location: /_%s%s", post_path, post_param);       

      if (post_content){
      FILE *f = fopen (temp+12, "w");
      if(f)
      {
        int len = strlen (post_content);
        for (int i = 0; post_content[i]; i++)
        {
          if (post_content[i] == '\r')
          {
            len--;
            memmove (&post_content[i], &post_content[i+1], len-i);
            post_content[len]=0;
          }
        }
        fwrite (post_content, 1, strlen (post_content)-1, f);
        fclose (f);
      }
      }
      }

      if (action == action_run)
      {
        char *commandline = strdup (temp+12);
        //printf ("%i\n", system(commandline));
        printf ("%i\n", runs(commandline));
        free (commandline);
      }

      if (action == action_parent || action == action_remove || action == action_rmdir) 
      { // override reload of current page set-up before in temp
      snprintf (temp, sizeof(temp), "Location: /_%s", post_path?post_path:"/sd/");
      if (!(post_origname && post_origname[0]))
      {
         strrchr(temp, '/')[0] = 0;
         strrchr(temp, '/')[1] = 0;
      }
      }
      req->extra_headers = temp;
      break;
    case action_rename:
      req->status = 301;
      char buf[512];
      snprintf (buf, sizeof(buf), "%s%s", post_path, post_origname);
      snprintf (temp, sizeof(temp), "Location: /_%s%s", post_path, post_param);
      rename(buf, temp + 12);
      req->extra_headers = temp;
      break;
    case action_reload:
      req->status = 301;
      snprintf (temp, sizeof(temp), "Location: /_%s%s", post_path, post_param);
      req->extra_headers = temp;
      break;
    case action_mkfile:
      {
      req->status = 301;
      snprintf (temp, sizeof(temp), "Location: /_%s%s", post_path, post_param);
      char *content = "";
      FILE *f = fopen (temp+12, "w");
      if(f)
      {
        fwrite (content, 1, strlen (content), f);
        fclose (f);
      }
      req->extra_headers = temp;
      }
      break;

    case action_mkdir:
      {
        req->status = 301;
        snprintf (temp, sizeof(temp), "Location: /_%s%s/", post_path, post_param);
        mkdir (temp+12, 0777);//"w");
        req->extra_headers = temp;
      }
      break;

    default:
      req->mime_type = "text/plain";
      req->status_string= "OK";
      OUTF( "unhandled action %i param[%s] path[%s]", action, post_param, post_path);
      return;
  }
  ctx_string_set (req->body, "");

  return;
}

static void
httpd_request_handler (HttpdRequest *req)
{ 
  if (!strcasecmp (req->method, "POST"))
  {
    httpd_post_handler (req);
  }
  else if (req->path[0]=='/')
   {
      const char *path = req->path;

      for (int i = 0; filemappings[i].path; i++)
      {
        if (!strcmp (filemappings[i].path, path))
        {
          httpd_serve_file (req, filemappings[i].fs_path);
          return;
        }
      }

      if (req->path[1]=='_' && req->path[2]=='/')
      {
         httpd_browse_handler (req);
      }
      else
      {

      const char *mime_type  = ui_get_mime_type (NULL, path);
      if (!mime_type)
      {
        req->status = 404;
        req->status_string= "NOSPOON";
        OUTF( "404 noroon %i %s", rno++, path);
        return;
      }
      else if (!strcmp (mime_type, "inode/directory"))
      {
         httpd_dir_handler (req);
      }
      else
      {
         httpd_serve_file (req, req->path);
         return;
      }
      }
      req->status_string= "OK";
   }
  else
   {
      req->status = 414;
      req->status_string= "NOOON";
      OUTF( "404 no spoon %i %s", rno++, req->path);
   }
}



static void request_init (HttpdRequest *req, FILE *f)
{
  req->f              = f;
  req->extra_headers  = NULL;
  req->emitted        = 0;

  req->status         = 200;
  req->status_string  = "OK";
  req->mime_type      = "text/plain";
  req->content_length = -1;
  req->last_modified  = -1;
  req->body           = ctx_string_new ("");
}

static void request_emit (HttpdRequest *req)
{
  char *header = http_headers (req, req->content_length);
  if (req->body)
  {
    int i;
    req->content_length = req->body->length;
    fprintf (req->f, "%s", header);
    for (i = 0; i < req->body->length; i++)
      fprintf (req->f, "%c", req->body->str[i]);
    if (req->body)
      ctx_string_free (req->body, 1);
    req->body = NULL;
  }
  else
   {
     fprintf (req->f, "%s", header);
   }
  free (header);
  req->emitted = 1;
  printf("logemit:%i %s %s %i %s\n", req->status, req->method,
        req->status_string, req->content_length, req->path);
}

static void request_finish (HttpdRequest *req)
{
  if (!req->emitted)
    request_emit (req);
  if (req->body)
    ctx_string_free (req->body, 1);
  req->body = NULL;

#if 0
  /* log */
  if (req->content_length > 0)
  fprintf (stderr, "%s %i %s %i %s\n\r", req->path, req->status, req->status_string, req->content_length, req->protocol);
#endif
}

static void httpd_magic(void)
{
  Ui *ui = NULL;
  char png_magic[]={0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
  char jpg_magic1[]={0xff, 0xd8, 0xff, 0xdb, 0xff, 0xd8, 0xff, 0xe0};
  char jpg_magic2[]={0xff, 0xd8, 0xff, 0xe0};
  char jpg_magic3[]={0xff, 0xd8, 0xff, 0xee};
  char jpg_magic4[]={0xff, 0xd8, 0xff, 0xe1};

  if (!ui_has_magic (ui, "image/png"))
    ui_register_magic(ui, "image/png",  NULL, png_magic,  sizeof(png_magic),  0);
  if (!ui_has_magic (ui, "image/jped"))
  {
    ui_register_magic(ui, "image/jpeg", NULL, jpg_magic1, sizeof(jpg_magic1), 0);
    ui_register_magic(ui, "image/jpeg", NULL, jpg_magic2, sizeof(jpg_magic2), 0);
    ui_register_magic(ui, "image/jpeg", NULL, jpg_magic3, sizeof(jpg_magic3), 0);
    ui_register_magic(ui, "image/jpeg", NULL, jpg_magic4, sizeof(jpg_magic4), 0);
  }

  if (!ui_has_magic (ui, "text/markdown"))
    ui_register_magic(ui, "text/markdown", ".md", NULL, 0, 1);
  if (!ui_has_magic (ui, "text/html"))
    ui_register_magic(ui, "text/html", ".html", NULL, 0, 1);
  if (!ui_has_magic (ui, "text/css"))
    ui_register_magic(ui, "text/css", ".css", NULL, 0, 1);
  if (!ui_has_magic (ui, "application/javascript"))
    ui_register_magic(ui, "application/javascript", ".js", NULL, 0, 1);
}

int httpd_port = -1; 

int _httpd_start_int (int port, 
#if 0
      void (*httpd_request_handler)(HttpdRequest *req),
#else
      void *foo,
#endif
                                    void *user_data)
{
  int    sock;
  struct sockaddr_in sin;
  sock = socket (AF_INET, SOCK_STREAM, 0);
  int try_port = 80;
  for (;try_port < try_port+16; try_port++)
  {
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons (try_port);
    if (!bind (sock, (struct sockaddr *) &sin, sizeof (sin)) != 0)
    {
      httpd_port = try_port;
      break;
    }
    if (try_port == 80)
      try_port = 8080-1;
  }
  fcntl (sock, F_SETFL, fcntl(sock, F_GETFL,0)| O_NONBLOCK);

  if (httpd_port > 0 && listen (sock, 5) == 0)
    {
      printf("httpd port %i\n", httpd_port);
    }
  else
    {
      printf("httpd %s\n", httpd_port<=0?"failed to open socket":
                                         "failed to bind port");
      return -1;
    }
  httpd_stop = 0;
  while (!httpd_stop) /* should have some terminating condition.. */
    {
#if 0 //USE_FORK
      int pid;
#endif
      HttpdRequest req;
      char *tmp;
      int content_length = -1;
      int s;
      FILE *f;
      s = accept (sock, NULL, NULL);

      if (s < 0)
      {
        usleep(1000 * 10);
        //printf("!\n");
        continue;
      }

#if 0 //USE_FORK
      pid = fork ();
      if (pid < 0)
        {
          fprintf (stderr, "error on fork\n");
          continue;
        }
      if (pid == 0) /* this is the client process */
        {
#endif
          f = fdopen (s, "a+");
          if (!fgets (httpd_buf, sizeof (httpd_buf), f))
            continue;
          memset(&req,0,sizeof(req));
          req.method   = strtok_r (httpd_buf, " ", &tmp);
          req.path     = strtok_r (NULL, " ", &tmp);
          req.protocol = strtok_r (NULL, "\r", &tmp);
          req.user_data = user_data;
          if (!req.method || !req.path || !req.protocol)
            continue;

          while (fgets (httpd_buf2, sizeof (httpd_buf2), f))
          {
            if (!strncmp (httpd_buf2, "Content-Length",
                 strlen ("Content-Length")))
              {
                char *sep = strchr (httpd_buf2, ':');
                if (sep)
                {
                  sep++;
                  while (*sep && *sep == ' ')sep ++;
                  content_length = atoi (sep);
                }
              }

            if (!strncmp (httpd_buf2, "\r", strlen ("\r")))
              // end of headers
              break;
          }
          request_init (&req, f);
          if (content_length > 0)
            {
              int read;
              int read_total = 0;

              do {
                int read_attempt = sizeof(httpd_buf2);
                if (content_length - read_total < read_attempt)
                  read_attempt = content_length - read_total;
              read = fread (httpd_buf2, 1, read_attempt, f);
              ctx_string_append_data (req.body, httpd_buf2, read);
              read_total += read;
              //printf ("read: %i %i of %i\n", read, read_total, content_length);
              } while (read>0 && read_total < content_length);

              if ((int)read_total != content_length)
              {
                fprintf (stderr, "httpd, mismatched post length and detect post-length %i vs %i", (int)read_total, content_length);
                req.status = 500;
                req.status_string = "LENGTH-EEK";
                request_finish (&req);
                fclose (f);
                close (s);
                continue;
              }
            }
          fseek (f, 0, SEEK_CUR); 
          if (!strcasecmp (req.method, "GET") ||
              !strcasecmp (req.method, "POST"))
            httpd_request_handler (&req);
          else
            {
              req.status = 501;
              req.status_string = "Not supported";
              ctx_string_append_printf (req.body, "only POST or GET");
            }
          request_finish (&req);
          fclose (f);
#if 0//USE_FORK
          exit (0);
        }
      else
        {
          close (s);
        }
#else
      close (s);
#endif
    }
  close (sock);
  return 0;
}

#include <signal.h>

int main (int argc, char **argv)
{
  httpd_magic ();
  signal(SIGPIPE, SIG_IGN);
  _httpd_start_int (8080, httpd_request_handler, (void*)23);
  return 42; // keeping once of register valid
}
