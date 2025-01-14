#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#ifndef CTX_STRING_H
#define CTX_STRING_H

typedef struct _CtxString CtxString;
struct _CtxString
{
  char *str;
  int   length;
  int   utf8_length;
  int   allocated_length;
  int   is_line;
};

CtxString   *ctx_string_new_with_size  (const char *initial, int initial_size);
CtxString   *ctx_string_new            (const char *initial);
CtxString   *ctx_string_new_printf (const char *format, ...);
char       *ctx_string_dissolve       (CtxString *string);
void        ctx_string_free           (CtxString *string, int freealloc);
const char *ctx_string_get            (CtxString *string);
uint32_t    ctx_string_get_unichar    (CtxString *string, int pos);
int         ctx_string_get_length     (CtxString *string);
int         ctx_string_get_utf8length (CtxString *string);
void        ctx_string_set            (CtxString *string, const char *new_string);
void        ctx_string_clear          (CtxString *string);
void        ctx_string_append_str     (CtxString *string, const char *str);
void        ctx_string_append_byte    (CtxString *string, char  val);
void        ctx_string_append_string  (CtxString *string, CtxString *string2);
void        ctx_string_append_unichar (CtxString *string, unsigned int unichar);
void        ctx_string_append_data    (CtxString *string, const char *data, int len);

void        ctx_string_pre_alloc       (CtxString *string, int size);
void        ctx_string_append_utf8char (CtxString *string, const char *str);
void        ctx_string_append_printf  (CtxString *string, const char *format, ...);
void        ctx_string_replace_utf8   (CtxString *string, int pos, const char *new_glyph);
void        ctx_string_insert_utf8    (CtxString *string, int pos, const char *new_glyph);

void        ctx_string_insert_unichar (CtxString *string, int pos, uint32_t unichar);
void        ctx_string_replace_unichar (CtxString *string, int pos, uint32_t unichar);
void        ctx_string_remove         (CtxString *string, int pos);
char       *ctx_strdup_printf         (const char *format, ...);
void        ctx_string_append_int     (CtxString *string, int val);
void        ctx_string_append_float   (CtxString *string, float val);

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif
