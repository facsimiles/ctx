
#if CTX_XML

/*
 * Copyright (c) 2002, 2003, Øyvind Kolås <pippin@hodefoting.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#ifndef XMLTOK_H
#define XMLTOK_H

#include <stdio.h>

#define inbufsize 4096

typedef struct _Mrg Mrg;

typedef struct _MrgXml MrgXml;

enum
{
  t_none = 0,
  t_whitespace,
  t_prolog,
  t_dtd,
  t_comment,
  t_word,
  t_tag,
  t_closetag,
  t_closeemptytag,
  t_endtag,
  t_att = 10,
  t_val,
  t_eof,
  t_entity,
  t_error
};

MrgXml *xmltok_new (FILE * file_in);
MrgXml *xmltok_buf_new (char *membuf);
void    xmltok_free (MrgXml *t);
int     xmltok_lineno (MrgXml *t);
int     xmltok_get (MrgXml *t, char **data, int *pos);

#endif /*XMLTOK_H */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>


/* mrg - MicroRaptor Gui
 * Copyright (c) 2014 Øyvind Kolås <pippin@hodefoting.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MRG_STRING_H
#define MRG_STRING_H

typedef struct _MrgString MrgString;

struct _MrgString
{
  char *str;
  int   length;
  int   utf8_length;
  int   allocated_length;
}  __attribute((packed));

MrgString   *mrg_string_new_with_size  (const char *initial, int initial_size);
MrgString   *mrg_string_new            (const char *initial);
MrgString   *mrg_string_new_printf     (const char *format, ...);
void         mrg_string_free           (MrgString  *string, int freealloc);
char        *mrg_string_dissolve       (MrgString  *string);
const char  *mrg_string_get            (MrgString  *string);
int          mrg_string_get_length     (MrgString  *string);
int          mrg_string_get_utf8_length (MrgString  *string);
void         mrg_string_set            (MrgString  *string, const char *new_string);
void         mrg_string_clear          (MrgString  *string);
void         mrg_string_append_str     (MrgString  *string, const char *str);
void         mrg_string_append_byte    (MrgString  *string, char  val);
void         mrg_string_append_string  (MrgString  *string, MrgString *string2);
void         mrg_string_append_unichar (MrgString  *string, unsigned int unichar);
void         mrg_string_append_data    (MrgString  *string, const char *data, int len);
void         mrg_string_append_printf  (MrgString  *string, const char *format, ...);
void         mrg_string_replace_utf8   (MrgString *string, int pos, const char *new_glyph);
void         mrg_string_insert_utf8    (MrgString *string, int pos, const char *new_glyph);
void         mrg_string_remove_utf8    (MrgString *string, int pos);


/* mrg - MicroRaptor Gui
 * Copyright (c) 2014 Øyvind Kolås <pippin@hodefoting.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void mrg_string_init (MrgString *string, int initial_size)
{
  string->allocated_length = initial_size;
  string->length = 0;
  string->utf8_length = 0;
  string->str = malloc (string->allocated_length);
  string->str[0]='\0';
}

static void mrg_string_destroy (MrgString *string)
{
  if (string->str)
  {
    free (string->str);
    string->str = NULL;
  }
}

void mrg_string_clear (MrgString *string)
{
  string->length = 0;
  string->utf8_length = 0;
  string->str[string->length]=0;
}

static inline void _mrg_string_append_byte (MrgString *string, char  val)
{
  if ((val & 0xC0) != 0x80)
    string->utf8_length++;
  if (string->length + 1 >= string->allocated_length)
    {
      char *old = string->str;
      string->allocated_length *= 2;
      string->str = malloc (string->allocated_length);
      memcpy (string->str, old, string->allocated_length/2);
      free (old);
    }
  string->str[string->length++] = val;
  string->str[string->length] = '\0';
}
void mrg_string_append_byte (MrgString *string, char  val)
{
  _mrg_string_append_byte (string, val);
}

void mrg_string_append_unichar (MrgString *string, unsigned int unichar)
{
  char *str;
  char utf8[5];
  utf8[ctx_unichar_to_utf8 (unichar, (unsigned char*)utf8)]=0;
  str = utf8;

  while (str && *str)
    {
      _mrg_string_append_byte (string, *str);
      str++;
    }
}

static inline void _mrg_string_append_str (MrgString *string, const char *str)
{
  if (!str) return;
  while (*str)
    {
      _mrg_string_append_byte (string, *str);
      str++;
    }
}
void mrg_string_append_str (MrgString *string, const char *str)
{
  _mrg_string_append_str (string, str);
}

MrgString *mrg_string_new_with_size (const char *initial, int initial_size)
{
  MrgString *string = calloc (sizeof (MrgString), 1);
  mrg_string_init (string, initial_size);
  if (initial)
    _mrg_string_append_str (string, initial);
  return string;
}

MrgString *mrg_string_new (const char *initial)
{
  return mrg_string_new_with_size (initial, 8);
}

void mrg_string_append_data (MrgString *string, const char *str, int len)
{
  int i;
  for (i = 0; i<len; i++)
    _mrg_string_append_byte (string, str[i]);
}

void mrg_string_append_string (MrgString *string, MrgString *string2)
{
  const char *str = mrg_string_get (string2);
  while (str && *str)
    {
      _mrg_string_append_byte (string, *str);
      str++;
    }
}

const char *mrg_string_get (MrgString *string)
{
  return string->str;
}

int mrg_string_get_length (MrgString *string)
{
  return string->length;
}

/* dissolving a string, means destroying it, but returning
 * the string, that should be manually freed.
 */
char *mrg_string_dissolve   (MrgString *string)
{
  char *ret = string->str;
  string->str = NULL;
  free (string);
  return ret;
}

void
mrg_string_free (MrgString *string, int freealloc)
{
  if (freealloc)
    {
      mrg_string_destroy (string);
    }
  free (string);
}

void
mrg_string_append_printf (MrgString *string, const char *format, ...)
{
  va_list ap;
  size_t needed;
  char  *buffer;
  va_start(ap, format);
  needed = vsnprintf(NULL, 0, format, ap) + 1;
  buffer = malloc(needed);
  va_end (ap);
  va_start(ap, format);
  vsnprintf(buffer, needed, format, ap);
  va_end (ap);
  _mrg_string_append_str (string, buffer);
  free (buffer);
}

MrgString *mrg_string_new_printf (const char *format, ...)
{
  MrgString *string = mrg_string_new_with_size ("", 8);
  va_list ap;
  size_t needed;
  char  *buffer;
  va_start(ap, format);
  needed = vsnprintf(NULL, 0, format, ap) + 1;
  buffer = malloc(needed);
  va_end (ap);
  va_start(ap, format);
  vsnprintf(buffer, needed, format, ap);
  va_end (ap);
  _mrg_string_append_str (string, buffer);
  free (buffer);
  return string;
}

void
mrg_string_set (MrgString *string, const char *new_string)
{
  mrg_string_clear (string);
  _mrg_string_append_str (string, new_string);
}



#ifndef __MRG_LIST__
#define  __MRG_LIST__

#include <stdlib.h>

/* The whole mrg_list implementation is in the header and will be inlined
 * wherever it is used.
 */

typedef struct _MrgList MrgList;
  struct _MrgList {void *data;MrgList *next;
  void (*freefunc)(void *data, void *freefunc_data);
  void *freefunc_data;
}
;

static inline void mrg_list_prepend_full (MrgList **list, void *data,
    void (*freefunc)(void *data, void *freefunc_data),
    void *freefunc_data)
{
  MrgList *new_=calloc (sizeof (MrgList), 1);
  new_->next=*list;
  new_->data=data;
  new_->freefunc=freefunc;
  new_->freefunc_data = freefunc_data;
  *list = new_;
}

static inline int mrg_list_length (MrgList *list)
{
  int length = 0;
  MrgList *l;
  for (l = list; l; l = l->next, length++);
  return length;
}

static inline void mrg_list_prepend (MrgList **list, void *data)
{
  MrgList *new_=calloc (sizeof (MrgList), 1);
  new_->next= *list;
  new_->data=data;
  *list = new_;
}

static inline void *mrg_list_last (MrgList *list)
{
  if (list)
    {
      MrgList *last;
      for (last = list; last->next; last=last->next);
      return last->data;
    }
  return NULL;
}

static inline void mrg_list_append_full (MrgList **list, void *data,
    void (*freefunc)(void *data, void *freefunc_data),
    void *freefunc_data)
{
  MrgList *new_= calloc (sizeof (MrgList), 1);
  new_->data=data;
  new_->freefunc = freefunc;
  new_->freefunc_data = freefunc_data;
  if (*list)
    {
      MrgList *last;
      for (last = *list; last->next; last=last->next);
      last->next = new_;
      return;
    }
  *list = new_;
  return;
}

static inline void mrg_list_append (MrgList **list, void *data)
{
  mrg_list_append_full (list, data, NULL, NULL);
}

static inline void mrg_list_remove (MrgList **list, void *data)
{
  MrgList *iter, *prev = NULL;
  if ((*list)->data == data)
    {
      if ((*list)->freefunc)
        (*list)->freefunc ((*list)->data, (*list)->freefunc_data);
      prev = (void*)(*list)->next;
      free (*list);
      *list = prev;
      return;
    }
  for (iter = *list; iter; iter = iter->next)
    if (iter->data == data)
      {
        if (iter->freefunc)
          iter->freefunc (iter->data, iter->freefunc_data);
        prev->next = iter->next;
        free (iter);
        break;
      }
    else
      prev = iter;
}

static inline void mrg_list_free (MrgList **list)
{
  while (*list)
    mrg_list_remove (list, (*list)->data);
}

static inline MrgList *mrg_list_nth (MrgList *list, int no)
{
  while(no-- && list)
    list = list->next;
  return list;
}

static inline MrgList *mrg_list_find (MrgList *list, void *data)
{
  for (;list;list=list->next)
    if (list->data == data)
      break;
  return list;
}

static MrgList*
mrg_list_merge_sorted (MrgList* list1,
                       MrgList* list2,
    int(*compare)(const void *a, const void *b, void *userdata), void *userdata
)
{
  if (list1 == NULL)
     return(list2);
  else if (list2==NULL)
     return(list1);

  if (compare (list1->data, list2->data, userdata) >= 0)
  {
    list1->next = mrg_list_merge_sorted (list1->next,list2, compare, userdata);
    /*list1->next->prev = list1;
      list1->prev = NULL;*/
    return list1;
  }
  else
  {
    list2->next = mrg_list_merge_sorted (list1,list2->next, compare, userdata);
    /*list2->next->prev = list2;
      list2->prev = NULL;*/
    return list2;
  }
}


static void
mrg_list_split_half (MrgList*  head,
                     MrgList** list1,
                     MrgList** list2)
{
  MrgList* fast;
  MrgList* slow;
  if (head==NULL || head->next==NULL)
  {
    *list1 = head;
    *list2 = NULL;
  }
  else
  {
    slow = head;
    fast = head->next;

    while (fast != NULL)
    {
      fast = fast->next;
      if (fast != NULL)
      {
        slow = slow->next;
        fast = fast->next;
      }
    }

    *list1 = head;
    *list2 = slow->next;
    slow->next = NULL;
  }
}


void mrg_list_sort (MrgList **head,
    int(*compare)(const void *a, const void *b, void *userdata),
    void *userdata)
{
  MrgList* list1;
  MrgList* list2;

  /* Base case -- length 0 or 1 */
  if ((*head == NULL) || ((*head)->next == NULL))
  {
    return;
  }

  mrg_list_split_half (*head, &list1, &list2);
  mrg_list_sort (&list1, compare, userdata);
  mrg_list_sort (&list2, compare, userdata);
  *head = mrg_list_merge_sorted (list1, list2, compare, userdata);
}



static inline void
mrg_list_insert_before (MrgList **list, MrgList *sibling,
                        void *data)
{
  if (*list == NULL || *list == sibling)
    {
      mrg_list_prepend (list, data);
    }
  else
    {
      MrgList *prev = NULL;
      for (MrgList *l = *list; l; l=l->next)
        {
          if (l == sibling)
            break;
          prev = l;
        }
      if (prev) {
        MrgList *new_=calloc(sizeof (MrgList), 1);
        new_->next = sibling;
        new_->data = data;
        prev->next=new_;
      }
    }
}

static inline void
mrg_list_insert_sorted (MrgList **list, void *data,
                       int(*compare)(const void *a, const void *b, void *userdata),
                       void *userdata)
{
  mrg_list_prepend (list, data);
  mrg_list_sort (list, compare, userdata);
}

static inline void
mrg_list_reverse (MrgList **list)
{
  MrgList *new_ = NULL;
  MrgList *l;
  for (l = *list; l; l=l->next)
    mrg_list_prepend (&new_, l->data);
  mrg_list_free (list);
  *list = new_;
}

#endif

static MrgList *interns = NULL;

const char * mrg_intern_string (const char *str)
{
  MrgList *i;
  for (i = interns; i; i = i->next)
  {
    if (!strcmp (i->data, str))
      return i->data;
  }
  str = strdup (str);
  mrg_list_append (&interns, (void*)str);
  return str;
}

void mrg_string_replace_utf8 (MrgString *string, int pos, const char *new_glyph)
{
  int new_len = ctx_utf8_len (*new_glyph);
  int old_len = string->utf8_length;
  char tmpg[3]=" ";
  if (new_len <= 1 && new_glyph[0] < 32)
  {
    tmpg[0]=new_glyph[0]+64;
    new_glyph = tmpg;
  }

  if (pos == old_len)
  {
    _mrg_string_append_str (string, new_glyph);
    return;
  }

  {
    for (int i = old_len; i <= pos; i++)
    {
      _mrg_string_append_byte (string, ' ');
      old_len++;
    }
  }

  if (string->length + new_len  > string->allocated_length)
  {
    char *tmp;
    char *defer;
    string->allocated_length = string->length + new_len;
    tmp = calloc (string->allocated_length, 1);
    strcpy (tmp, string->str);
    defer = string->str;
    string->str = tmp;
    free (defer);
  }

  char *p = (void*)ctx_utf8_skip (string->str, pos);
  int prev_len = ctx_utf8_len (*p);
  char *rest;
  if (*p == 0 || *(p+prev_len) == 0)
  {
    rest = strdup("");
  }
  else
  {
    rest = strdup (p + prev_len);
  }

  memcpy (p, new_glyph, new_len);
  memcpy (p + new_len, rest, strlen (rest) + 1);
  string->length += new_len;
  string->length -= prev_len;
  free (rest);

  string->utf8_length = ctx_utf8_strlen (string->str);
}

void mrg_string_insert_utf8 (MrgString *string, int pos, const char *new_glyph)
{
  int new_len = ctx_utf8_len (*new_glyph);
  int old_len = string->utf8_length;
  char tmpg[3]=" ";
  if (new_len <= 1 && new_glyph[0] < 32)
  {
    tmpg[0]=new_glyph[0]+64;
    new_glyph = tmpg;
  }

  if (pos == old_len)
  {
    _mrg_string_append_str (string, new_glyph);
    return;
  }

  {
    for (int i = old_len; i <= pos; i++)
    {
      _mrg_string_append_byte (string, ' ');
      old_len++;
    }
  }

  if (string->length + new_len + 1  > string->allocated_length)
  {
    char *tmp;
    char *defer;
    string->allocated_length = string->length + new_len + 1;
    tmp = calloc (string->allocated_length, 1);
    strcpy (tmp, string->str);
    defer = string->str;
    string->str = tmp;
    free (defer);
  }

  char *p = (void*)ctx_utf8_skip (string->str, pos);
  int prev_len = ctx_utf8_len (*p);
  char *rest;
  if (*p == 0 || *(p+prev_len) == 0)
  {
    rest = strdup("");
  }
  else
  {
    rest = strdup (p);
  }

  memcpy (p, new_glyph, new_len);
  memcpy (p + new_len, rest, strlen (rest) + 1);
  string->length += new_len;
  free (rest);

  string->utf8_length = ctx_utf8_strlen (string->str);
}

int mrg_string_get_utf8_length (MrgString  *string)
{
  //return mrg_utf8_strlen (string->str);
  return string->utf8_length;
}

void mrg_string_remove_utf8 (MrgString *string, int pos)
{
  int old_len = string->utf8_length;

  {
    for (int i = old_len; i <= pos; i++)
    {
      _mrg_string_append_byte (string, ' ');
      old_len++;
    }
  }

  char *p = (void*)ctx_utf8_skip (string->str, pos);
  int prev_len = ctx_utf8_len (*p);
  char *rest;
  if (*p == 0 || *(p+prev_len) == 0)
  {
    rest = strdup("");
  }
  else
  {
    rest = strdup (p + prev_len);
  }

  memcpy (p, rest, strlen (rest) + 1);
  string->length -= prev_len;
  free (rest);

  string->utf8_length = ctx_utf8_strlen (string->str);
}


#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif

struct _MrgXml
{
  FILE     *file_in;
  int       state;
  MrgString *curdata;
  MrgString *curtag;
  int       c;
  int       c_held;

  unsigned char *inbuf;
  int       inbuflen;
  int       inbufpos;

  int       line_no;
};

enum
{
  s_null = 0,
  s_start,
  s_tag,
  s_tagnamestart,
  s_tagname,
  s_tagnamedone,
  s_intag,
  s_attstart,
  s_attname,
  s_attdone,
  s_att,
  s_atteq,
  s_eqquot,
  s_eqvalstart,
  s_eqapos,
  s_eqaposval,
  s_eqaposvaldone,
  s_eqval,
  s_eqvaldone,
  s_eqquotval,
  s_eqquotvaldone,
  s_tagend,
  s_empty,
  s_inempty,
  s_emptyend,
  s_whitespace,
  s_whitespacedone,
  s_entitystart,
  s_entity,
  s_entitydone,
  s_word,
  s_worddone,
  s_tagclose,
  s_tagclosenamestart,
  s_tagclosename,
  s_tagclosedone,
  s_tagexcl,
  s_commentdash1,
  s_commentdash2,
  s_incomment,
  s_commentenddash1,
  s_commentenddash2,
  s_commentdone,
  s_dtd,
  s_prolog,
  s_prologq,
  s_prologdone,
  s_eof,
  s_error
};

char     *c_ws = " \n\r\t";

enum
{
  c_nil = 0,
  c_eat = 1,                    /* request that another char be used for the next state */
  c_store = 2                   /* store the current char in the output buffer */
};

typedef struct
{
  int       state;
  char     *chars;
  unsigned char r_start;
  unsigned char r_end;
  int       next_state;
  int       resetbuf;
  int       charhandling;
  int       return_type;        /* if set return current buf, with type set to the type */
}
state_entry;

#define max_entries 20

static state_entry state_table[s_error][max_entries];

static void
a (int state,
   char *chars,
   unsigned char r_start,
   unsigned char r_end, int charhandling, int next_state)
{
  int       no = 0;

  while (state_table[state][no].state != s_null)
    no++;
  state_table[state][no].state = state;
  state_table[state][no].r_start = r_start;
  if (chars)
    state_table[state][no].chars = strdup (chars);
  state_table[state][no].r_end = r_end;
  state_table[state][no].charhandling = charhandling;
  state_table[state][no].next_state = next_state;
}

static void
r (int state, int return_type, int next_state)
{
  state_table[state][0].state = state;
  state_table[state][0].return_type = return_type;
  state_table[state][0].next_state = next_state;
}

/* *INDENT-OFF* */

static void
init_statetable (void) {
    static int inited=0;
    if(inited)
        return;
    inited=1;
    memset(state_table,0,sizeof(state_table));
    a(s_start,        "<",    0,0,            c_eat,            s_tag);
    a(s_start,        c_ws,    0,0,            c_eat+c_store,    s_whitespace);
    a(s_start,        "&",    0,0,            c_eat,            s_entitystart);
    a(s_start,        NULL,    0,255,            c_eat+c_store,    s_word);

    a(s_tag,        c_ws,    0,0,            c_eat,            s_tag);
    a(s_tag,        "/",    0,0,            c_eat,            s_tagclose);
    a(s_tag,        "!",    0,0,            c_eat,            s_tagexcl);
    a(s_tag,        "?",    0,0,            c_eat,            s_prolog);
    a(s_tag,        NULL,    0,255,            c_eat+c_store,    s_tagnamestart);

    a(s_tagclose,    NULL,    0,255,            c_eat+c_store,    s_tagclosenamestart);
    a(s_tagclosenamestart,    ">",    0,0,    c_eat,            s_tagclosedone);
    a(s_tagclosenamestart,    NULL,    0,255,    c_eat+c_store,    s_tagclosename);
    a(s_tagclosename,    ">",    0,0,        c_eat,            s_tagclosedone);
    a(s_tagclosename,    NULL,    0,255,        c_eat+c_store,    s_tagclosename);
    r(s_tagclosedone,    t_closetag,                            s_start);

    a(s_whitespace,        c_ws,    0,0,        c_eat+c_store,    s_whitespace);
    a(s_whitespace,        NULL,    0,255,        c_nil,            s_whitespacedone);
    r(s_whitespacedone,    t_whitespace,                        s_start);

    a(s_entitystart,";",    0,0,            c_eat,            s_entitydone);
    a(s_entitystart,NULL,    0,255,            c_eat+c_store,    s_entity);
    a(s_entity,        ";",    0,0,            c_eat,            s_entitydone);
    a(s_entity,NULL,        0,255,            c_eat+c_store,    s_entity);
    r(s_entitydone,    t_entity,                                s_start);

    a(s_word,        c_ws,    0,0,            c_nil,            s_worddone);
    a(s_word,        "<&",    0,0,            c_nil,            s_worddone);
    a(s_word,        NULL,    0,255,            c_eat+c_store,    s_word);
    r(s_worddone,    t_word,                                    s_start);

    a(s_tagnamestart,c_ws,    0,0,            c_nil,            s_tagnamedone);
    a(s_tagnamestart,    "/>",    0,0,        c_nil,            s_tagnamedone);
    a(s_tagnamestart,NULL,    0,255,            c_eat+c_store,    s_tagname);
    a(s_tagname,    c_ws,    0,0,            c_nil,            s_tagnamedone);
    a(s_tagname,    "/>",    0,0,            c_nil,            s_tagnamedone);
    a(s_tagname,    NULL,    0,255,            c_eat+c_store,    s_tagname);
    r(s_tagnamedone,    t_tag,                                s_intag);

    a(s_intag,        c_ws,    0,0,            c_eat,            s_intag);
    a(s_intag,        ">",    0,0,            c_eat,            s_tagend);
    a(s_intag,        "/",    0,0,            c_eat,            s_empty);
    a(s_intag,        NULL,    0,255,            c_eat+c_store,    s_attstart);

    a(s_attstart,    c_ws,    0,0,            c_eat,            s_attdone);
    a(s_attstart,    "=/>",    0,0,            c_nil,            s_attdone);
    a(s_attstart,    NULL,    0,255,            c_eat+c_store,    s_attname);
    a(s_attname,    "=/>",    0,0,            c_nil,            s_attdone);
    a(s_attname,    c_ws,    0,0,            c_eat,            s_attdone);
    a(s_attname,    NULL,    0,255,            c_eat+c_store,    s_attname);
    r(s_attdone,    t_att,                                    s_att);
    a(s_att,        c_ws,    0,0,            c_eat,            s_att);
    a(s_att,        "=",    0,0,            c_eat,            s_atteq);
    a(s_att,        NULL,    0,255,            c_eat,            s_intag);
    a(s_atteq,        "'",    0,0,            c_eat,            s_eqapos);
    a(s_atteq,        "\"",    0,0,            c_eat,            s_eqquot);
    a(s_atteq,        c_ws,    0,0,            c_eat,            s_atteq);
    a(s_atteq,        NULL,    0,255,            c_nil,            s_eqval);

    a(s_eqapos,        "'",    0,0,            c_eat,            s_eqaposvaldone);
    a(s_eqapos,        NULL,    0,255,            c_eat+c_store,    s_eqaposval);
    a(s_eqaposval,        "'",    0,0,        c_eat,            s_eqaposvaldone);
    a(s_eqaposval,        NULL,    0,255,        c_eat+c_store,    s_eqaposval);
    r(s_eqaposvaldone,    t_val,                                    s_intag);

    a(s_eqquot,        "\"",    0,0,            c_eat,            s_eqquotvaldone);
    a(s_eqquot,        NULL,    0,255,            c_eat+c_store,    s_eqquotval);
    a(s_eqquotval,        "\"",    0,0,        c_eat,            s_eqquotvaldone);
    a(s_eqquotval,        NULL,    0,255,        c_eat+c_store,    s_eqquotval);
    r(s_eqquotvaldone,    t_val,                                    s_intag);

    a(s_eqval,        c_ws,    0,0,            c_nil,            s_eqvaldone);
    a(s_eqval,        "/>",    0,0,            c_nil,            s_eqvaldone);
    a(s_eqval,        NULL,    0,255,            c_eat+c_store,    s_eqval);

    r(s_eqvaldone,    t_val,                                    s_intag);

    r(s_tagend,        t_endtag,                s_start);

    r(s_empty,              t_endtag,                               s_inempty);
    a(s_inempty,        ">",0,0,                c_eat,            s_emptyend);
    a(s_inempty,        NULL,0,255,                c_eat,            s_inempty);
    r(s_emptyend,    t_closeemptytag,                        s_start);

    a(s_prolog,        "?",0,0,                c_eat,            s_prologq);
    a(s_prolog,        NULL,0,255,                c_eat+c_store,    s_prolog);

    a(s_prologq,    ">",0,0,                c_eat,            s_prologdone);
    a(s_prologq,    NULL,0,255,                c_eat+c_store,    s_prolog);
    r(s_prologdone,    t_prolog,                s_start);

    a(s_tagexcl,    "-",0,0,                c_eat,            s_commentdash1);
    a(s_tagexcl,    "D",0,0,                c_nil,            s_dtd);
    a(s_tagexcl,    NULL,0,255,                c_eat,            s_start);

    a(s_commentdash1,    "-",0,0,                c_eat,            s_commentdash2);
    a(s_commentdash1,    NULL,0,255,                c_eat,            s_error);

    a(s_commentdash2,    "-",0,0,                c_eat,            s_commentenddash1);
    a(s_commentdash2,    NULL,0,255,                c_eat+c_store,    s_incomment);

    a(s_incomment   ,    "-",0,0,                c_eat,            s_commentenddash1);
    a(s_incomment   ,    NULL,0,255,                c_eat+c_store,    s_incomment);

    a(s_commentenddash1,    "-",0,0,            c_eat,            s_commentenddash2);
    a(s_commentenddash1,    NULL,0,255,            c_eat+c_store,    s_incomment);

    a(s_commentenddash2,    ">",0,0,            c_eat,            s_commentdone);
    a(s_commentenddash2,    NULL,0,255,            c_eat+c_store,    s_incomment);

    r(s_commentdone,    t_comment,                s_start);

}

/* *INDENT-ON* */

static int
is_oneof (char c, char *chars)
{
  while (*chars)
    {
      if (c == *chars)
        return 1;
      chars++;
    }
  return 0;
}

static int
nextchar (MrgXml *t)
{
  int       ret;

  if (t->file_in)
    {
      if (t->inbufpos >= t->inbuflen)
        {
          t->inbuflen = fread (t->inbuf, 1, inbufsize, t->file_in);
          t->inbufpos = 0;
          if (!t->inbuflen)
            return -1;
        }

      ret = (int) t->inbuf[t->inbufpos++];

      if (ret == '\n')
        t->line_no++;
    }
  else
    {
      if (t->inbufpos >= t->inbuflen)
        {
          return -1;
        }
      ret = (int) t->inbuf[t->inbufpos++];
      if (ret == '\n')
        t->line_no++;
    }
  return ret;
}

int
xmltok_get (MrgXml *t, char **data, int *pos)
{
  state_entry *s;

  init_statetable ();
  mrg_string_clear (t->curdata);
  while (1)
    {
      if (!t->c_held)
        {
          t->c = nextchar (t);
          if (t->c == -1)
          {
            if (pos)*pos = t->inbufpos;
            return t_eof;
          }
          t->c_held = 1;
        }
      if (t->state == s_dtd)
        {     /* FIXME: should make better code for skipping DTD */

          /*            int angle = 0; */
          int       squote = 0;
          int       dquote = 0;
          int       abracket = 1;

          /*            int sbracket = 0; */

          mrg_string_append_byte (t->curdata, t->c);

          while (abracket)
            {
              switch (t->c = nextchar (t))
                {
                case -1:
                  return t_eof;
                case '<':
                  if ((!squote) && (!dquote))
                    abracket++;
                  mrg_string_append_byte (t->curdata, t->c);
                  break;
                case '>':
                  if ((!squote) && (!dquote))
                    abracket--;
                  if (abracket)
                    mrg_string_append_byte (t->curdata, t->c);
                  break;
                case '"':
                case '\'':
                case '[':
                case ']':
                default:
                  mrg_string_append_byte (t->curdata, t->c);
                  break;
                }
            }
          t->c_held = 0;
          t->state = s_start;

          if (pos)*pos = t->inbufpos;
          return t_dtd;
        }
      s = &state_table[t->state][0];
      while (s->state)
        {
          if (s->return_type != t_none)
            {
              *data = (char *) mrg_string_get (t->curdata);
              t->state = s->next_state;
              if (s->return_type == t_tag)
                mrg_string_set (t->curtag, mrg_string_get (t->curdata));
              if (s->return_type == t_endtag)
                *data = (char *) mrg_string_get (t->curtag);
              if (s->return_type == t_closeemptytag)
                *data = (char *) mrg_string_get (t->curtag);
              if (pos)
                *pos = t->inbufpos;
              return s->return_type;
            }
          if ((s->chars && is_oneof (t->c, s->chars))
              || ((s->r_start + s->r_end)
                  && (t->c >= s->r_start && t->c <= s->r_end)))
            {
              if (s->charhandling & c_store)
                {
                  mrg_string_append_byte (t->curdata, t->c);
                }
              if (s->charhandling & c_eat)
                {
                  t->c_held = 0;
                }
              t->state = s->next_state;
              break;
            }
          s++;
        }
    }
  if (pos)
    *pos = t->inbufpos;
  return t_eof;
}

MrgXml *
xmltok_new (FILE * file_in)
{
  MrgXml *ret;

  ret = calloc (1, sizeof (MrgXml));
  ret->file_in = file_in;
  ret->state = s_start;
  ret->curtag = mrg_string_new ("");
  ret->curdata = mrg_string_new ("");
  ret->inbuf = calloc (1, inbufsize);
  return ret;
}

MrgXml *
xmltok_buf_new (char *membuf)
{
  MrgXml *ret;

  ret = calloc (1, sizeof (MrgXml));
  ret->file_in = NULL;
  ret->state = s_start;
  ret->curtag = mrg_string_new ("");
  ret->curdata = mrg_string_new ("");
  ret->inbuf = (void*)membuf;
  ret->inbuflen = strlen (membuf);
  ret->inbufpos = 0;
  return ret;
}

void
xmltok_free (MrgXml *t)
{
  mrg_string_free (t->curtag, 1);
  mrg_string_free (t->curdata, 1);

  if (t->file_in)
    {
      /*        fclose (t->file_in); */
      free (t->inbuf);
    }
  free (t);
}

char     *empty_tags[] = {
  "img", "IMG", "br", "BR", "hr", "HR", "META", "meta", "link", "LINK",
  NULL
};

char     *endomission_tags[] = {
  "li", "LI", "p", "P", "td", "TD", "tr", "TR", NULL
};

int
xmltok_lineno (MrgXml *t)
{
  return t->line_no;
}


/* mrg - MicroRaptor Gui
 * Copyright (c) 2014 Øyvind Kolås <pippin@hodefoting.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <math.h>

typedef enum _MrgType MrgType;
enum _MrgType {
  MRG_PRESS          = 1 << 0,
  MRG_MOTION         = 1 << 1,
  MRG_RELEASE        = 1 << 2,
  MRG_ENTER          = 1 << 3,
  MRG_LEAVE          = 1 << 4,
  MRG_TAP            = 1 << 5,
  MRG_TAP_AND_HOLD   = 1 << 6,

  /* NYI: SWIPE, ZOOM ROT_ZOOM, */

  MRG_DRAG_PRESS     = 1 << 7,
  MRG_DRAG_MOTION    = 1 << 8,
  MRG_DRAG_RELEASE   = 1 << 9,
  MRG_KEY_DOWN       = 1 << 10,
  MRG_KEY_UP         = 1 << 11,
  MRG_SCROLL         = 1 << 12,
  MRG_MESSAGE        = 1 << 13,
  MRG_DROP           = 1 << 14,

  /* client should store state - preparing
                                 * for restart
                                 */

  MRG_POINTER  = (MRG_PRESS | MRG_MOTION | MRG_RELEASE | MRG_DROP),
  MRG_TAPS     = (MRG_TAP | MRG_TAP_AND_HOLD),
  MRG_CROSSING = (MRG_ENTER | MRG_LEAVE),
  MRG_DRAG     = (MRG_DRAG_PRESS | MRG_DRAG_MOTION | MRG_DRAG_RELEASE),
  MRG_KEY      = (MRG_KEY_DOWN | MRG_KEY_UP),
  MRG_MISC     = (MRG_MESSAGE),
  MRG_ANY      = (MRG_POINTER | MRG_DRAG | MRG_CROSSING | MRG_KEY | MRG_MISC | MRG_TAPS),
};

#define MRG_CLICK   MRG_PRESS   // SHOULD HAVE MORE LOGIC


typedef struct _MrgHtml      MrgHtml; 
typedef struct _MrgHtmlState MrgHtmlState;
#define MRG_MAX_STYLE_DEPTH 640
#define MRG_MAX_STATE_DEPTH 128 //XXX: can these be different?
#define MRG_MAX_FLOATS      64
#define MRG_MAX_CBS         1024

/* other important maximums */
#define MRG_MAX_BINDINGS     1024
#define MRG_MAX_TEXT_LISTEN  1024

typedef struct _MrgRectangle MrgRectangle;
struct _MrgRectangle {
  int x;
  int y;
  int width;
  int height;
};

typedef enum _MrgModifierState MrgModifierState;

enum _MrgModifierState
{
  MRG_MODIFIER_STATE_SHIFT   = (1<<0),
  MRG_MODIFIER_STATE_CONTROL = (1<<1),
  MRG_MODIFIER_STATE_ALT     = (1<<2),
  MRG_MODIFIER_STATE_BUTTON1 = (1<<3),
  MRG_MODIFIER_STATE_BUTTON2 = (1<<4),
  MRG_MODIFIER_STATE_BUTTON3 = (1<<5)
};



typedef enum _MrgScrollDirection MrgScrollDirection;
enum _MrgScrollDirection
{
  MRG_SCROLL_DIRECTION_UP,
  MRG_SCROLL_DIRECTION_DOWN,
  MRG_SCROLL_DIRECTION_LEFT,
  MRG_SCROLL_DIRECTION_RIGHT
};

typedef struct _MrgEvent MrgEvent;
typedef struct _Mrg Mrg;

struct _MrgEvent {
  MrgType  type;
  Mrg     *mrg;
  uint32_t time;

  MrgModifierState state;

  int      device_no; /* 0 = left mouse button / virtual focus */
                      /* 1 = middle mouse button */
                      /* 2 = right mouse button */
                      /* 3 = first multi-touch .. (NYI) */

  float   device_x; /* untransformed (device) coordinates  */
  float   device_y;

  /* coordinates; and deltas for motion/drag events in user-coordinates: */
  float   x;
  float   y;
  float   start_x; /* start-coordinates (press) event for drag, */
  float   start_y; /*    untransformed coordinates */
  float   prev_x;  /* previous events coordinates */
  float   prev_y;
  float   delta_x; /* x - prev_x, redundant - but often useful */
  float   delta_y; /* y - prev_y, redundant - ..  */

  MrgScrollDirection scroll_direction;

  unsigned int unicode; /* only valid for key-events */

  const char *string;   /* as key can be "up" "down" "space" "backspace" "a" "b" "ø" etc .. */
                        /* this is also where the message is delivered for
                         * MESSAGE events
                         *
                         * and the data for drop events are delivered
                         */
  int stop_propagate; /* */
};

typedef void (*MrgCb) (MrgEvent *event,
                       void     *data,
                       void     *data2);

typedef struct MrgItemCb {
  MrgType types;
  MrgCb   cb;
  void*   data1;
  void*   data2;

  void (*finalize) (void *data1, void *data2, void *finalize_data);
  void  *finalize_data;

} MrgItemCb;

typedef struct MrgItem {
  CtxMatrix inv_matrix;  /* for event coordinate transforms */

  /* bounding box */
  float          x0;
  float          y0;
  float          x1;
  float          y1;

  //cairo_path_t   *path;
  double          path_hash;

  MrgType   types; /* all cb's ored together */
  MrgItemCb cb[MRG_MAX_CBS];
  int       cb_count;

  int       ref_count;
} MrgItem;

typedef struct _MrgStyleNode MrgStyleNode;
typedef struct _MrgHtmlState MrgHtmlState;

#define MRG_STYLE_MAX_CLASSES 16
#define MRG_STYLE_MAX_PSEUDO  16

struct _MrgStyleNode
{
  int         is_direct_parent; /* for use in selector chains with > */
  const char *id;
  const char *element;
  const char *classes[MRG_STYLE_MAX_CLASSES];
  const char *pseudo[MRG_STYLE_MAX_PSEUDO];
};


typedef enum {
  MRG_FLOAT_NONE = 0,
  MRG_FLOAT_LEFT,
  MRG_FLOAT_RIGHT,
  MRG_FLOAT_FIXED
} MrgFloat;


typedef struct MrgFloatData {
  MrgFloat  type;
  float     x;
  float     y;
  float     width;
  float     height;
} MrgFloatData;


struct _MrgHtmlState
{
  float        original_x;
  float        original_y;
  float        block_start_x;
  float        block_start_y;
  float        ptly;
  float        vmarg;
  MrgFloatData float_data[MRG_MAX_FLOATS];
  int          floats;
};

#define MRG_XML_MAX_ATTRIBUTES    32
#define MRG_XML_MAX_ATTRIBUTE_LEN 32
#define MRG_XML_MAX_VALUE_LEN     640
#define MRG_MAX_DEVICES 16
#define MRG_MAX_BINDINGS 1024
typedef void (*MrgDestroyNotify) (void *data);
typedef void (*MrgNewText)       (const char *new_text, void *data);
typedef void (*UiRenderFun)      (Mrg *mrg, void *ui_data);


typedef struct MrgBinding {
  char *nick;
  char *command;
  char *label;
  MrgCb cb;
  void *cb_data;
  MrgDestroyNotify destroy_notify;
  void  *destroy_data;
} MrgBinding;



struct _MrgHtml
{
  Mrg *mrg;
  int foo;
  MrgHtmlState  states[MRG_MAX_STYLE_DEPTH];
  MrgHtmlState *state;
  int state_no;
  MrgList *geo_cache;

  char         attribute[MRG_XML_MAX_ATTRIBUTES][MRG_XML_MAX_ATTRIBUTE_LEN];
  char  value[MRG_XML_MAX_ATTRIBUTES][MRG_XML_MAX_VALUE_LEN];

  int   attributes;
};


typedef struct _MrgGeoCache MrgGeoCache;
struct _MrgGeoCache
{
  void *id_ptr;
  float height;
  float width;
  int   hover;
  int gen;
};


static void
_mrg_draw_background_increment (Mrg *mrg, void *data, int last);

MrgGeoCache *_mrg_get_cache (MrgHtml *htmlctx, void *id_ptr)
{
  MrgList *l;
  for (l = htmlctx->geo_cache; l; l = l->next)
  {
    MrgGeoCache *item = l->data;
    if (item->id_ptr == id_ptr)
    {
      item->gen++;
      return item;
    }
  }
  {
    MrgGeoCache *item = calloc (sizeof (MrgGeoCache), 1);
    item->id_ptr = id_ptr;
    mrg_list_prepend_full (&htmlctx->geo_cache, item, (void*)free, NULL);
    return item;
  }
  return NULL;
}

void mrg_set_edge_right (Mrg *mrg, float val);
void mrg_set_edge_left (Mrg *mrg, float val);
void mrg_set_edge_top (Mrg *mrg, float val);
void mrg_set_edge_bottom (Mrg *mrg, float val);
float mrg_edge_right (Mrg *mrg);
float mrg_edge_left (Mrg *mrg);
float mrg_y (Mrg *mrg);
float mrg_x (Mrg *mrg);
float mrg_em (Mrg *mrg);
void mrg_set_xy (Mrg *mrg, float x, float y);


struct _MrgColor {
  float red;
  float green;
  float blue;
  float alpha;
};

typedef enum {
  MRG_DISPLAY_INLINE = 0,
  MRG_DISPLAY_BLOCK,
  MRG_DISPLAY_LIST_ITEM,
  MRG_DISPLAY_HIDDEN,
  MRG_DISPLAY_INLINE_BLOCK
} MrgDisplay;

/* matches cairo order */
typedef enum
{
  MRG_FONT_WEIGHT_NORMAL = 0,
  MRG_FONT_WEIGHT_BOLD
} MrgFontWeight;

typedef enum
{
  MRG_FILL_RULE_NONZERO = 0,
  MRG_FILL_RULE_EVEN_ODD
} MrgFillRule;

/* matches cairo order */
typedef enum
{
  MRG_FONT_STYLE_NORMAL = 0,
  MRG_FONT_STYLE_ITALIC,
  MRG_FONT_STYLE_OBLIQUE
} MrgFontStyle;

typedef enum
{
  MRG_BOX_SIZING_CONTENT_BOX = 0,
  MRG_BOX_SIZING_BORDER_BOX
} MrgBoxSizing;

/* matching nchanterm definitions */

typedef enum {
  MRG_REGULAR     = 0,
  MRG_BOLD        = (1 << 0),
  MRG_DIM         = (1 << 1),
  MRG_UNDERLINE   = (1 << 2),
  MRG_REVERSE     = (1 << 3),
  MRG_OVERLINE    = (1 << 4),
  MRG_LINETHROUGH = (1 << 5),
  MRG_BLINK       = (1 << 6)
} MrgTextDecoration;

typedef enum {
  MRG_POSITION_STATIC = 0,
  MRG_POSITION_RELATIVE,
  MRG_POSITION_FIXED,
  MRG_POSITION_ABSOLUTE
} MrgPosition;

typedef enum {
  MRG_OVERFLOW_VISIBLE = 0,
  MRG_OVERFLOW_HIDDEN,
  MRG_OVERFLOW_SCROLL,
  MRG_OVERFLOW_AUTO
} MrgOverflow;

typedef enum {
  MRG_CLEAR_NONE = 0,
  MRG_CLEAR_LEFT,
  MRG_CLEAR_RIGHT,
  MRG_CLEAR_BOTH
} MrgClear;

typedef enum {
  MRG_TEXT_ALIGN_LEFT = 0,
  MRG_TEXT_ALIGN_RIGHT,
  MRG_TEXT_ALIGN_JUSTIFY,
  MRG_TEXT_ALIGN_CENTER
} MrgTextAlign;

typedef enum {
  MRG_WHITE_SPACE_NORMAL = 0,
  MRG_WHITE_SPACE_NOWRAP,
  MRG_WHITE_SPACE_PRE,
  MRG_WHITE_SPACE_PRE_LINE,
  MRG_WHITE_SPACE_PRE_WRAP
} MrgWhiteSpace;

typedef enum {
  MRG_VERTICAL_ALIGN_BASELINE = 0,
  MRG_VERTICAL_ALIGN_MIDDLE,
  MRG_VERTICAL_ALIGN_BOTTOM,
  MRG_VERTICAL_ALIGN_TOP,
  MRG_VERTICAL_ALIGN_SUB,
  MRG_VERTICAL_ALIGN_SUPER
} MrgVerticalAlign;

typedef enum {
  MRG_CURSOR_AUTO = 0,
  MRG_CURSOR_ALIAS,
  MRG_CURSOR_ALL_SCROLL,
  MRG_CURSOR_CELL,
  MRG_CURSOR_CONTEXT_MENU,
  MRG_CURSOR_COL_RESIZE,
  MRG_CURSOR_COPY,
  MRG_CURSOR_CROSSHAIR,
  MRG_CURSOR_DEFAULT,
  MRG_CURSOR_E_RESIZE,
  MRG_CURSOR_EW_RESIZE,
  MRG_CURSOR_HELP,
  MRG_CURSOR_MOVE,
  MRG_CURSOR_N_RESIZE,
  MRG_CURSOR_NE_RESIZE,
  MRG_CURSOR_NESW_RESIZE,
  MRG_CURSOR_NS_RESIZE,
  MRG_CURSOR_NW_RESIZE,
  MRG_CURSOR_NO_DROP,
  MRG_CURSOR_NONE,
  MRG_CURSOR_NOT_ALLOWED,
  MRG_CURSOR_POINTER,
  MRG_CURSOR_PROGRESS,
  MRG_CURSOR_ROW_RESIZE,
  MRG_CURSOR_S_RESIZE,
  MRG_CURSOR_SE_RESIZE,
  MRG_CURSOR_SW_RESIZE,
  MRG_CURSOR_TEXT,
  MRG_CURSOR_VERTICAL_TEXT,
  MRG_CURSOR_W_RESIZE,
  MRG_CURSOR_WAIT,
  MRG_CURSOR_ZOOM_IN,
  MRG_CURSOR_ZOOM_OUT
} MrgCursor;

typedef enum {
  MRG_LINE_CAP_BUTT,
  MRG_LINE_CAP_ROUND,
  MRG_LINE_CAP_SQUARE
} MrgLineCap;

typedef enum {
  MRG_LINE_JOIN_MITER,
  MRG_LINE_JOIN_ROUND,
  MRG_LINE_JOIN_BEVEL
} MrgLineJoin;

typedef enum {
  MRG_UNICODE_BIDI_NORMAL = 0,
  MRG_UNICODE_BIDI_EMBED,
  MRG_UNICODE_BIDI_BIDI_OVERRIDE
} MrgUnicodeBidi;

typedef enum {
  MRG_DIRECTION_LTR = 0,
  MRG_DIRECTION_RTL
} MrgDirection;

typedef enum {
  MRG_VISIBILITY_VISIBLE = 0,
  MRG_VISIBILITY_HIDDEN
} MrgVisibility;

typedef enum {
  MRG_LIST_STYLE_OUTSIDE = 0,
  MRG_LIST_STYLE_INSIDE
} MrgListStyle;

/* This style class should be able to grow to contain some color names with
 * semantic meaning.
 */
struct _MrgStyle {
  /* text-related */
  float             font_size;
  char              font_family[128];
  CtxColor          color;
  float             text_indent;
  float             letter_spacing;
  float             word_spacing;

  MrgVisibility     visibility;

  MrgTextDecoration text_decoration;
  float             line_height;
  float             line_width;

  CtxColor          background_color;

  float             stroke_width;
  float             text_stroke_width;
  CtxColor          text_stroke_color;
  float             tab_size;

  MrgFillRule         fill_rule;
  MrgFontStyle        font_style;
  MrgFontWeight       font_weight;
  MrgLineCap          stroke_linecap;
  MrgLineJoin         stroke_linejoin;
  MrgTextAlign        text_align;
  MrgFloat            float_;
  MrgClear            clear;
  MrgOverflow         overflow;
  MrgDisplay          display;
  MrgPosition         position;
  MrgBoxSizing        box_sizing;
  MrgVerticalAlign    vertical_align;
  MrgWhiteSpace       white_space;
  MrgUnicodeBidi      unicode_bidi;
  MrgDirection        direction;
  MrgListStyle        list_style;
  unsigned char       stroke;
  unsigned char       fill;
  unsigned char       width_auto;
  unsigned char       margin_left_auto;
  unsigned char       margin_right_auto;
  unsigned char       print_symbols;
  CtxColor            stroke_color;

  /* vector shape / box related */
  CtxColor            fill_color;

  CtxColor          border_top_color;
  CtxColor          border_left_color;
  CtxColor          border_right_color;
  CtxColor          border_bottom_color;

  MrgCursor         cursor;

  /* layout related */

  float             top;
  float             left;
  float             right;
  float             bottom;
  float             width;
  float             height;
  float             min_height;
  float             max_height;
  float             min_width;
  float             max_width;

  float             border_top_width;
  float             border_left_width;
  float             border_right_width;
  float             border_bottom_width;

  float             padding_top;
  float             padding_left;
  float             padding_right;
  float             padding_bottom;

  float             margin_top;
  float             margin_left;
  float             margin_right;
  float             margin_bottom;

  void             *id_ptr;

  char              syntax_highlight[9];
};
typedef struct _MrgStyle MrgStyle;

typedef struct MrgState {
  float      (*wrap_edge_left)  (Mrg *mrg, void *data);
  float      (*wrap_edge_right) (Mrg *mrg, void *data);
  void        *wrap_edge_data;
  void       (*post_nl)  (Mrg *mrg, void *post_nl_data, int last);
  void        *post_nl_data;
  float        edge_top;
  float        edge_left;
  float        edge_right;
  float        edge_bottom;

  int          skip_lines;  /* better with an em offset? */
  int          max_lines;   /* better with max-y in ems? ? */

  //void        *id_ptr;

  char        *style_id;
  MrgStyleNode style_node;
  MrgStyle     style;

  int          overflowed;
  /* ansi/vt100 approximations of set text fg/bg color  */
  int          fg;
  int          bg;

  int          span_bg_started;
  int          children;
}  MrgState;;

struct _Mrg {
  float          rem;
  MrgHtml        html;

  Ctx           *ctx;
  int            width;
  int            height;
  float          ddpx;

  MrgList       *stylesheet;
  void          *css_parse_state;

  MrgString     *style;
  MrgString     *style_global;

  MrgList       *items;
  //MrgItem       *grab;

  int            frozen;
  int            fullscreen;

  //int          is_press_grabbed;

  int            quit;

  MrgList       *grabs; /* could split the grabs per device in the same way,
                           to make dispatch overhead smaller,. probably
                           not much to win though. */
  MrgItem       *prev[MRG_MAX_DEVICES];
  float          pointer_x[MRG_MAX_DEVICES];
  float          pointer_y[MRG_MAX_DEVICES];
  unsigned char  pointer_down[MRG_MAX_DEVICES];

  MrgBinding     bindings[MRG_MAX_BINDINGS];
  int            n_bindings;


  float          x; /* in px */
  float          y; /* in px */

  MrgRectangle   dirty;
  MrgRectangle   dirty_during_paint; // queued during painting

  MrgState      *state;
  MrgModifierState modifier_state;

  MrgList       *geo_cache;
  void          *eeek2;  /* something sometimes goes too deep! */
  void          *eeek1;  /* XXX something sometimes goes too deep in state */
  MrgState       states[MRG_MAX_STATE_DEPTH];
  int            state_no;
  int            in_paint;
  unsigned char *glyphs;  /* for terminal backend */
  unsigned char *styles;  /* ----------"--------- */
  void          *backend_data;
  int            do_clip;

  MrgEvent drag_event[MRG_MAX_DEVICES];

  int (*mrg_get_contents) (const char  *referer,
                           const char  *input_uri,
                           char       **contents,
                           long        *length,
                           void        *get_contents_data);
  void *get_contents_data;

  void (*ui_update)(Mrg *mrg, void *user_data);
  void *user_data;

  //MrgBackend *backend;

  char *title;

    /** text editing state follows **/
  int       text_edited;
  int       got_edit;
  MrgString *edited_str;

  int              text_edit_blocked;
  MrgNewText       update_string;
  void            *update_string_user_data;

  MrgDestroyNotify update_string_destroy_notify;
  void            *update_string_destroy_data;

  int       cursor_pos;
  float     e_x;
  float     e_y;
  float     e_ws;
  float     e_we;
  float     e_em;
  float     offset_x;
  float     offset_y;
  //cairo_scaled_font_t *scaled_font;

  MrgType      text_listen_types[MRG_MAX_TEXT_LISTEN];
  MrgCb        text_listen_cb[MRG_MAX_TEXT_LISTEN];
  void        *text_listen_data1[MRG_MAX_TEXT_LISTEN];
  void        *text_listen_data2[MRG_MAX_TEXT_LISTEN];

  void       (*text_listen_finalize[MRG_MAX_TEXT_LISTEN])(void *listen_data, void *listen_data2, void *finalize_data);
  void        *text_listen_finalize_data[MRG_MAX_TEXT_LISTEN];
  int          text_listen_count;
  int          text_listen_active;

    MrgList     *idles;
  int          idle_id;

  long         tap_delay_min;
  long         tap_delay_max;
  long         tap_delay_hold;

  double       tap_hysteresis;

  int          printing;
  //cairo_t     *printing_cr;
};


static float _mrg_dynamic_edge_right2 (Mrg *mrg, MrgHtmlState *state)
{
  float ret = mrg_edge_right (mrg);
  float y = mrg_y (mrg);
  float em = mrg_em (mrg);
  int i;

  if (state->floats)
    for (i = 0; i < state->floats; i++)
    {
      MrgFloatData *f = &state->float_data[i];
      if (f->type == MRG_FLOAT_RIGHT &&
          y >= f->y  &&
          y - em < f->y + f->height &&

          f->x < ret)
          ret = f->x;
    }
  return ret;
}

static float _mrg_dynamic_edge_left2 (Mrg *mrg, MrgHtmlState *state)
{
  float ret = mrg_edge_left (mrg);
  float y = mrg_y (mrg);
  float em = mrg_em (mrg);
  int i;

  if (state->floats)
    for (i = 0; i < state->floats; i++)
    {
      MrgFloatData *f = &state->float_data[i];
      if (f->type == MRG_FLOAT_LEFT &&
          y >= f->y &&
          y - em < f->y + f->height &&
          f->x + f->width > ret)
          ret = f->x + f->width;
    }
  return ret;
}

static float _mrg_parent_dynamic_edge_left (MrgHtml *ctx)
{
  MrgHtmlState *state = ctx->state;
  if (ctx->state_no)
    state = &ctx->states[ctx->state_no-1];
  return _mrg_dynamic_edge_left2 (ctx->mrg, state);
}

static float _mrg_parent_dynamic_edge_right (MrgHtml *ctx)
{
  MrgHtmlState *state = ctx->state;
  if (ctx->state_no)
    state = &ctx->states[ctx->state_no-1];
  return _mrg_dynamic_edge_right2 (ctx->mrg, state);
}

float _mrg_dynamic_edge_left (Mrg *mrg)
{
  if (mrg->state->wrap_edge_left)
    return mrg->state->wrap_edge_left (mrg, mrg->state->wrap_edge_data);
  return mrg->state->edge_left;
}

float _mrg_dynamic_edge_right (Mrg *mrg)
{
  if (mrg->state->wrap_edge_right)
    return mrg->state->wrap_edge_right (mrg, mrg->state->wrap_edge_data);
  return mrg->state->edge_right;
}

static float wrap_edge_left (Mrg *mrg, void *data)
{
  MrgHtml *ctx = data;
  MrgHtmlState *state = ctx->state;
  return _mrg_dynamic_edge_left2 (mrg, state);
}

static float wrap_edge_right (Mrg *mrg, void *data)
{
  MrgHtml *ctx = data;
  MrgHtmlState *state = ctx->state;
  return _mrg_dynamic_edge_right2 (mrg, state);
}

static void clear_left (MrgHtml *ctx)
{
  Mrg *mrg = ctx->mrg;
  float y = mrg_y (mrg);
  int i;

  if (ctx->state->floats)
  {
    for (i = 0; i < ctx->state->floats; i++)
      {
        MrgFloatData *f = &ctx->state->float_data[i];
        {
          if (f->type == MRG_FLOAT_LEFT)
          {
            if (f->y + f->height > y)
              y = f->y + f->height;
          }
        }
      }
  }
  mrg_set_xy (mrg, mrg_x (mrg), y);
}

static void clear_right (MrgHtml *ctx)
{
  Mrg *mrg = ctx->mrg;
  float y = mrg_y (mrg);
  int i;

  if (ctx->state->floats)
  {
    for (i = 0; i < ctx->state->floats; i++)
      {
        MrgFloatData *f = &ctx->state->float_data[i];
        {
          if (f->type == MRG_FLOAT_RIGHT)
          {
            if (f->y + f->height > y)
              y = f->y + f->height;
          }
        }
      }
  }
  mrg_set_xy (mrg, mrg_x (mrg), y);
}

static void clear_both (MrgHtml *ctx)
{
  Mrg *mrg = ctx->mrg;
#if 0
  clear_left (mrg);
  clear_right (mrg);
#else
  float y = mrg_y (mrg);
  int i;

  if (ctx->state->floats)
  {
    for (i = 0; i < ctx->state->floats; i++)
      {
        MrgFloatData *f = &ctx->state->float_data[i];
        {
          if (f->y + f->height > y)
            y = f->y + f->height;
        }
      }
  }
  y += mrg_em (mrg) * ctx_state_get (&mrg->ctx->state, CTX_line_height);
  mrg_set_xy (mrg, mrg_x (mrg), y);
  //_mrg_draw_background_increment (mrg, &mrg->html, 0);
#endif
}

static Ctx *mrg_cr (Mrg *mrg)
{
  return mrg->ctx;
}

#if 0
float mrg_em (Mrg *mrg)
{
  return ctx_get_font_size (mrg_cr (mrg));
}
#endif

float mrg_x (Mrg *mrg)
{
  return ctx_x (mrg_cr (mrg));
}

float mrg_y (Mrg *mrg)
{
  return ctx_y (mrg_cr (mrg));
}

float mrg_edge_bottom  (Mrg *mrg)
{
  return mrg->state->edge_bottom;
}

float mrg_edge_top  (Mrg *mrg)
{
  return mrg->state->edge_top;
}

float mrg_edge_left  (Mrg *mrg)
{
  return mrg->state->edge_left;
}

float mrg_edge_right (Mrg *mrg)
{
  return mrg->state->edge_right;
}

float _mrg_dynamic_edge_right (Mrg *mrg);
float _mrg_dynamic_edge_left (Mrg *mrg);

void  mrg_set_edge_top (Mrg *mrg, float val)
{
  mrg->state->edge_top = val;
  mrg_set_xy (mrg, _mrg_dynamic_edge_left (mrg) + ctx_get(mrg_cr(mrg), CTX_text_indent)
      , mrg->state->edge_top + mrg_em (mrg));
}

void  mrg_set_edge_left (Mrg *mrg, float val)
{
  mrg->state->edge_left = val;
}

void  mrg_set_edge_bottom (Mrg *mrg, float val)
{
  mrg->state->edge_bottom = val;
}

void  mrg_set_edge_right (Mrg *mrg, float val)
{
  mrg->state->edge_right = val;
}

void mrg_set_xy (Mrg *mrg, float x, float y)
{
  ctx_move_to (mrg_cr (mrg), x, y);
  mrg->state->overflowed = 0;
}

float mrg_rem (Mrg *mrg)
{
  return mrg->rem;
}

/**
 * mrg_style:
 * @mrg the mrg-context
 *
 * Returns the currently 
 *
 */
MrgStyle *mrg_style (Mrg *mrg)
{
  return &mrg->state->style;
}

void mrg_start     (Mrg *mrg, const char *class_name, void *id_ptr);
void mrg_start_with_style (Mrg        *mrg,
                           const char *style_id,
                           void       *id_ptr,
                           const char *style);
void mrg_start_with_stylef (Mrg *mrg, const char *style_id, void *id_ptr,
                       const char *format, ...);


static void mrg_parse_style_id (Mrg          *mrg,
                                const char   *style_id,
                                MrgStyleNode *node)
{
  const char *p;
  char temp[128] = "";
  int  temp_l = 0;
  if (!style_id)
  {
    return; // XXX: why does this happen?
  }

  memset (node, 0, sizeof (MrgStyleNode));

  for (p = style_id; ; p++)
  {
    switch (*p)
    {
      case '.':
      case ':':
      case '#':
      case 0:
        if (temp_l)
        {
          switch (temp[0])
          {
            case '.':
              {
                int i = 0;
                for (i = 0; node->classes[i]; i++);
                node->classes[i] = mrg_intern_string (&temp[1]);
              }
              break;
            case ':':
              {
                int i = 0;
                for (i = 0; node->pseudo[i]; i++);
                node->pseudo[i] = mrg_intern_string (&temp[1]);
              }
              break;
            case '#':
              node->id = mrg_intern_string (&temp[1]);
              break;
            default:
              node->element = mrg_intern_string (temp);
              break;
          }
          temp_l = 0;
        }
        if (*p == 0)
          return;
        temp[temp_l++] = *p;  // XXX: added to make reported fallthrough
        temp[temp_l]=0;       //      not be reported - butexplicit
        break;
      default:
        temp[temp_l++] = *p;
        temp[temp_l]=0;
    }
  }
}

void _mrg_init_style (Mrg *mrg)
{
  MrgStyle *s = mrg_style (mrg);

  s->text_decoration= 0;

  /* things not set here, are inherited from the parent style context,
   * more properly would be to rig up a fresh context, and copy inherited
   * values over, that would permit specifying inherit on any propery.
   */

  s->display = MRG_DISPLAY_INLINE;
  s->float_ = MRG_FLOAT_NONE;
  s->clear = MRG_CLEAR_NONE;
  s->overflow = MRG_OVERFLOW_VISIBLE;

  //s->stroke_width = 0.2;
#if 0
  s->stroke_color.red = 1;
  s->stroke_color.green = 0;
  s->stroke_color.blue = 1;
  s->stroke_color.alpha = 1;
  s->fill_color.red = 1;
  s->fill_color.green = 1;
  s->fill_color.blue = 0;
  s->fill_color.alpha = 1;
#endif

  ctx_color_set_rgba (&mrg->ctx->state, &s->background_color, 1, 1, 1, 0);
  /* this shouldn't be inherited? */
  //s->background_color.red = 1;
  //s->background_color.green = 1;
  //s->background_color.blue = 1;
  //s->background_color.alpha = 0;

  mrg->state->fg = 0;
  mrg->state->bg = 7;

  s->border_top_color.alpha = 0;
  s->border_left_color.alpha = 0;
  s->border_right_color.alpha = 0;
  s->border_bottom_color.alpha = 0;
  s->border_top_width = 0;
  s->border_left_width = 0;
  s->border_right_width = 0;
  s->border_bottom_width = 0;
  s->margin_top = 0;
  s->margin_left = 0;
  s->margin_right = 0;
  s->margin_bottom = 0;
  s->padding_top = 0;
  s->padding_left = 0;
  s->padding_right = 0;
  s->padding_bottom = 0;
  s->position = MRG_POSITION_STATIC;
  s->top = 0;
  s->left = 0;
  s->height = 0;
  s->width = 0;
  s->width_auto = 1;
}


/* mrg - MicroRaptor Gui
 * Copyright (c) 2014 Øyvind Kolås <pippin@hodefoting.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

const char * html_css =
"html, address,\n"
"blockquote,\n"
"body, dd, div,\n"
"dl, dt, fieldset, form,\n"
"frame, frameset,\n"
"h1, h2, h3, h4,\n"
"h5, h6, noframes,\n"
"ol, p, ul, center,\n"
"dir, hr, menu, pre { display: block; unicode-bidi: embed }\n"
"li { display: list-item }\n"
"head { display: none }\n"
"table { display: table }\n"
"tr { display: table-row }\n"
"thead { display: table-header-group }\n"
"tbody { display: table-row-group }\n"
"tfoot { display: table-footer-group }\n"
"col { display: table-column }\n"
"img { display: inline-block }\n"
"colgroup { display: table-column-group }\n"
"td, th          { display: table-cell }\n"
"caption         { display: table-caption }\n"
"th              { font-weight: bolder; text-align: center }\n"
"caption         { text-align: center }\n"
"body            { margin: 0.5em }\n"
"h1              { font-size: 2em; margin: .67em 0 }\n"
"h2              { font-size: 1.5em; margin: .75em 0 }\n"
"h3              { font-size: 1.17em; margin: .83em 0 }\n"
"h4, p,\n"
"blockquote, ul,\n"
"fieldset, form,\n"
"ol, dl, dir,\n"
"menu            { margin: 1.12em 0 }\n"
"h5              { font-size: .83em; margin: 1.5em 0 }\n"
"h6              { font-size: .75em; margin: 1.67em 0 }\n"
"h1, h2, h3, h4,\n"
"h5, h6, b,\n"
"strong          { font-weight: bolder }\n"
"blockquote      { margin-left: 4em; margin-right: 4em }\n"
"i, cite, em,\n"
"var, address    { font-style: italic }\n"
"pre, tt, code,\n"
"kbd, samp       { font-family: monospace }\n"
"pre             { white-space: pre }\n"
"button, textarea,\n"
"input, select   { display: inline-block }\n"
"big             { font-size: 1.17em }\n"
"small, sub, sup { font-size: .83em }\n"
"sub             { vertical-align: sub }\n"
"sup             { vertical-align: super }\n"
"table           { border-spacing: 2px; }\n"
"thead, tbody,\n"
"tfoot           { vertical-align: middle }\n"
"td, th, tr      { vertical-align: inherit }\n"
"s, strike, del  { text-decoration: line-through }\n"
"hr              { border: 1px inset black }\n"
"ol, ul, dir,\n"
"menu, dd        { padding-left: 2.5em }\n"
"ol              { list-style-type: decimal }\n"
"ol ul, ul ol,\n"
"ul ul, ol ol    { margin-top: 0; margin-bottom: 0 }\n"
"u, ins          { text-decoration: underline }\n"
//"br:before     { content: \"\\A\"; white-space: pre-line }\n"
"center          { text-align: center }\n"
":link, :visited { text-decoration: underline }\n"
":focus          { outline: thin dotted invert }\n"

".cursor { color: white; background: black; } \n"

"br       { display: block; }\n"
"document { color : black; font-weight: normal; background-color: white; }\n"
"body     { background-color: transparent; }\n"
"a        { color: blue; text-decoration: underline; }\n"
"a:hover  { background: black; color: white; text-decoration: underline; }\n"
"style, script { display: hidden; }\n"

"hr { margin-top:16px;font-size: 1px; }\n"  /* hack that works in one way, but shrinks top margin too much */
;

#define MRG_MAX_SELECTOR_LENGTH 32

typedef struct StyleEntry {
  char        *selector;
  MrgStyleNode parsed[MRG_MAX_SELECTOR_LENGTH];
  int          sel_len;
  char        *css;
  int          specificity;
} StyleEntry;

static void free_entry (StyleEntry *entry)
{
  free (entry->selector);
  free (entry->css);
  free (entry);
}

static int compute_specificity (const char *selector, int priority)
{
  const char *p;
  int n_id = 0;
  int n_class = 0;
  int n_tag = 0;
  int n_pseudo = 0;
  int in_word = 0;

  for (p = selector; *p; p++)
  {
    switch (*p)
    {
      case ' ':
        in_word = 0;
        break;
      case ':':
        in_word = 1;
        n_pseudo++;
        break;
      case '.':
        in_word = 1;
        n_class++;
        break;
      case '#':
        in_word = 1;
        n_id++;
        break;
      default:
        if (!in_word)
        {
          in_word = 1;
          n_tag++;
        }
    }
  }
  return priority * 100000 + n_pseudo * 10000 + n_id * 1000 + n_class * 100 + n_tag * 10;
}

static void mrg_parse_selector (Mrg *mrg, const char *selector, StyleEntry *entry)
{
  const char *p = selector;
  char section[256];
  int sec_l = 0;

  char type = ' ';

  for (p = selector; ; p++)
  {
    switch (*p)
    {
      case '.': case ':': case '#': case ' ': case 0:
        if (sec_l)
        {
          switch (type)
          {
            case ' ':
              entry->parsed[entry->sel_len].element = mrg_intern_string (section);
              break;
            case '#':
              entry->parsed[entry->sel_len].id = mrg_intern_string (section);
              break;
            case '.':
              {
                int i = 0;
                for (i = 0; entry->parsed[entry->sel_len].classes[i]; i++);
                entry->parsed[entry->sel_len].classes[i] = mrg_intern_string (section);
              }
              break;
            case ':':
              {
                int i = 0;
                for (i = 0; entry->parsed[entry->sel_len].pseudo[i]; i++);
                entry->parsed[entry->sel_len].pseudo[i] = mrg_intern_string (section);
              }
              break;
          }
        if (*p == ' ' || *p == 0)
        entry->sel_len ++;
        }
        if (*p == 0)
        {
          return;
        }
        section[(sec_l=0)] = 0;
        type = *p;
        break;
      default:
        section[sec_l++] = *p;
        section[sec_l] = 0;
        break;
    }
  }
}

static void mrg_stylesheet_add_selector (Mrg *mrg, const char *selector, const char *css, int priority)
{
  StyleEntry *entry = calloc (sizeof (StyleEntry), 1);
  entry->selector = strdup (selector);
  entry->css = strdup (css);
  entry->specificity = compute_specificity (selector, priority);
  mrg_parse_selector (mrg, selector, entry);
  mrg_list_prepend_full (&mrg->stylesheet, entry, (void*)free_entry, NULL);
}

#define MAXLEN 4096

#define MAKE_ERROR \
 if (error)\
 {\
   char errbuf[128];\
   sprintf (errbuf, "%i unexpected %c at %i'  %c%c%c", __LINE__, *p, (int)(p-css),\
     p[0], p[1], p[2]);\
   *error = strdup (errbuf);\
 }

#define MAX_RULES 64

typedef struct _MrgCssParseState MrgCssParseState;

struct _MrgCssParseState {
  int   state;
  char  rule[MAX_RULES][MAXLEN];
  int   rule_no ;
  int   rule_l[MAX_RULES];
  char  val[MAXLEN];
  int   val_l;
};

/* XXX: this funciton has no buffer bounds checking */
/* doesnt balance {} () [] and quotes */

enum
{
  NEUTRAL = 0,
  NEUTRAL_COMMENT,
  IN_RULE,
  RULE_COMMENT,
  IN_ARULE,
  ARULE_COMMENT,
  IN_IMPORT,
  IMPORT_COMMENT,
  EXPECT_COLON,
  COLON_COMMENT,
  EXPECT_VAL,
  EXPECT_VAL_COMMENT,
  IN_VAL,
  VAL_COMMENT,
};

void mrg_set_mrg_get_contents (Mrg *mrg,
  int (*mrg_get_contents) (const char  *referer,
                      const char  *input_uri,
                      char       **contents,
                      long        *length,
                      void        *get_contents_data),
  void *get_contents_data)

{
  mrg->mrg_get_contents = mrg_get_contents;
  mrg->get_contents_data = get_contents_data;
}


int
mrg_get_contents (Mrg         *mrg,
                  const char  *referer,
                  const char  *input_uri,
                  char       **contents,
                  long        *length)
{
  if (mrg->mrg_get_contents)
  {
    int ret;
    ret = mrg->mrg_get_contents (referer, input_uri, contents, length, mrg->get_contents_data);
    return ret;
  }
  else
  {
    *contents = NULL;
    *length = 0;
    return -1;
  }
}

static void _mrg_stylesheet_add (MrgCssParseState *ps, Mrg *mrg, const char *css, const char *uri_base,
                         int priority, char **error)
{
  const char *p;
  if (!ps)
    ps = mrg->css_parse_state = calloc (sizeof (MrgCssParseState), 1);

  if (!css)
    return;

  for (p = css; *p; p++)
  {
    switch (ps->state)
    {
      case NEUTRAL_COMMENT:
      case RULE_COMMENT:
      case ARULE_COMMENT:
      case IMPORT_COMMENT:
      case VAL_COMMENT:
      case EXPECT_VAL_COMMENT:
      case COLON_COMMENT:      if (p[0] == '*' && p[1] == '/') { p++; ps->state--; } break;
      case NEUTRAL:
        switch (*p)
        {
          case '/': if (p[1] == '*') { p++; ps->state = NEUTRAL_COMMENT; } break;
          case ' ':
          case '\n':
          case '\t':
          case ';':
            break;
          case '{':
          case '}':
            MAKE_ERROR;
            return;
          case '@':
            ps->state = IN_ARULE;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]++] = *p;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]] = 0;
            break;
          default:
            ps->state = IN_RULE;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]++] = *p;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]] = 0;
            break;
        }
        break;
      case IN_ARULE:
        switch (*p)
        {
          case '/': if (p[1] == '*') { p++; ps->state = ARULE_COMMENT; } break;
          case '{':
            ps->state = IN_VAL; // should be AVAL for media sections...
            break;
          case '\n':
          case '\t':
          case ' ':
            if (!strcmp (ps->rule[0], "import"))
            {
              MAKE_ERROR;
            }
            else
            {
              ps->state = IN_IMPORT;
            }
            break;
          case ';':
          case '}':
            MAKE_ERROR;
            return;
          default:
            ps->state = IN_ARULE;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]++] = *p;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]] = 0;
            break;
        }
        break;
      case IN_IMPORT:
        switch (*p)
        {
          int no;
          case '/': if (p[1] == '*') { p++; ps->state = IMPORT_COMMENT; } break;
          case ';':
            while (ps->val_l && (
                ps->val[ps->val_l-1] == ' ' ||
                ps->val[ps->val_l-1] == '\n' ||
                ps->val[ps->val_l-1] == '\t'))
              ps->val_l--;
            while (ps->val_l && ps->val[ps->val_l-1] != ')')
              ps->val_l--;
            if (ps->val[ps->val_l-1] == ')')
              ps->val_l--;
            if (ps->val[ps->val_l-1] == '"')
              ps->val_l--;
            ps->val[ps->val_l]=0;

            if(mrg->mrg_get_contents){
              char *contents;
              long length;
              char *uri = ps->val;

              /* really ugly string trimming to get to import uri.. */
              while (uri[0]==' ') uri++;
              if (!memcmp (uri, "url", 3)) uri+=3;
              if (uri[0]=='(') uri++;
              if (uri[0]=='"') uri++;
      
              /* XXX: should parse out the media part, and check if we should
               * really include this file
               */

              mrg_get_contents (mrg, uri_base, uri, &contents, &length);
              if (contents)
              {
                MrgCssParseState child_parser = {0,};
                _mrg_stylesheet_add (&child_parser, mrg, contents, uri, priority, error);
                free (contents);
              }
            }

            for (no = 0; no < ps->rule_no+1; no ++)
              ps->rule[no][ps->rule_l[no]=0] = 0;
            ps->val_l = 0;
            ps->val[0] = 0;
            ps->rule_no = 0;
            ps->state = NEUTRAL;
            break;
          default:
            ps->state = IN_IMPORT;
            ps->val[ps->val_l++] = *p;
            ps->val[ps->val_l] = 0;
            break;

        }
        break;
      case IN_RULE:
        switch (*p)
        {
          case '/': if (p[1] == '*') { p++; ps->state = RULE_COMMENT; } break;
          case '{':
            ps->state = IN_VAL;
            break;
          case '\n':
          case '\t':
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]++] = ' ';
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]] = 0;
            break;
          case ',':
            ps->state = NEUTRAL;
            ps->rule_no++;
            break;
          case ';':
          case '}':
            MAKE_ERROR;
            return;
          default:
            ps->state = IN_RULE;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]++] = *p;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]] = 0;
            break;
        }
        break;
      case EXPECT_COLON:
        switch (*p)
        {
          case '/': if (p[1] == '*') { p++; ps->state = COLON_COMMENT; } break;
          case ' ':
          case '\n':
          case '\t':
            break;
          case ':':
            ps->state = EXPECT_VAL;
            break;
          default:
            MAKE_ERROR;
            return;
        }
        break;
      case EXPECT_VAL:
        switch (*p)
        {
          case '/': if (p[1] == '*') { p++; ps->state = EXPECT_VAL_COMMENT; } break;
          case ' ':
          case '\n':
          case '\t':
          case ';':
            break;
          case '{':
            ps->state = IN_VAL;
            break;
          default:
            MAKE_ERROR;
            return;
        }
        break;
      case IN_VAL:
        switch (*p)
        {
          int no;

          /* XXX: parsing grammar is a bit more complicated, wrt quotes and
           * brackets..
           */

          case '/': if (p[1] == '*') { p++; ps->state = VAL_COMMENT; } break;
          case '}':
            while (ps->val_l && (
                ps->val[ps->val_l-1] == ' ' ||
                ps->val[ps->val_l-1] == '\n' ||
                ps->val[ps->val_l-1] == '\t'))
              ps->val_l--;
            ps->val[ps->val_l]=0;
            for (no = 0; no < ps->rule_no+1; no ++)
            {
              while (ps->rule_l[no] && (
                  ps->rule[no][ps->rule_l[no]-1] == ' ' ||
                  ps->rule[no][ps->rule_l[no]-1] == '\n' ||
                  ps->rule[no][ps->rule_l[no]-1] == '\t'))
                ps->rule_l[no]--;
              ps->rule[no][ps->rule_l[no]]=0;

              mrg_stylesheet_add_selector (mrg, ps->rule[no], ps->val, priority);
              ps->rule[no][ps->rule_l[no]=0] = 0;
            }

            ps->val_l = 0;
            ps->val[0] = 0;
            ps->rule_no = 0;
            ps->state = NEUTRAL;
            break;
          default:
            ps->state = IN_VAL;
            ps->val[ps->val_l++] = *p;
            ps->val[ps->val_l] = 0;
            break;

        }
        break;
    }
  }
}

void mrg_stylesheet_add (Mrg *mrg, const char *css, const char *uri_base,
                         int priority, char **error)
{
  MrgCssParseState *ps = mrg->css_parse_state;
  _mrg_stylesheet_add (ps, mrg, css, uri_base, priority, error);
}
#define MRG_STYLE_INTERNAL 10
#define MRG_STYLE_GLOBAL   15
#define MRG_STYLE_XML      20
#define MRG_STYLE_APP      20
#define MRG_STYLE_INLINE   25
#define MRG_STYLE_CODE     30

void mrg_css_default (Mrg *mrg)
{
  char *error = NULL;

  mrg_stylesheet_add (mrg, html_css, NULL, MRG_STYLE_INTERNAL, &error);
  if (error)
  {
    fprintf (stderr, "Mrg css parsing error: %s\n", error);
  }

  mrg_stylesheet_add (mrg,

" bold { font-weight:bold; } \n"
" dim *, dim { opacity:0.5; } \n"
" underline *, underline{ text-decoration:underline; }\n"
" reverse *,selected *, reverse,selected { text-decoration:reverse;}\n"
" unhandled       { color:cyan; }\n"
" binding:key     { background-color:white; color:black; }\n"
" binding:label   { color:cyan; }\n"
      
      ,NULL, MRG_STYLE_INTERNAL, &error);

  if (error)
  {
    fprintf (stderr, "mrg css parsing error: %s\n", error);
  }
}

void mrg_stylesheet_clear (Mrg *mrg)
{
  if (mrg->stylesheet)
    mrg_list_free (&mrg->stylesheet);
  mrg_css_default (mrg);
}

typedef struct StyleMatch
{
  StyleEntry *entry;
  int score;
} StyleMatch;

static int compare_matches (const void *a, const void *b, void *d)
{
  const StyleMatch *ma = a;
  const StyleMatch *mb = b;
  return ma->score - mb->score;
}

static inline int _mrg_nth_match (const char *selector, int child_no)
{
  const char *tmp = selector + 10;
  int a = 0;
  int b = 0;

  if (!strcmp (tmp, "odd)"))
  {
    a = 2; b = 1;
  }
  else if (!strcmp (tmp, "even)"))
  {
    a = 2; b = 0;
  }
  else
  {
    if (strchr (tmp, 'n'))
    {
      a = atoi (tmp);
      b = atoi (strchr (tmp, 'n')+1);
    }
    else
    {
      b = atoi (tmp);
    }
  }

  if (!a)
    return child_no == b;
  else
    if (child_no == b ||
       ((child_no - b > 0) == (a>0)))
      return !((child_no - b) % a);

  return 0;
}

int _mrg_child_no (Mrg *mrg)
{
  return mrg->states[mrg->state_no-1].children;
}

static inline int match_nodes (Mrg *mrg, MrgStyleNode *sel_node, MrgStyleNode *subject)
{
  int j, k;

  if (sel_node->element &&
      sel_node->element != subject->element)
    return 0;

  if (sel_node->id &&
      sel_node->id != subject->id)
    return 0;

  for (j = 0; sel_node->classes[j]; j++)
  {
    int found = 0;
    for (k = 0; subject->classes[k] && !found; k++)
    {
      if (sel_node->classes[j] == subject->classes[k])
        found = 1;
    }
    if (!found)
      return 0;
  }
  for (j = 0; sel_node->pseudo[j]; j++)
  {
    if (!strcmp (sel_node->pseudo[j], "first-child"))
    {
      if (!(_mrg_child_no (mrg) == 1))
        return 0;
    }
    else if (!strncmp (sel_node->pseudo[j], "nth-child(", 10))
    {
      if (!_mrg_nth_match (sel_node->pseudo[j], _mrg_child_no (mrg)))
        return 0;
    }
    else
    {
      int found = 0;

      for (k = 0; subject->pseudo[k] && !found; k++)
      {
        if (sel_node->pseudo[j] == subject->pseudo[k])
          found = 1;
      }
      if (!found)
        return 0;
    }
  }

  return 1;
}

static int mrg_selector_vs_ancestry (Mrg *mrg, StyleEntry *entry, MrgStyleNode **ancestry, int a_depth)
{
  int s = entry->sel_len - 1;

  /* right most part of selector must match */
  if (!match_nodes (mrg, &entry->parsed[s], ancestry[a_depth-1]))
    return 0;

  s--;
  a_depth--;

  if (s < 0)
    return 1;

  while (s >= 0)
  {
    int ai;
    int found_node = 0;

  /* XXX: deal with '>' */
    // if (entry->parsed[s].direct_ancestor) //
    for (ai = a_depth-1; ai >= 0 && !found_node; ai--)
    {
      if (match_nodes (mrg, &entry->parsed[s], ancestry[ai]))
        found_node = 1;
    }
    if (found_node)
    {
      a_depth = ai;
    }
    else
    {
      return 0;
    }
    s--;
  }

  return 1;
}

static int mrg_css_selector_match (Mrg *mrg, StyleEntry *entry, MrgStyleNode **ancestry, int a_depth)
{
  if (entry->selector[0] == '*' &&
      entry->selector[1] == 0)
    return entry->specificity;

  if (a_depth == 0)
    return 0;

  if (mrg_selector_vs_ancestry (mrg, entry, ancestry, a_depth))
    return entry->specificity;

  return 0;
}

static char *_mrg_css_compute_style (Mrg *mrg, MrgStyleNode **ancestry, int a_depth)
{
  MrgList *l;
  MrgList *matches = NULL;
  int totlen = 2;
  char *ret = NULL;

  for (l = mrg->stylesheet; l; l = l->next)
  {
    StyleEntry *entry = l->data;
    int score = 0;
    score = mrg_css_selector_match (mrg, entry, ancestry, a_depth);

    if (score)
    {
      StyleMatch *match = malloc (sizeof (StyleMatch));
      match->score = score;
      match->entry = entry;
      mrg_list_prepend_full (&matches, match, (void*)free, NULL);
      totlen += strlen (entry->css) + 1;
    }
  }

  if (matches)
  {
    MrgList *l;
    char *p;

    p = ret = malloc (totlen);

    mrg_list_sort (&matches, compare_matches, NULL);
    for (l = matches; l; l = l->next)
    {
      StyleMatch *match = l->data;
      StyleEntry *entry = match->entry;
      strcpy (p, entry->css);
      p += strlen (entry->css);
      strcpy (p, ";");
      p ++;
    }
    mrg_list_free (&matches);
  }
  return ret;
}

static int _mrg_get_ancestry (Mrg *mrg, MrgStyleNode **ancestry)
{
  int i, j;
  for (i = 0, j = 0; i <= mrg->state_no; i++)
    if (mrg->states[i].style_id)
    {
      ancestry[j++] = &(mrg->states[i].style_node);
    }
  ancestry[j] = NULL;
  return j;
}

char *_mrg_stylesheet_collate_style (Mrg *mrg)
{
  MrgStyleNode *ancestry[MRG_MAX_STYLE_DEPTH];
  int ancestors = _mrg_get_ancestry (mrg, ancestry);
  char *ret = _mrg_css_compute_style (mrg, ancestry, ancestors);
  return ret;
}

void  mrg_set_line_height (Mrg *mrg, float line_height)
{
  //if (mrg_is_terminal (mrg))
  //  line_height = 1.0;
  mrg_style (mrg)->line_height = line_height;
}

float mrg_line_height (Mrg *mrg)
{
  return mrg_style (mrg)->line_height;
}

void  mrg_set_rem         (Mrg *mrg, float em)
{
  mrg->rem = em;
}

float mrg_em (Mrg *mrg)
{
  return mrg->state->style.font_size;
}

void  mrg_set_em (Mrg *mrg, float em)
{
  //if (mrg_is_terminal (mrg)) /* XXX: not _really_ neccesary, and disables huge-title fonts,.."and (small-font vnc)" */
  //   em = CPX / mrg->ddpx;
  mrg->state->style.font_size = em;
}

void mrg_css_set (Mrg *mrg, const char *css)
{
  mrg_string_set (mrg->style, css);
}

void mrg_css_add (Mrg *mrg, const char *css)
{
  mrg_string_append_str (mrg->style, css);
}


void mrg_set_style (Mrg *mrg, const char *style)
{
}

void _mrg_layout_pre (Mrg *mrg, MrgHtml *ctx);
void _mrg_layout_post (Mrg *mrg, MrgHtml *ctx);


void mrg_start_with_style (Mrg        *mrg,
                           const char *style_id,
                           void       *id_ptr,
                           const char *style)
{
  mrg->states[mrg->state_no].children++;
  mrg->state_no++;
  mrg->state = &mrg->states[mrg->state_no];
  *mrg->state = mrg->states[mrg->state_no-1];
  mrg->states[mrg->state_no].children = 0;

  mrg->state->style_id = style_id ? strdup (style_id) : NULL;

  mrg_parse_style_id (mrg,
      mrg->state->style_id,
      &mrg->state->style_node);

  ctx_set (mrg_cr(mrg), CTX_display, MRG_DISPLAY_INLINE);
  mrg->state->style.id_ptr = id_ptr;

  _mrg_init_style (mrg);

  if (mrg->in_paint)
    ctx_save (mrg_cr (mrg));

  {
    char *collated_style = _mrg_stylesheet_collate_style (mrg);
    if (collated_style)
    {
      mrg_set_style (mrg, collated_style);
      free (collated_style);
    }
  }
  if (style)
  {
    mrg_set_style (mrg, style);
  }
  _mrg_layout_pre (mrg, &mrg->html);
}

void
mrg_start_with_stylef (Mrg *mrg, const char *style_id, void *id_ptr,
                       const char *format, ...)
{
  va_list ap;
  size_t needed;
  char  *buffer;
  va_start(ap, format);
  needed = vsnprintf(NULL, 0, format, ap) + 1;
  buffer = malloc(needed);
  va_end (ap);
  va_start(ap, format);
  vsnprintf(buffer, needed, format, ap);
  va_end (ap);
  mrg_start_with_style (mrg, style_id, id_ptr, buffer);
  free (buffer);
}

void mrg_start (Mrg *mrg, const char *style_id, void *id_ptr)
{
  mrg_start_with_style (mrg, style_id, id_ptr, NULL);
}

void mrg_end (Mrg *mrg)
{
  _mrg_layout_post (mrg, &mrg->html);
  if (mrg->state->style_id)
  {
    free (mrg->state->style_id);
    mrg->state->style_id = NULL;
  }
  mrg->state_no--;
  if (mrg->state_no < 0)
    fprintf (stderr, "unbalanced mrg_start/mrg_end, enderflow\n");
  mrg->state = &mrg->states[mrg->state_no];
  if (mrg->in_paint)
    ctx_restore (mrg_cr (mrg));
}


void mrg_end       (Mrg *mrg);


void mrg_stylesheet_clear (Mrg *mrg);
void mrg_stylesheet_add (Mrg *mrg, const char *css, const char *uri,
                         int priority,
                         char **error);

void mrg_css_set (Mrg *mrg, const char *css);
void mrg_css_add (Mrg *mrg, const char *css);


void mrg_set_style (Mrg *mrg, const char *style);
void mrg_set_stylef (Mrg *mrg, const char *format, ...);


void  mrg_set_line_height (Mrg *mrg, float line_height);
float  mrg_line_height (Mrg *mrg);



static void
mrg_ctx_set_source_color (Ctx *ctx, CtxColor *color)
{
   float rgba[4];
   ctx_color_get_rgba (&ctx->state, color, rgba);
   ctx_set_rgba (ctx, rgba[0], rgba[1], rgba[2], rgba[3]);
}

static void
ctx_fill_preserve (Ctx *ctx)
{
  // XXX
  ctx_fill (ctx);
}

static void mrg_path_fill_stroke (Mrg *mrg)
{
  Ctx *ctx = mrg_cr (mrg);
  MrgStyle *style = mrg_style (mrg);
  if (style->fill_color.alpha > 0.001)
  {
    mrg_ctx_set_source_color (ctx, &style->fill_color);
    ctx_fill_preserve (ctx);
  }

  if (style->stroke_width > 0.001)
  {
    ctx_set_line_width (ctx, style->stroke_width);
    mrg_ctx_set_source_color (ctx, &style->stroke_color);
    ctx_stroke (ctx);
  }
  ctx_new_path (ctx);
}

void _mrg_border_top (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_cr (mrg);
  MrgStyle *style = mrg_style (mrg);

  ctx_save (ctx);

  if (style->border_top_width &&
      style->border_top_color.alpha > 0.001)
  {
    ctx_new_path (ctx);
    ctx_move_to (ctx, x - style->padding_left - style->border_left_width,
                       y - style->padding_top - style->border_top_width);
    ctx_rel_line_to (ctx, width + style->padding_left + style->padding_right + style->border_left_width + style->border_right_width, 0);
    ctx_rel_line_to (ctx, -style->border_right_width, style->border_top_width);
    ctx_rel_line_to (ctx, - (width + style->padding_right + style->padding_left), 0);

    mrg_ctx_set_source_color (ctx, &style->border_top_color);
    ctx_fill (ctx);
  }
  ctx_restore (ctx);
}

void _mrg_border_bottom (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_cr (mrg);
  MrgStyle *style = mrg_style (mrg);

  ctx_save (ctx);

  if (style->border_bottom_width &&
      style->border_bottom_color.alpha > 0.001)
  {
    ctx_new_path (ctx);
    ctx_move_to (ctx, x + width + style->padding_right, y + height + style->padding_bottom);
    ctx_rel_line_to (ctx, style->border_right_width, style->border_bottom_width);
    ctx_rel_line_to (ctx, - (width + style->padding_left + style->padding_right + style->border_left_width + style->border_right_width), 0);
    ctx_rel_line_to (ctx, style->border_left_width, -style->border_bottom_width);

    mrg_ctx_set_source_color (ctx, &style->border_bottom_color);
    ctx_fill (ctx);
  }

  ctx_restore (ctx);
}

void _mrg_border_top_r (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *cr = mrg_cr (mrg);
  MrgStyle *style = mrg_style (mrg);

  ctx_save (cr);

  if (style->border_top_width &&
      style->border_top_color.alpha > 0.001)
  {
    ctx_new_path (cr);
    ctx_move_to (cr, x,
                       y - style->padding_top - style->border_top_width);
    ctx_rel_line_to (cr, width + style->padding_right + style->border_right_width, 0);
    ctx_rel_line_to (cr, -style->border_right_width, style->border_top_width);
    ctx_rel_line_to (cr, - (width + style->padding_right), 0);

    mrg_ctx_set_source_color (cr, &style->border_top_color);
    ctx_fill (cr);
  }
  ctx_restore (cr);
}
void _mrg_border_bottom_r (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_cr (mrg);
  MrgStyle *style = mrg_style (mrg);

  ctx_save (ctx);

  if (style->border_bottom_width &&
      style->border_bottom_color.alpha > 0.001)
  {
    ctx_new_path (ctx);
    ctx_move_to (ctx, x + width + style->padding_right, y + height + style->padding_bottom);
    ctx_rel_line_to (ctx, style->border_right_width, style->border_bottom_width);
    ctx_rel_line_to (ctx, - (width + style->padding_left + style->padding_right + style->border_left_width + style->border_right_width), 0);
    ctx_rel_line_to (ctx, style->border_left_width, -style->border_bottom_width);

    mrg_ctx_set_source_color (ctx, &style->border_bottom_color);
    ctx_fill (ctx);
  }

  ctx_restore (ctx);
}

void _mrg_border_top_l (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_cr (mrg);
  MrgStyle *style = mrg_style (mrg);

  ctx_save (ctx);

  if (style->border_top_width &&
      style->border_top_color.alpha > 0.001)
  {
    ctx_new_path (ctx);
    ctx_move_to (ctx, x - style->padding_left - style->border_left_width,
                       y - style->padding_top - style->border_top_width);
    ctx_rel_line_to (ctx, width + style->padding_left + style->padding_right + style->border_left_width, 0);
    ctx_rel_line_to (ctx, 0, style->border_top_width);
    ctx_rel_line_to (ctx, - (width + style->padding_left), 0);

    mrg_ctx_set_source_color (ctx, &style->border_top_color);
    ctx_fill (ctx);
  }
  ctx_restore (ctx);
}
void _mrg_border_bottom_l (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_cr (mrg);
  MrgStyle *style = mrg_style (mrg);

  ctx_save (ctx);

  if (style->border_bottom_width &&
      style->border_bottom_color.alpha > 0.001)
  {
    ctx_new_path (ctx);
    ctx_move_to (ctx, x + width, y + height + style->padding_bottom);
    ctx_rel_line_to (ctx, 0, style->border_bottom_width);
    ctx_rel_line_to (ctx, - (width + style->padding_left + style->border_left_width), 0);
    ctx_rel_line_to (ctx, style->border_left_width, -style->border_bottom_width);

    mrg_ctx_set_source_color (ctx, &style->border_bottom_color);
    ctx_fill (ctx);
  }

  ctx_restore (ctx);
}


void _mrg_border_top_m (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_cr (mrg);
  MrgStyle *style = mrg_style (mrg);

  ctx_save (ctx);

  if (style->border_top_width &&
      style->border_top_color.alpha > 0.001)
  {
    ctx_new_path (ctx);
    ctx_move_to (ctx, x,
                       y - style->padding_top - style->border_top_width);
    ctx_rel_line_to (ctx, width, 0);
    ctx_rel_line_to (ctx, 0, style->border_top_width);
    ctx_rel_line_to (ctx, -width, 0);

    mrg_ctx_set_source_color (ctx, &style->border_top_color);
    ctx_fill (ctx);
  }
  ctx_restore (ctx);
}
void _mrg_border_bottom_m (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_cr (mrg);
  MrgStyle *style = mrg_style (mrg);

  ctx_save (ctx);

  if (style->border_bottom_width &&
      style->border_bottom_color.alpha > 0.001)
  {
    ctx_new_path (ctx);
    ctx_move_to (ctx, x + width, y + height + style->padding_bottom);
    ctx_rel_line_to (ctx, 0, style->border_bottom_width);
    ctx_rel_line_to (ctx, - width, 0);
    ctx_rel_line_to (ctx, 0, -style->border_bottom_width);

    mrg_ctx_set_source_color (ctx, &style->border_bottom_color);
    ctx_fill (ctx);
  }

  ctx_restore (ctx);
}
void _mrg_border_left (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_cr (mrg);
  MrgStyle *style = mrg_style (mrg);

  ctx_save (ctx);

  if (style->border_left_width &&
      style->border_left_color.alpha > 0.001)
  {
    ctx_new_path (ctx);
    ctx_move_to (ctx, x - style->padding_left - style->border_left_width,
                       y - style->padding_top - style->border_top_width);
    ctx_rel_line_to (ctx, style->border_left_width, style->border_top_width);
    ctx_rel_line_to (ctx, 0, height + style->padding_top + style->padding_bottom );
    ctx_rel_line_to (ctx, -style->border_left_width, style->border_bottom_width);
    mrg_ctx_set_source_color (ctx, &style->border_left_color);
    ctx_fill (ctx);
  }

  ctx_restore (ctx);
}
void _mrg_border_right (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_cr (mrg);
  MrgStyle *style = mrg_style (mrg);

  ctx_save (ctx);

  if (style->border_right_width &&
      style->border_right_color.alpha > 0.001)
  {
    ctx_new_path (ctx);
    ctx_move_to (ctx, x + width + style->padding_right, y + height + style->padding_bottom);
    ctx_rel_line_to (ctx, style->border_right_width, style->border_bottom_width);
    ctx_rel_line_to (ctx, 0, - (height + style->padding_top + style->padding_bottom + style->border_top_width + style->border_bottom_width));
    ctx_rel_line_to (ctx, -style->border_right_width, style->border_top_width);

    mrg_ctx_set_source_color (ctx, &style->border_right_color);
    ctx_fill (ctx);
  }

  ctx_restore (ctx);
}

static void mrg_box (Mrg *mrg, int x, int y, int width, int height)
{
  _mrg_draw_background_increment (mrg, &mrg->html, 1);
  _mrg_border_top (mrg, x, y, width, height);
  _mrg_border_left (mrg, x, y, width, height);
  _mrg_border_right (mrg, x, y, width, height);
  _mrg_border_bottom (mrg, x, y, width, height);
}

static void mrg_box_fill (Mrg *mrg, MrgStyle *style, float x, float y, float width, float height)
{
  Ctx *ctx = mrg_cr (mrg);
  if (style->background_color.alpha <= 0.0001)
    return;

  height = floor (y + height) - floor(y);
  y = floor (y);

  ctx_save (ctx);
  {
    ctx_new_path (ctx);
    ctx_move_to (ctx, x,
                       y);
    ctx_rel_line_to (ctx, 0, height );
    ctx_rel_line_to (ctx, width, 0);
    ctx_rel_line_to (ctx, 0, -(height ));

    ctx_set_fill_rule (ctx, CTX_FILL_RULE_EVEN_ODD);
    mrg_ctx_set_source_color (ctx, &style->background_color);
    ctx_fill (ctx);
  }
  ctx_restore (ctx);
}

/*
 *  each style state level needs to know how far down it has
 *  painted background,.. on background increment we do all of them..
 *  .. floats are problematic - maybe draw them in second layer.
 *
 */

static void
_mrg_draw_background_increment2 (Mrg *mrg, MrgState *state, 
    MrgHtmlState *html_state, void *data, int last)
{
  MrgHtml *html = &mrg->html;
  Ctx *ctx = mrg_cr (mrg);
  MrgStyle *style; // XXX
  float gap = ctx_get_font_size (ctx) * ctx_get(ctx, CTX_line_height);

  int width = ctx_get(ctx, CTX_width);
#if ooops
  if (style->background_color.alpha <= 0.0001)
    return;
#endif
  if (ctx_get(ctx, CTX_display) == MRG_DISPLAY_INLINE &&
      ctx_get(ctx, CTX_float) == MRG_FLOAT_NONE)
    return;

  if (last)
    gap += ctx_get(ctx, CTX_padding_bottom);

  if (!width)
  {
    MrgGeoCache *geo = _mrg_get_cache (html, style->id_ptr);
    if (geo->width)
      width = geo->width;
    else
      width = mrg_edge_right (mrg) - mrg_edge_left (mrg); // XXX : err
  }

  if (html_state->ptly == 0)
  {
    mrg_box_fill (mrg, style,
      html_state->block_start_x - style->padding_left,
      (html_state->block_start_y - mrg_em (mrg) - style->padding_top),
      width + style->padding_left + style->padding_right,
      style->padding_top + gap);
    
    html_state->ptly = 
      html_state->block_start_y - mrg_em (mrg) - style->padding_top +
      (style->padding_top + gap);
  }
  else
  {
    if (( (mrg_y (mrg) - style->font_size) - html_state->ptly) + gap > 0)
    {
      mrg_box_fill (mrg, style,
          html_state->block_start_x - style->padding_left,
          html_state->ptly,
          width + style->padding_left + style->padding_right,
          ((mrg_y (mrg) - style->font_size) - html_state->ptly) + gap);

      html_state->ptly = mrg_y (mrg) - style->font_size  + gap;
    }
  }
}

static void
_mrg_draw_background_increment (Mrg *mrg, void *data, int last)
{
  MrgHtml *ctx = &mrg->html;
  int state;
  for (state = 0; state <= ctx->state_no; state++)
  {
    _mrg_draw_background_increment2 (mrg,
        &mrg->states[mrg->state_no - (ctx->state_no) + state],
        &ctx->states[state],
        data, last);
  }
}

float mrg_ddpx (Mrg *mrg)
{
  return 1;
}

float mrg_pointer_x (Mrg *mrg)
{
  return 0.0;
}
float mrg_pointer_y (Mrg *mrg)
{
  return 0.0;
}

void mrg_print (Mrg *mrg, const char *string)
{
  Ctx *ctx = mrg_cr (mrg);
  ctx_text (ctx, string);
}

void
mrg_printf (Mrg *mrg, const char *format, ...)
{
  va_list ap;
  size_t needed;
  char  *buffer;
  va_start(ap, format);
  needed = vsnprintf(NULL, 0, format, ap) + 1;
  buffer = malloc(needed);
  va_end (ap);
  va_start(ap, format);
  vsnprintf(buffer, needed, format, ap);
  va_end (ap);
  mrg_print (mrg, buffer);
  free (buffer);
}



void _mrg_layout_pre (Mrg *mrg, MrgHtml *html)
{
  MrgStyle *style;
  Ctx *ctx = mrg_cr (mrg);
  float dynamic_edge_left, dynamic_edge_right;

  html->state_no++;
  html->state = &html->states[html->state_no];
  *html->state = html->states[html->state_no-1];

  style = mrg_style (mrg);

  html->state->original_x = mrg_x (mrg);
  html->state->original_y = mrg_y (mrg);

  if (html->state_no)
  {
    dynamic_edge_right = _mrg_parent_dynamic_edge_right(html);
    dynamic_edge_left = _mrg_parent_dynamic_edge_left(html);
  }
  else
  {
    dynamic_edge_right = mrg_edge_right(mrg);
    dynamic_edge_left = mrg_edge_left(mrg);
  }

  if (style->clear & MRG_CLEAR_RIGHT)
    clear_right (html);
  if (style->clear & MRG_CLEAR_LEFT)
    clear_left (html);

#define CTX_padding_right       CTX_STRH('p','a','d','d','i','n','g','_','r','i','g','h')
#define CTX_margin_right        CTX_STRH('m','a','r','g','i','n','_','r','i','g','h','t')
#define CTX_border_right_width  CTX_STRH('b','o','r','d','e','r','_','r','i','g','h','t')
#define CTX_padding_left       CTX_STRH('p','a','d','d','i','n','g','_','l','e','f','t')
#define CTX_margin_left        CTX_STRH('m','a','r','g','i','n','_','l','e','f','t',0)
#define CTX_border_left_width  CTX_STRH('b','o','r','d','e','r','_','l','e','f','t','_')
#define CTX_padding_top       CTX_STRH('p','a','d','d','i','n','g','_','l','e','f','t')
#define CTX_margin_top        CTX_STRH('m','a','r','g','i','n','_','l','e','f','t',0)
#define CTX_border_top_width  CTX_STRH('b','o','r','d','e','r','_','l','e','f','t','_')

  if (style->display == MRG_DISPLAY_BLOCK ||
      style->display == MRG_DISPLAY_LIST_ITEM)
  {
    if (style->padding_left + style->margin_left + style->border_left_width
        != 0)
    {
      mrg_set_edge_left (mrg, mrg_edge_left (mrg) +
            ctx_get (ctx, CTX_padding_left) +
            ctx_get (ctx, CTX_margin_left) +
            ctx_get (ctx, CTX_border_left_width));
    }
    if (style->padding_right + style->margin_right + style->border_right_width
        != 0)
    {
      mrg_set_edge_right (mrg, mrg_edge_right (mrg) -
            (ctx_get (ctx, CTX_padding_right) +
             ctx_get (ctx, CTX_margin_right) +
             ctx_get (ctx, CTX_border_right_width)));
    }

    if (style->margin_top > html->state->vmarg)
    mrg_set_edge_top (mrg, mrg_y (mrg) + style->border_top_width + (style->margin_top - html->state->vmarg));
    else
    {
      /* XXX: just ignoring vmarg when top-margin is negative? */
      mrg_set_edge_top (mrg, mrg_y (mrg) + style->border_top_width + (style->margin_top));
    }

    html->state->block_start_x = mrg_edge_left (mrg);
    html->state->block_start_y = mrg_y (mrg);
  }

  if (style->display == MRG_DISPLAY_LIST_ITEM)
  {
    float x = mrg->x;
    _mrg_draw_background_increment (mrg, html, 0);
    mrg->x -= mrg_em (mrg) * 1;
    mrg_print (mrg, "•"); //⚫"); //●");
    mrg->x = x;
  }

  switch (style->position)
  {
    case MRG_POSITION_RELATIVE:
      /* XXX: deal with style->right and style->bottom */
      ctx_translate (mrg_cr (mrg), style->left, style->top);
      /* fallthrough */

    case MRG_POSITION_STATIC:


      if (style->float_ == MRG_FLOAT_RIGHT)
      {
        float width = style->width;

        if (width == 0.0)
        {
          MrgGeoCache *geo = _mrg_get_cache (html, style->id_ptr);
          if (geo->width)
            width = geo->width;
          else
            width = mrg_edge_right (mrg) - mrg_edge_left (mrg);
        }

        width = (width + style->padding_right + style->padding_left + style->border_left_width + style->border_right_width);


        if (width + style->margin_left + style->margin_right >
            mrg_edge_right(mrg)-mrg_edge_left(mrg))
        {
          clear_both (html);
          mrg_set_edge_left (mrg, mrg_edge_right (mrg) - width);
          mrg_set_edge_right (mrg, mrg_edge_right (mrg) - (style->margin_right + style->padding_right + style->border_right_width));

        }
        else
        {
        while (dynamic_edge_right - dynamic_edge_left < width + style->margin_left + style->margin_right)
        {
          mrg_set_xy (mrg, mrg_x (mrg), mrg_y (mrg) + 1.0);
          dynamic_edge_right = _mrg_parent_dynamic_edge_right(html);
          dynamic_edge_left = _mrg_parent_dynamic_edge_left(html);
        }

        mrg_set_edge_left (mrg, dynamic_edge_right - width);
        mrg_set_edge_right (mrg, dynamic_edge_right - (style->margin_right + style->padding_right + style->border_right_width));

        }

        mrg_set_edge_top (mrg, mrg_y (mrg) + (style->margin_top - html->state->vmarg) - mrg_em(mrg));

        html->state->block_start_x = mrg_x (mrg);
        html->state->block_start_y = mrg_y (mrg);
        html->state->floats = 0;

      } else if (style->float_ == MRG_FLOAT_LEFT)
      {
        float left, y;

        float width = style->width;

        if (width == 0.0)
        {
          MrgGeoCache *geo = _mrg_get_cache (html, style->id_ptr);
          if (geo->width)
            width = geo->width;
          else
            width = 4 * mrg_em (mrg);//mrg_edge_right (mrg) - mrg_edge_left (mrg);
        }

        width = (width + style->padding_right + style->padding_left + style->border_left_width + style->border_right_width);

        if (width + style->margin_left + style->margin_right >
            mrg_edge_right(mrg)-mrg_edge_left(mrg))
        {
          clear_both (html);
          left = mrg_edge_left (mrg) + style->padding_left + style->border_left_width + style->margin_left;
        }
        else
        {
        while (dynamic_edge_right - dynamic_edge_left < width + style->margin_left + style->margin_right)
        {
          mrg_set_xy (mrg, mrg_x (mrg), mrg_y (mrg) + 1.0);
          dynamic_edge_right = _mrg_parent_dynamic_edge_right(html);
          dynamic_edge_left = _mrg_parent_dynamic_edge_left(html);
        }
          left = dynamic_edge_left + style->padding_left + style->border_left_width + style->margin_left;
        }

        y = mrg_y (mrg);

        mrg_set_edge_left (mrg, left);
        mrg_set_edge_right (mrg,  left + width +
            style->padding_left + style->border_right_width);
        mrg_set_edge_top (mrg, mrg_y (mrg) + (style->margin_top - html->state->vmarg) - mrg_em(mrg));
        html->state->block_start_x = mrg_x (mrg);
        html->state->block_start_y = y - style->font_size + style->padding_top + style->border_top_width;
        html->state->floats = 0;

        /* change cursor point after floating something left; if pushed far
         * down, the new correct
         */
        mrg_set_xy (mrg, html->state->original_x = left + width + style->padding_left + style->border_right_width + style->padding_right + style->margin_right + style->margin_left + style->border_left_width,
            y - style->font_size + style->padding_top + style->border_top_width);
      } /* XXX: maybe spot for */
      break;
    case MRG_POSITION_ABSOLUTE:
      {
        html->state->floats = 0;
        mrg_set_edge_left (mrg, style->left + style->margin_left + style->border_left_width + style->padding_left);
        mrg_set_edge_right (mrg, style->left + style->width);
        mrg_set_edge_top (mrg, style->top + style->margin_top + style->border_top_width + style->padding_top);
        html->state->block_start_x = mrg_x (mrg);
        html->state->block_start_y = mrg_y (mrg);
      }
      break;
    case MRG_POSITION_FIXED:
      {
        int width = style->width;

        if (!width)
        {
          MrgGeoCache *geo = _mrg_get_cache (html, style->id_ptr);
          if (geo->width)
            width = geo->width;
          else
            width = mrg_edge_right (mrg) - mrg_edge_left (mrg);
        }

        ctx_identity_matrix (mrg_cr (mrg));
        ctx_scale (mrg_cr(mrg), mrg_ddpx (mrg), mrg_ddpx (mrg));
        html->state->floats = 0;

        mrg_set_edge_left (mrg, style->left + style->margin_left + style->border_left_width + style->padding_left);
        mrg_set_edge_right (mrg, style->left + style->margin_left + style->border_left_width + style->padding_left + width);//mrg_width (mrg) - style->padding_right - style->border_right_width - style->margin_right); //style->left + style->width); /* why only padding and not also border?  */
        mrg_set_edge_top (mrg, style->top + style->margin_top + style->border_top_width + style->padding_top);
        html->state->block_start_x = mrg_x (mrg);
        html->state->block_start_y = mrg_y (mrg);
      }
      break;
  }

  if (style->display == MRG_DISPLAY_BLOCK ||
      style->display == MRG_DISPLAY_INLINE_BLOCK ||
      style->float_)
  {
     float height = style->height;
     float width = style->width;

     if (!height)
       {
         MrgGeoCache *geo = _mrg_get_cache (html, style->id_ptr);
         if (geo->height)
           height = geo->height;
         else
           height = mrg_em (mrg) * 4;
       }
     if (!width)
       {
         MrgGeoCache *geo = _mrg_get_cache (html, style->id_ptr);
         if (geo->width)
           width = geo->width;
         else
           width = mrg_em (mrg) * 4;
       }

    if (height  /* XXX: if we knew height of dynamic elements
                        from previous frame, we could use it here */
       && style->overflow == MRG_OVERFLOW_HIDDEN)
       {
         ctx_rectangle (mrg_cr(mrg),
            html->state->block_start_x - style->padding_left - style->border_left_width,
            html->state->block_start_y - mrg_em(mrg) - style->padding_top - style->border_top_width,
            width + style->border_right_width + style->border_left_width + style->padding_left + style->padding_right, //mrg_edge_right (mrg) - mrg_edge_left (mrg) + style->padding_left + style->padding_right + style->border_left_width + style->border_right_width,
            height + style->padding_top + style->padding_bottom + style->border_top_width + style->border_bottom_width);
         ctx_clip (mrg_cr(mrg));
       }

    html->state->ptly = 0;
    _mrg_draw_background_increment (mrg, html, 0);
  }
}

#if 0
static void mrg_css_add_class (Mrg *mrg, const char *class_name)
{
  int i;
  MrgStyleNode *node = &mrg->state->style_node;
  for (i = 0; node->classes[i]; i++);
  node->classes[i] = mrg_intern_string (class_name);
}

static void mrg_css_add_pseudo_class (Mrg *mrg, const char *pseudo_class)
{
  int i;
  MrgStyleNode *node = &mrg->state->style_node;
  for (i = 0; node->pseudo[i]; i++);
  node->pseudo[i] = mrg_intern_string (pseudo_class);
}
#endif

void _mrg_set_wrap_edge_vfuncs (Mrg *mrg,
    float (*wrap_edge_left)  (Mrg *mrg, void *wrap_edge_data),
    float (*wrap_edge_right) (Mrg *mrg, void *wrap_edge_data),
    void *wrap_edge_data)
{
  mrg->state->wrap_edge_left = wrap_edge_left;
  mrg->state->wrap_edge_right = wrap_edge_right;
  mrg->state->wrap_edge_data = wrap_edge_data;
}

void _mrg_set_post_nl (Mrg *mrg,
    void (*post_nl)  (Mrg *mrg, void *post_nl_data, int last),
    void *post_nl_data
    )
{
  mrg->state->post_nl = post_nl;
  mrg->state->post_nl_data = post_nl_data;
}


void _mrg_get_ascent_descent (Mrg *mrg, float *ascent, float *descent)
{
#if 0 // MRG_CAIRO
  cairo_scaled_font_t *scaled_font = mrg->scaled_font;
  cairo_font_extents_t extents;
  if (mrg_is_terminal (mrg))
  {
    if (ascent) *ascent = 0;
    if (descent) *descent= 0;
    return;
  }

  if (mrg->in_paint)
  {
    cairo_set_font_size (mrg_cr(mrg), mrg_style(mrg)->font_size);
    scaled_font = cairo_get_scaled_font (mrg_cr (mrg));
  }
  cairo_scaled_font_extents (scaled_font, &extents);

  if (ascent)  *ascent  = extents.ascent;
  if (descent) *descent = extents.descent;
#else
  if (ascent)  *ascent  = mrg_style(mrg)->font_size;
  if (descent) *descent = 0.0;
#endif
}


static float _mrg_text_shift (Mrg *mrg)
{
  //MrgStyle *style = mrg_style (mrg);
  float ascent, descent;
  _mrg_get_ascent_descent (mrg, &ascent, &descent);
  return (descent * 0.9); // XXX
}


float paint_span_bg_final (Mrg   *mrg, float x, float y,
                           float  width)
{
  MrgStyle *style = mrg_style (mrg);
  Ctx *ctx = mrg_cr (mrg);
  if (style->display != MRG_DISPLAY_INLINE)
    return 0.0;

  if (style->background_color.alpha > 0.001)
  {
    ctx_save (ctx);
    ctx_rectangle (ctx, x,
                         y - mrg_em (mrg) * style->line_height +_mrg_text_shift (mrg)
                         ,
                         width + style->padding_right,
                         mrg_em (mrg) * style->line_height);
    mrg_ctx_set_source_color (ctx, &style->background_color);
    ctx_fill (ctx);
    ctx_restore (ctx);
  }

  _mrg_border_top_r (mrg, x, y - mrg_em (mrg) , width, mrg_em (mrg));
  _mrg_border_bottom_r (mrg, x, y - mrg_em (mrg), width, mrg_em (mrg));
  _mrg_border_right (mrg, x, y - mrg_em (mrg), width, mrg_em (mrg));

  return style->padding_right + style->border_right_width;
}



void _mrg_layout_post (Mrg *mrg, MrgHtml *ctx)
{
  float vmarg = 0;
  MrgStyle *style = mrg_style (mrg);
  
  /* adjust cursor back to before display */

  if ((style->display == MRG_DISPLAY_BLOCK || style->float_) &&
       style->height != 0.0)
  {
    float diff = style->height - (mrg_y (mrg) - (ctx->state->block_start_y - mrg_em(mrg)));
    mrg_set_xy (mrg, mrg_x (mrg), mrg_y (mrg) + diff);
    if (diff > 0)
      _mrg_draw_background_increment (mrg, ctx, 1);
  }

  /* remember data to store about float, XXX: perhaps better to store
   * straight into parent state?
   */
  if (style->float_)
  {
    int was_float = 0;
    float fx,fy,fw,fh; // these tempvars arent really needed.
    was_float = style->float_;
    fx = ctx->state->block_start_x - style->padding_left - style->border_left_width - style->margin_left;
    fy = ctx->state->block_start_y - mrg_em(mrg) - style->padding_top - style->border_top_width
      - style->margin_top;

    fw = mrg_edge_right (mrg) - mrg_edge_left (mrg)
     + style->padding_left + style->border_left_width + style->margin_left
     + style->padding_right + style->border_right_width + style->margin_right;

    fh = mrg_y (mrg) - (ctx->state->block_start_y - mrg_em(mrg))
         + style->margin_bottom + style->padding_top + style->padding_bottom + style->border_top_width + style->border_bottom_width + style->margin_top + mrg_em (mrg);

    ctx->states[ctx->state_no-1].float_data[ctx->states[ctx->state_no-1].floats].type = was_float;
    ctx->states[ctx->state_no-1].float_data[ctx->states[ctx->state_no-1].floats].x = fx;
    ctx->states[ctx->state_no-1].float_data[ctx->states[ctx->state_no-1].floats].y = fy;
    ctx->states[ctx->state_no-1].float_data[ctx->states[ctx->state_no-1].floats].width = fw;
    ctx->states[ctx->state_no-1].float_data[ctx->states[ctx->state_no-1].floats].height = fh;
    ctx->states[ctx->state_no-1].floats++;
  }

  if (style->display == MRG_DISPLAY_BLOCK || style->float_)
  {
    MrgGeoCache *geo = _mrg_get_cache (ctx, style->id_ptr);

    if (style->width == 0)
    {
#if 0
      if (mrg_y (mrg) == (ctx->state->block_start_y))
        geo->width = mrg_x (mrg) - (ctx->state->block_start_x);
      else
        geo->width = mrg->state->edge_right  - (ctx->state->block_start_x);
#endif
      geo->width = mrg_x (mrg) - (ctx->state->block_start_x);
    }
    else
      geo->width = style->width;

    //:mrg_edge_right (mrg) - mrg_edge_left (mrg);
    if (style->height == 0)
      geo->height = mrg_y (mrg) - (ctx->state->block_start_y - mrg_em(mrg));
    else
      geo->height = style->height;

    geo->gen++;

    mrg_box (mrg,
        ctx->state->block_start_x,
        ctx->state->block_start_y - mrg_em(mrg),
        geo->width,
        geo->height);

    {
      CtxMatrix transform;
      ctx_get_matrix (mrg_cr (mrg), &transform);
      float x = mrg_pointer_x (mrg);
      float y = mrg_pointer_y (mrg);
      ctx_matrix_invert (&transform);
      ctx_matrix_apply_transform (&transform, &x, &y);

      if (x >= ctx->state->block_start_x &&
          x <  ctx->state->block_start_x + geo->width &&
          y >= ctx->state->block_start_y - mrg_em (mrg) &&
          y <  ctx->state->block_start_y - mrg_em (mrg) + geo->height)
      {
        geo->hover = 1;
      }
      else
      {
        geo->hover = 0;
      }
    }

    //mrg_edge_right (mrg) - mrg_edge_left (mrg), mrg_y (mrg) - (ctx->state->block_start_y - mrg_em(mrg)));

    if (!style->float_ && style->display == MRG_DISPLAY_BLOCK)
    {
      vmarg = style->margin_bottom;

      mrg_set_xy (mrg, 
          mrg_edge_left (mrg),
          mrg_y (mrg) + vmarg + style->border_bottom_width);
    }
  }
  else if (style->display == MRG_DISPLAY_INLINE)
  {
    mrg->x += paint_span_bg_final (mrg, mrg->x, mrg->y, 0);
  }


  if (style->position == MRG_POSITION_RELATIVE)
    ctx_translate (mrg_cr (mrg), -style->left, -style->top);

  if (style->float_ ||
      style->position == MRG_POSITION_ABSOLUTE ||
      style->position == MRG_POSITION_FIXED)
  {
    mrg_set_xy (mrg, ctx->state->original_x,
                     ctx->state->original_y);
  }
  if (ctx->state_no)
    ctx->states[ctx->state_no-1].vmarg = vmarg;

  ctx->state_no--;
  ctx->state = &ctx->states[ctx->state_no];
}

enum {
  HTML_ATT_UNKNOWN = 0,
  HTML_ATT_STYLE,
  HTML_ATT_CLASS,
  HTML_ATT_ID,
  HTML_ATT_HREF,
  HTML_ATT_REL,
  HTML_ATT_SRC
};

static char *entities[][2]={
  {"shy",    ""},   // soft hyphen,. should be made use of in wrapping..
  {"nbsp",   " "},  //
  {"lt",     "<"},
  {"gt",     ">"},
  {"trade",  "™"},
  {"copy",   "©"},
  {"middot", "·"},
  {"bull",   "•"},
  {"Oslash", "Ø"},
  {"oslash", "ø"},
  {"hellip", "…"},
  {"aring",  "å"},
  {"Aring",  "Å"},
  {"aelig",  "æ"},
  {"AElig",  "Æ"},
  {"Aelig",  "Æ"},
  {"laquo",  "«"},
  {"raquo",  "»"},

  /*the above were added as encountered, the rest in anticipation  */

  {"reg",    "®"},
  {"deg",    "°"},
  {"plusmn", "±"},
  {"sup2",   "²"},
  {"sup3",   "³"},
  {"sup1",   "¹"},
  {"ordm",   "º"},
  {"para",   "¶"},
  {"cedil",  "¸"},
  {"bull",   "·"},
  {"amp",   "&"},
  {"mdash",  "–"},
  {"apos",   "'"},
  {"quot",   "\""},
  {"iexcl",  "¡"},
  {"cent",   "¢"},
  {"pound",  "£"},
  {"euro",   "€"},
  {"yen",    "¥"},
  {"curren", "¤"},
  {"sect",   "§"},
  {"phi",    "Φ"},
  {"omega",  "Ω"},
  {"alpha",  "α"},

  /* XXX: incomplete */

  {NULL, NULL}
};

static const char *get_attr (MrgHtml *ctx, const char *attr)
{
  int i;
  for (i = 0; i < ctx->attributes; i++)
  {
    if (!strcmp (ctx->attribute[i], attr))
    {
      return ctx->value[i];
    }
  }
  return NULL;
}

static void
mrg_parse_transform (Mrg *mrg, CtxMatrix *matrix, const char *str)
{
  if (!strncmp (str, "matrix", 5))
  {
    char *s;
    int numbers = 0;
    double number[12];
    ctx_matrix_identity (matrix);
    s = (void*) strchr (str, '(');
    if (!s)
      return;
    s++;
    for (; *s; s++)
    {
      switch (*s)
      {
        case '+':case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
        number[numbers] = strtod (s, &s);
        s--;
        numbers++;
      }
    }
    matrix->m[0][0] = number[0];
    matrix->m[1][0] = number[1];
    matrix->m[0][1] = number[2];
    matrix->m[1][1] = number[3];
    matrix->m[0][2] = number[4];
    matrix->m[1][2] = number[5];
  }
  else if (!strncmp (str, "scale", 5))
  {
    char *s;
    int numbers = 0;
    double number[12];
    ctx_matrix_identity (matrix);
    s = (void*) strchr (str, '(');
    if (!s)
      return;
    s++;
    for (; *s; s++)
    {
      switch (*s)
      {
        case '+':case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
        number[numbers] = strtod (s, &s);
        s--;
        numbers++;
      }
    }
    if (numbers == 1)
      ctx_matrix_scale (matrix, number[0], number[0]);
    else
      ctx_matrix_scale (matrix, number[0], number[1]);
  }
  else if (!strncmp (str, "translate", 5))
  {
    char *s;
    int numbers = 0;
    double number[12];
    ctx_matrix_identity (matrix);
    s = (void*) strchr (str, '(');
    if (!s)
      return;
    s++;
    for (; *s; s++)
    {
      switch (*s)
      {
        case '+':case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
        number[numbers] = strtod (s, &s);
        s--;
        numbers++;
      }
    }
    ctx_matrix_translate (matrix, number[0], number[1]);
  }
  else if (!strncmp (str, "rotate", 5))
  {
    char *s;
    int numbers = 0;
    double number[12];
    ctx_matrix_identity (matrix);
    s = (void*) strchr (str, '(');
    if (!s)
      return;
    s++;
    for (; *s; s++)
    {
      switch (*s)
      {
        case '+':case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
        number[numbers] = strtod (s, &s);
        s--;
        numbers++;
      }
    }
    ctx_matrix_rotate (matrix, number[0] / 360.0 * 2 * M_PI);
  }
  else
  {
    fprintf (stderr, "unhandled transform: %s\n", str);
    ctx_matrix_identity (matrix);
  }
}

int
mrg_parse_svg_path (Mrg *mrg, const char *str)
{
  Ctx *ctx = mrg_cr (mrg);
  char  command = 'm';
  char *s;
  int numbers = 0;
  double number[12];
  double pcx, pcy, cx, cy;

  if (!str)
    return -1;
  ctx_move_to (ctx, 0, 0);
  cx = 0; cy = 0;
  pcx = cx; pcy = cy;

  s = (void*)str;
again:
  numbers = 0;

  for (; *s; s++)
  {
    switch (*s)
    {
      case 'z':
      case 'Z':
        pcx = cx; pcy = cy;
        ctx_close_path (ctx);
        break;
      case 'm':
      case 'a':
      case 's':
      case 'S':
      case 'M':
      case 'c':
      case 'C':
      case 'l':
      case 'L':
      case 'h':
      case 'H':
      case 'v':
      case 'V':
         command = *s;
         break;

      case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
      if (*s == '-')
      {
        number[numbers] = -strtod (s+1, &s);
        s--;
      }
      else
      {
        number[numbers] = strtod (s, &s);
        s--;
      }
      numbers++;

      switch (command)
      {
        case 'a':
          /* fallthrough */
        case 'A':
          if (numbers == 7)
          {
            /// XXX: NYI
            s++;
            goto again;
          }
          /* fallthrough */
        case 'm':
          if (numbers == 2)
          {
            ctx_rel_move_to (ctx, number[0], number[1]);
	    cx += number[0];
	    cy += number[1];
            pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'h':
          if (numbers == 1)
          {
            ctx_rel_line_to (ctx, number[0], 0.0);
	    cx += number[0];
            pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'v':
          if (numbers == 1)
          {
            ctx_rel_line_to (ctx, 0.0, number[0]);
	    cy += number[0];
            pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'l':
          if (numbers == 2)
          {
            ctx_rel_line_to (ctx, number[0], number[1]);
	    cx += number[0];
	    cy += number[1];
            pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'c':
          if (numbers == 6)
          {
            ctx_rel_curve_to (ctx, number[0], number[1],
                                    number[2], number[3],
                                    number[4], number[5]);
            pcx = cx + number[2];
	    pcy = cy + number[3];
	    cx += number[4];
	    cy += number[5];
            s++;
            goto again;
          }
          break;
	case 's':
          if (numbers == 4)
          {
            ctx_curve_to (ctx, 2 * cx - pcx, 2 * cy - pcy,
                                number[0] + cx, number[1] + cy,
                                number[2] + cx, number[3] + cy);
	    pcx = number[0] + cx;
	    pcy = number[1] + cy;
	    cx += number[2];
	    cy += number[3];
            s++;
            goto again;
          }
          break;
        case 'M':
          if (numbers == 2)
          {
            ctx_move_to (ctx, number[0], number[1]);
	    cx = number[0];
	    cy = number[1];
	    pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'H':
          if (numbers == 1)
          {
            ctx_line_to (ctx, number[0], cy);
	    cx = number[0];
	    pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'V':
          if (numbers == 1)
          {
            ctx_line_to (ctx, cx, number[0]);
	    cy = number[0];
	    pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'L':
          if (numbers == 2)
          {
            ctx_line_to (ctx, number[0], number[1]);
	    cx = number[0];
	    cy = number[1];
	    pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'C':
          if (numbers == 6)
          {
            ctx_curve_to (ctx, number[0], number[1],
                                number[2], number[3],
                                number[4], number[5]);
	    pcx = number[2];
	    pcy = number[3];
	    cx = number[4];
	    cy = number[5];
            s++;
            goto again;
          }
          break;
        case 'S':
          if (numbers == 4)
          {
            float ax = 2 * cx - pcx;
            float ay = 2 * cy - pcy;
            ctx_curve_to (ctx, ax, ay,
                                number[0], number[1],
                                number[2], number[3]);
	    pcx = number[0];
	    pcy = number[1];
	    cx = number[2];
	    cy = number[3];
            s++;
            goto again;
          }
          break;
        default:
          fprintf (stderr, "_%c", *s);
          break;
      }
      break;
      default:
        break;
    }
  }
  return 0;
}

static float mrg_parse_float (Mrg *mrg, const char *a, char **b)
{
  return strtod (a, b);
}

static void
mrg_parse_polygon (Mrg *mrg, const char *str)
{
  Ctx *ctx = mrg_cr (mrg);
  char *s;
  int numbers = 0;
  int started = 0;
  double number[12];

  if (!str)
    return;
  ctx_move_to (ctx, 0, 0);

  s = (void*)str;
again:
  numbers = 0;

  for (; *s; s++)
  {
    switch (*s)
    {
      case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
      number[numbers] = mrg_parse_float (mrg, s, &s);
      s--;
      numbers++;

      if (numbers == 2)
      {
        if (started)
          ctx_line_to (ctx, number[0], number[1]);
        else
        {
          ctx_move_to (ctx, number[0], number[1]);
          started = 1;
        }
        s++;
        goto again;
      }
      default:
        break;
    }
  }
}

void _mrg_set_wrap_edge_vfuncs (Mrg *mrg,
    float (*wrap_edge_left)  (Mrg *mrg, void *wrap_edge_data),
    float (*wrap_edge_right) (Mrg *mrg, void *wrap_edge_data),
    void *wrap_edge_data);

void _mrg_set_post_nl (Mrg *mrg,
    void (*post_nl)  (Mrg *mrg, void *post_nl_data, int last),
    void *post_nl_data);


int mrg_get_contents (Mrg         *mrg,
                      const char  *referer,
                      const char  *input_uri,
                      char       **contents,
                      long        *length);


void  mrg_text_listen (Mrg *mrg, MrgType types,
                       MrgCb cb, void *data1, void *data2);

void  mrg_text_listen_full (Mrg *mrg, MrgType types,
                            MrgCb cb, void *data1, void *data2,
          void (*finalize)(void *listen_data, void *listen_data2, void *finalize_data),
          void  *finalize_data);
void  mrg_text_listen_done (Mrg *mrg);

void  mrg_text_listen_done (Mrg *mrg)
{
  mrg->text_listen_active = 0;
}

void  mrg_text_listen_full (Mrg *mrg, MrgType types,
                            MrgCb cb, void *data1, void *data2,
                      void   (*finalize)(void *listen_data, void *listen_data2, void *finalize_data),
                      void    *finalize_data)
{
  int no = mrg->text_listen_count;
  if (cb == NULL)
  {
    mrg_text_listen_done (mrg);
    return;
  }
  if (no + 1 >= MRG_MAX_TEXT_LISTEN)
  {
    fprintf (stderr, "mrg text listen overflow\n");
    return;
  }

  mrg->text_listen_types[no] = types;
  mrg->text_listen_cb[no] = cb;
  mrg->text_listen_data1[no] = data1;
  mrg->text_listen_data2[no] = data2;
  mrg->text_listen_finalize[no] = finalize;
  mrg->text_listen_finalize_data[no] = finalize_data;
  mrg->text_listen_count++;
  mrg->text_listen_active = 1;
}


char *_mrg_resolve_uri (const char *base_uri, const char *uri);
typedef struct _MrgImage MrgImage;
struct _MrgImage
{
  char *path;
  CtxBuffer *surface;
};


MrgImage *mrg_query_image (Mrg        *mrg,
                           const char *path,
                           int        *width,
                           int        *height)
{
  return NULL;
}

void mrg_image (Mrg *mrg, float x0, float y0, float width, float height, float opacity, const char *path, int *used_width, int *used_height)
{
}

enum
{
  URI_STATE_IN_PROTOCOL = 0,
  URI_STATE_IN_HOST,
  URI_STATE_IN_PORT,
  URI_STATE_E_S1,
  URI_STATE_E_S2,
  URI_STATE_IN_PATH,
  URI_STATE_IN_FRAGMENT,
};

int split_uri (char *uri,
               char **protocol,
               char **host,
               char **port,
               char **path,
               char **fragment)
{
  char *p;
  *protocol =
  *host =
  *port =
  *path =
  *fragment = NULL;

  if (strstr (uri, "//") || strchr(uri, ':'))
  {
    int mr = URI_STATE_IN_PROTOCOL;

    if (protocol)
      *protocol = uri;

    if (uri[0] == '/' &&
        uri[1] == '/')
    {
      mr = URI_STATE_E_S1;
      *protocol = NULL;
    }

    for (p = uri; *p; p++)
    {
      switch (mr)
     {
        case URI_STATE_IN_PROTOCOL:
          switch (*p)
          {
            default:
              break;
            case ':':
              *p = '\0';
              mr = URI_STATE_E_S1;
              break;
          }
          break;
        case URI_STATE_E_S1:
          switch (*p)
          {
            default:
              mr = URI_STATE_IN_HOST;
              if (path) *path = p;
              break;
            case '/':
              mr = URI_STATE_E_S2;
              break;
          }
          break;
        case URI_STATE_E_S2:
          switch (*p)
          {
            default:
              // XXX ?
              break;
            case '/':
              mr = URI_STATE_IN_HOST;
              if (host) *host = p+1;
              break;
          }
          break;
        case URI_STATE_IN_HOST:
          switch (*p)
          {
            default:
              break;
            case ':':
              *p = '\0';
              mr = URI_STATE_IN_PORT;
              if (port) *port = p+1;
              break;
            case '/':
              *p = '\0';
              mr = URI_STATE_IN_PATH;
              if (path) *path = p+1;
              break;
          }
         break;
        case URI_STATE_IN_PORT:
          switch (*p)
          {
            default:
              break;
            case '/':
              *p = '\0';
              mr = URI_STATE_IN_PATH;
              if (path) *path = p+1;
              break;
          }
          break;
        case URI_STATE_IN_PATH:
          switch (*p)
          {
            default:
              break;
            case '#':
              *p = '\0';
              mr = URI_STATE_IN_FRAGMENT;
              if (fragment) *fragment = p+1;
              break;
          }
          break;
      }
    }
  }
  else
  {

    int mr = URI_STATE_IN_HOST;
    if (protocol)
      *protocol = NULL;

    if (uri[0]=='/')
    {
      if (host)
        *host = NULL;
      if (port)
        *port = NULL;
      *uri = '\0';
      mr = URI_STATE_IN_PATH;
      if (path) *path = uri+1;
    }
    else
    {
      mr = URI_STATE_IN_PATH;
      if (path) *path = uri;
    }

    for (p = uri; *p; p++)
    {
      switch (mr)
      {
        case URI_STATE_IN_PROTOCOL:
          switch (*p)
          {
            default:
              break;
            case ':':
              *p = '\0';
              mr = URI_STATE_E_S1;
              break;
          }
          break;
        case URI_STATE_E_S1:
          switch (*p)
          {
            default:
              // XXX ?
              break;
            case '/':
              mr = URI_STATE_E_S2;
              break;
          }
          break;
        case URI_STATE_E_S2:
         switch (*p)
          {
            default:
              // XXX ?
              break;
            case '/':
              mr = URI_STATE_IN_HOST;
              if (host) *host = p+1;
              break;
          }
          break;
        case URI_STATE_IN_HOST:
          switch (*p)
          {
            default:
              break;
            case ':':
              *p = '\0';
              mr = URI_STATE_IN_PORT;
              if (port) *port = p+1;
              break;
            case '/':
              *p = '\0';
              mr = URI_STATE_IN_PATH;
              if (path) *path = p+1;
              break;
          }
          break;
        case URI_STATE_IN_PORT:
          switch (*p)
          {
            default:
              break;
            case '/':
              *p = '\0';
              mr = URI_STATE_IN_PATH;
              if (path) *path = p+1;
              break;
          }
          break;
        case URI_STATE_IN_PATH:
          switch (*p)
          {
            default:
              break;
            case '#':
              *p = '\0';
              mr = URI_STATE_IN_FRAGMENT;
              if (fragment) *fragment = p+1;
              break;
          }
          break;
      }
   }
  }
  if (*protocol && (*protocol)[0] == 0)
    *protocol = NULL;
  return 0;
}

char *_mrg_resolve_uri (const char *base_uri, const char *uri)
{
  char *ret;
  char *uri_dup = strdup (uri);

  if (!base_uri)
    return uri_dup;

  char *base_dup = strdup (base_uri);

  char *protocol = NULL;
  char *host = NULL;
  char *port = NULL;
  char *path = NULL;
  char *fragment = NULL;
  char *base_protocol = NULL;
  char *base_host = NULL;
  char *base_port = NULL;
  char *base_path = NULL;
  char *base_fragment = NULL;

  //int retlen = 0;
  int samehost = 0;

  split_uri (uri_dup, &protocol, &host, &port, &path, &fragment);
  split_uri (base_dup, &base_protocol, &base_host, &base_port, &base_path, &base_fragment);

  if (!protocol)
    protocol = base_protocol;
  if (!host)
  {
    host = base_host;
    port = base_port;
    samehost = 1;
  }
  ret = malloc (
      (path?strlen (path):0)
      + (fragment?strlen (fragment):0) +
      (host?strlen (host):0) + 640);
  if (protocol)
  {
    if (uri[0] == '/' && uri[1] != '/')
      sprintf (ret, "%s://%s%s%s%s", protocol, host, port?":":"", port?port:"", uri);
    else if (uri[0] == '.' && uri[1] == '.' && uri[2] == '/' &&
             uri[3] == '.' && uri[4] == '.')
    {
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      if (base_path[strlen (base_path)-1] == '/')
        base_path[strlen (base_path)-1] = 0;
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      else
        base_path[0]=0;
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      if (base_path[strlen (base_path)-1] == '/')
        base_path[strlen (base_path)-1] = 0;
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      else
        base_path[0]=0;

      sprintf (ret, "c%s://%s%s%s/%s%s%s%s", protocol, host, port?":":"", port?port:"", samehost?base_path:"", &path[6], fragment?"#":"", fragment?fragment:"");
    }
    else if (uri[0] == '.' && uri[1] == '.')
    {
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      if (base_path[strlen (base_path)-1] == '/')
        base_path[strlen (base_path)-1] = 0;
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      else
        base_path[0]=0;

      sprintf (ret, "%s://%s%s%s/%s%s%s%s", protocol, host, port?":":"", port?port:"", samehost?base_path:"", &path[3], fragment?"#":"", fragment?fragment:"");
    }
    else
   {
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      else if (strchr (base_path, '/'))
        strchr (base_path, '/')[1] = 0;
      else
        base_path[0] = 0;

      if (host)
      sprintf (ret, "%s://%s%s%s/%s%s%s%s", protocol, host, port?":":"", port?port:"", samehost?base_path:"", path, fragment?"#":"", fragment?fragment:"");
      else
      sprintf (ret, "%s:%s%s%s%s", protocol, samehost?base_path:"", path, fragment?"#":"", fragment?fragment:"");

    }
  }
  else
  {
    if (uri[0] == '/')
      sprintf (ret, "%s", path);
    else
    {
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      sprintf (ret, "/%s%s", base_path, path);
    }
  }

  free (uri_dup);
  free (base_dup);
  return ret;
}

void mrg_xml_render (Mrg *mrg,
                     char *uri_base,
                     void (*link_cb) (MrgEvent *event, void *href, void *link_data),
                     void *link_data,
                     void *(finalize)(void *listen_data, void *listen_data2, void *finalize_data),
                     void *finalize_data,
                     char *html_)
{
  char *html;
  MrgXml *xmltok;
  MrgHtml *ctx        = &mrg->html;
  char tag[64][16];
  int pos             = 0;
  int type            = t_none;
  static int depth    = 0;
  int in_style        = 0;
  int should_be_empty = 0;
  int tagpos          = 0;
  MrgString *style = mrg_string_new ("");
  int whitespaces = 0;

  html = malloc (strlen (html_) + 3);
  sprintf (html, "%s ", html_);
  xmltok = xmltok_buf_new (html);

  {
    int no = mrg->text_listen_count;
    mrg->text_listen_data1[no] = link_data;
    mrg->text_listen_data2[no] = html_;
    mrg->text_listen_finalize[no] = (void*)finalize;
    mrg->text_listen_finalize_data[no] = finalize_data;
    mrg->text_listen_count++;
  }

  _mrg_set_wrap_edge_vfuncs (mrg, wrap_edge_left, wrap_edge_right, ctx);
  _mrg_set_post_nl (mrg, _mrg_draw_background_increment, ctx);
  ctx->mrg = mrg;
  ctx->state = &ctx->states[0];

  while (type != t_eof)
  {
    char *data = NULL;
    type = xmltok_get (xmltok, &data, &pos);

    if (type == t_tag ||
        type == t_att ||
        type == t_endtag ||
        type == t_closeemptytag ||
        type == t_closetag)
    {
      int i;
      for (i = 0; data[i]; i++)
        data[i] = tolower (data[i]);
    }

    switch (type)
    {
      case t_entity:
        {
          int i;
          int dealt_with = 0;
          if (data[0]=='#')
          {
            int c = atoi (&data[1]);
            mrg_printf (mrg, "%c", c);
          }
          else
          for (i = 0; entities[i][0] && !dealt_with; i++)
            if (!strcmp (data, entities[i][0]))
            {
              mrg_print (mrg, entities[i][1]);
              dealt_with = 1;
            }

          if (!dealt_with){
            mrg_start (mrg, "dim", (void*)((size_t)pos));
            mrg_print (mrg, data);
            mrg_end (mrg);
          }
        }
        break;
      case t_word:
        if (in_style)
        {
          mrg_stylesheet_add (mrg, data, uri_base, MRG_STYLE_XML, NULL);
        }
        else
        {
          mrg_print (mrg, data);
        }
        whitespaces = 0;
        break;

      case t_whitespace:
        if (in_style)
        {
          mrg_stylesheet_add (mrg, data, uri_base, MRG_STYLE_XML, NULL);
        }
        else
        {
          switch (mrg_style (mrg)->white_space)
          {
            case MRG_WHITE_SPACE_PRE: /* handles as pre-wrap for now */
            case MRG_WHITE_SPACE_PRE_WRAP:
              mrg_print (mrg, data);
              break;
            case MRG_WHITE_SPACE_PRE_LINE:
              switch (*data)
              {
                case ' ':
                  whitespaces ++;
                  if (whitespaces == 1)
                    mrg_print (mrg, " ");
                  break;
                case '\n':
                  whitespaces = 0;
                  break;
              }
              break;
            case MRG_WHITE_SPACE_NOWRAP: /* XXX: handled like normal, this is bad.. */
            case MRG_WHITE_SPACE_NORMAL: 
              whitespaces ++;
              if (whitespaces == 1)
                mrg_print (mrg, " ");
              break;
          }
        }
        break;
      case t_tag:
        ctx->attributes = 0;
        tagpos = pos;
        mrg_string_clear (style);
        break;
      case t_att:
        if (ctx->attributes < MRG_XML_MAX_ATTRIBUTES-1)
          strncpy (ctx->attribute[ctx->attributes], data, MRG_XML_MAX_ATTRIBUTE_LEN-1);
        break;
      case t_val:
        if (ctx->attributes < MRG_XML_MAX_ATTRIBUTES-1)
          strncpy (ctx->value[ctx->attributes++], data, MRG_XML_MAX_VALUE_LEN-1);
        break;
      case t_endtag:


        if (depth && ((!strcmp (data, "tr") && !strcmp (tag[depth-1], "td"))))
        {
          mrg_end (mrg);
          depth--;
          mrg_end (mrg);
          depth--;
        }
        if (depth && ((!strcmp (data, "tr") && !strcmp (tag[depth-1], "td"))))
        {
          mrg_end (mrg);
          depth--;
          mrg_end (mrg);
          depth--;
        }
        else if (depth && ((!strcmp (data, "dd") && !strcmp (tag[depth-1], "dt")) ||
                      (!strcmp (data, "li") && !strcmp (tag[depth-1], "li")) ||
                      (!strcmp (data, "dt") && !strcmp (tag[depth-1], "dd")) ||
                      (!strcmp (data, "td") && !strcmp (tag[depth-1], "td")) ||
                      (!strcmp (data, "tr") && !strcmp (tag[depth-1], "tr")) ||
                      (!strcmp (data, "dd") && !strcmp (tag[depth-1], "dd")) ||
                      (!strcmp (data, "p") && !strcmp (tag[depth-1], "p"))))
        {
          mrg_end (mrg);
          depth--;
        }

        strcpy (tag[depth], data);
        depth ++;

        {
          char combined[256]="";
          char *klass = (void*)get_attr (ctx, "class");
          /* XXX: spaces in class should be turned into .s making
           * it possible to use multiple classes
           */
          const char *id = get_attr (ctx, "id");

          const char *pseudo = "";

          if (klass)
          {
            klass = strdup (klass);
            if(1){
              int i;
              for (i = 0; klass[i]; i++)
                if (klass[i] == ' ')
                  klass[i]='.';
            }
          }

          //if (mrg_style(mrg)->id_ptr)
          { // XXX : perhaps do this a tiny bit differently?
            MrgGeoCache *geo = _mrg_get_cache (ctx, (void*)(size_t)(tagpos));
            if (geo && geo->hover)
            {
              if (mrg->pointer_down[1])
                pseudo = ":active:hover";
              else
                pseudo = ":hover";
            }
          }
          sprintf (combined, "%s%s%s%s%s%s",
              data,
              klass?".":"",
              klass?klass:"",
              id?"#":"",
              id?id:"", pseudo);

          if (klass)
            free (klass);
          {
            /* collect XML attributes and convert into CSS declarations */
            const char *style_attribute[] ={
              "fill-rule",
              "font-size",
              "font-family",
              "fill-color",
              "fill",
              "stroke-width",
              "stroke-color",
              "stroke-linecap",
              "stroke-miterlimit",
              "stroke-linejoin",
              "stroke",
              "color",
              "background-color",
              "background",
              NULL};

            int i;
            for (i = 0; i < ctx->attributes; i++)
            {
              int j;
              for (j = 0; style_attribute[j]; j++)
                if (!strcmp (ctx->attribute[i], style_attribute[j]))
                {
                  mrg_string_append_printf (style, "%s: %s;",
                      style_attribute[j], ctx->value[i]);
                  break;
                }
            }
          }
          mrg_string_append_str (style, get_attr (ctx, "style"));
          mrg_start_with_style (mrg, combined, (void*)((size_t)tagpos), style->str);
        }

        if (!strcmp (data, "g"))
        {
          const char *transform;
          if ((transform = get_attr (ctx, "transform")))
            {
              CtxMatrix matrix;
              mrg_parse_transform (mrg, &matrix, transform);
              ctx_transform (mrg_cr (mrg), &matrix);
            }
        }

        if (!strcmp (data, "polygon"))
        {
          mrg_parse_polygon (mrg, get_attr (ctx, "d"));
          mrg_path_fill_stroke (mrg);
        }

        if (!strcmp (data, "path"))
        {
          mrg_parse_svg_path (mrg, get_attr (ctx, "d"));
          mrg_path_fill_stroke (mrg);
        }

        if (!strcmp (data, "rect"))
        {
          float width, height, x, y;
          const char *val;
          val = get_attr (ctx, "width");
          width = val ? mrg_parse_float (mrg, val, NULL) : 0;
          val = get_attr (ctx, "height");
          height = val ? mrg_parse_float (mrg, val, NULL) : 0;
          val = get_attr (ctx, "x");
          x = val ? mrg_parse_float (mrg, val, NULL) : 0;
          val = get_attr (ctx, "y");
          y = val ? mrg_parse_float (mrg, val, NULL) : 0;

          ctx_rectangle (mrg_cr (mrg), x, y, width, height);
          mrg_path_fill_stroke (mrg);
        }

        if (!strcmp (data, "text"))
        {
          mrg->x = mrg_parse_float (mrg, get_attr (ctx, "x"), NULL);
          mrg->y = mrg_parse_float (mrg, get_attr (ctx, "y"), NULL);
        }

        if (!strcmp (data, "a"))
        {
          if (link_cb && get_attr (ctx, "href")) 
            mrg_text_listen_full (mrg, MRG_CLICK, link_cb, _mrg_resolve_uri (uri_base,  get_attr (ctx, "href")), link_data, (void*)free, NULL); //XXX: free is not invoked according to valgrind
        }

        if (!strcmp (data, "style"))
          in_style = 1;
        else
          in_style = 0;

        should_be_empty = 0;

        if (!strcmp (data, "link"))
        {
          const char *rel;
          if ((rel=get_attr (ctx, "rel")) && !strcmp (rel, "stylesheet") && get_attr (ctx, "href"))
          {
            char *contents;
            long length;
            mrg_get_contents (mrg, uri_base, get_attr (ctx, "href"), &contents, &length);
            if (contents)
            {
              mrg_stylesheet_add (mrg, contents, uri_base, MRG_STYLE_XML, NULL);
              free (contents);
            }
          }
        }

        if (!strcmp (data, "img") && get_attr (ctx, "src"))
        {
          int img_width, img_height;
          const char *src = get_attr (ctx, "src");

          if (mrg_query_image (mrg, src, &img_width, &img_height))
          {
            float width = mrg_style(mrg)->width;
            float height = mrg_style(mrg)->height;

            if (width < 1)
            {
               width = img_width;
            }
            if (height < 1)
            {
               height = img_height *1.0 / img_width * width;
            }

            _mrg_draw_background_increment (mrg, &mrg->html, 0);
            mrg->y += height;

            mrg_image (mrg,
            mrg->x,
            mrg->y - height,
            width,
            height,
            1.0f,
            src, NULL, NULL);

            mrg->x += width;
          }
          else
          {
            mrg_printf (mrg, "![%s]", src);
          }
        }
#if 1
        if (!strcmp (data, "link") ||
            !strcmp (data, "meta") ||
            !strcmp (data, "input") ||
            !strcmp (data, "img") ||
            !strcmp (data, "br") ||
            !strcmp (data, "hr"))
        {
          should_be_empty = 1;
          mrg_end (mrg);
          depth--;
        }
#endif
        break;

      case t_closeemptytag:
      case t_closetag:
        if (!should_be_empty)
        {
          if (!strcmp (data, "a"))
          {
            mrg_text_listen_done (mrg);
          }
          in_style = 0;
          mrg_end (mrg);
          depth--;

          if (strcmp (tag[depth], data))
          {
            if (!strcmp (tag[depth], "p"))
            {
              mrg_end (mrg);
              depth --;
            } else 
            if (depth > 0 && !strcmp (tag[depth-1], data))
            {
              fprintf (stderr, "%i: fixing close of %s when %s is open\n", pos, data, tag[depth]);

              mrg_end (mrg);
              depth --;
            }
            else if (depth > 1 && !strcmp (tag[depth-2], data))
            {
              int i;
              fprintf (stderr, "%i: fixing close of %s when %s was open\n", pos, data, tag[depth]);

              for (i = 0; i < 2; i ++)
              {
                depth --;
                mrg_end (mrg);
              }
            }
            else if (depth > 2 && !strcmp (tag[depth-3], data))
            {
              int i;
              fprintf (stderr, "%i: fixing close of %s when %s wass open\n", pos, data, tag[depth]);

              for (i = 0; i < 3; i ++)
              {
                depth --;
                mrg_end (mrg);
              }
            }
            else if (depth > 3 && !strcmp (tag[depth-4], data))
            {
              int i;
              fprintf (stderr, "%i: fixing close of %s when %s wasss open\n", pos, data, tag[depth]);

              for (i = 0; i < 4; i ++)
              {
                depth --;
                mrg_end (mrg);
              }
            }
            else if (depth > 4 && !strcmp (tag[depth-5], data))
            {
              int i;
              fprintf (stderr, "%i: fixing close of %s when %s wassss open\n", pos, data, tag[depth]);

              for (i = 0; i < 5; i ++)
              {
                depth --;
                mrg_end (mrg);
              }

            }
            else
            {
              if (!strcmp (data, "table") && !strcmp (tag[depth], "td"))
              {
                depth--;
                mrg_end (mrg);
                depth--;
                mrg_end (mrg);
              }
              else if (!strcmp (data, "table") && !strcmp (tag[depth], "tr"))
              {
                depth--;
                mrg_end (mrg);
              }
              else
              fprintf (stderr, "%i closed %s but %s is open\n", pos, data, tag[depth]);
            }
          }
        }
        break;
    }
  }

  xmltok_free (xmltok);
  if (depth!=0){
    fprintf (stderr, "html parsing unbalanced, %i open tags.. \n", depth);
    while (depth > 0)
    {
      fprintf (stderr, " %s ", tag[depth-1]);
      mrg_end (mrg);
      depth--;
    }
    fprintf (stderr, "\n");
  }

//  mrg_list_free (&ctx->geo_cache); /* XXX: no point in doing that here */
  mrg_string_free (style, 1);
  free (html);
}

void mrg_xml_renderf (Mrg *mrg,
                      char *uri_base,
                      void (*link_cb) (MrgEvent *event, void *href, void *link_data),
                      void *link_data,
                      char *format,
                      ...)
{
  va_list ap;
  size_t needed;
  char  *buffer;
  va_start(ap, format);
  needed = vsnprintf(NULL, 0, format, ap) + 1;
  buffer = malloc(needed);
  va_end (ap);
  va_start(ap, format);
  vsnprintf(buffer, needed, format, ap);
  va_end (ap);
  mrg_xml_render (mrg, uri_base, link_cb, link_data, NULL, NULL, buffer);
  free (buffer);
}

void mrg_print_xml (Mrg *mrg, char *xml)
{
  mrg_xml_render (mrg, NULL, NULL, NULL, NULL, NULL, xml);
}

void
mrg_printf_xml (Mrg *mrg, const char *format, ...)
{
  va_list ap;
  size_t needed;
  char  *buffer;
  va_start(ap, format);
  needed = vsnprintf(NULL, 0, format, ap) + 1;
  buffer = malloc(needed);
  va_end (ap);
  va_start(ap, format);
  vsnprintf(buffer, needed, format, ap);
  va_end (ap);
  mrg_print_xml (mrg, buffer);
  free (buffer);
}

#endif
