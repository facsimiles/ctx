#ifndef __CTX_LIST__
#define __CTX_LIST__

#include <stdlib.h>

#ifndef CTX_EXTERNAL_MALLOC
static inline void *ctx_realloc (void *mem, size_t old_size, size_t new_size)
{
  if (old_size){};
  return (void*)realloc (mem, new_size);
}

static inline void *ctx_malloc (size_t size)
{
  return (void*)malloc (size);
}

static inline void ctx_free (void *mem)
{
  free (mem);
}

static inline void *ctx_calloc (size_t size, size_t count)
{
  return calloc (size, count);
}

#endif

/* The whole ctx_list implementation is in the header and will be inlined
 * wherever it is used.
 */
struct _CtxList {
  void *data;
  CtxList *next;
  void (*freefunc)(void *data, void *freefunc_data);
  void *freefunc_data;
};

static inline void ctx_list_prepend_full (CtxList **list, void *data,
    void (*freefunc)(void *data, void *freefunc_data),
    void *freefunc_data)
{
  CtxList *new_= (CtxList*)ctx_calloc (1, sizeof (CtxList));
  new_->next = *list;
  new_->data=data;
  new_->freefunc=freefunc;
  new_->freefunc_data = freefunc_data;
  *list = new_;
}

static inline int ctx_list_length (CtxList *list)
{
  int length = 0;
  CtxList *l;
  for (l = list; l; l = l->next, length++);
  return length;
}

static inline void ctx_list_prepend (CtxList **list, void *data)
{
  CtxList *new_ = (CtxList*) ctx_calloc (1, sizeof (CtxList));
  new_->next= *list;
  new_->data=data;
  *list = new_;
}

static inline CtxList *ctx_list_nth (CtxList *list, int no)
{
  while (no-- && list)
    { list = list->next; }
  return list;
}

static inline void *ctx_list_nth_data (CtxList *list, int no)
{
  CtxList *l = ctx_list_nth (list, no);
  if (l)
    return l->data;
  return NULL;
}


static inline void
ctx_list_insert_before (CtxList **list, CtxList *sibling,
                       void *data)
{
  if (*list == NULL || *list == sibling)
    {
      ctx_list_prepend (list, data);
    }
  else
    {
      CtxList *prev = NULL;
      for (CtxList *l = *list; l; l=l->next)
        {
          if (l == sibling)
            { break; }
          prev = l;
        }
      if (prev)
        {
          CtxList *new_ = (CtxList*)ctx_calloc (1, sizeof (CtxList));
          new_->next = sibling;
          new_->data = data;
          prev->next=new_;
        }
    }
}

static inline void ctx_list_remove_link (CtxList **list, CtxList *link)
{
  CtxList *iter, *prev = NULL;
  if ((*list) == link)
    {
      prev = (*list)->next;
      *list = prev;
      link->next = NULL;
      return;
    }
  for (iter = *list; iter; iter = iter->next)
    if (iter == link)
      {
        if (prev)
          prev->next = iter->next;
        link->next = NULL;
        return;
      }
    else
      prev = iter;
}

static inline void ctx_list_remove (CtxList **list, void *data)
{
  CtxList *iter, *prev = NULL;
  if ((*list)->data == data)
    {
      if ((*list)->freefunc)
        (*list)->freefunc ((*list)->data, (*list)->freefunc_data);
      prev = (*list)->next;
      ctx_free (*list);
      *list = prev;
      return;
    }
  for (iter = *list; iter; iter = iter->next)
    if (iter->data == data)
      {
        if (iter->freefunc)
          iter->freefunc (iter->data, iter->freefunc_data);
        prev->next = iter->next;
        ctx_free (iter);
        break;
      }
    else
      prev = iter;
}

static inline void ctx_list_free (CtxList **list)
{
  while (*list)
    ctx_list_remove (list, (*list)->data);
}

static inline void
ctx_list_reverse (CtxList **list)
{
  CtxList *new_ = NULL;
  CtxList *l;
  for (l = *list; l; l=l->next)
    ctx_list_prepend (&new_, l->data);
  ctx_list_free (list);
  *list = new_;
}

static inline void *ctx_list_last (CtxList *list)
{
  if (list)
    {
      CtxList *last;
      for (last = list; last->next; last=last->next);
      return last->data;
    }
  return NULL;
}

static inline void ctx_list_concat (CtxList **list, CtxList *list_b)
{
  if (*list)
    {
      CtxList *last;
      for (last = *list; last->next; last=last->next);
      last->next = list_b;
      return;
    }
  *list = list_b;
}

static inline void ctx_list_append_full (CtxList **list, void *data,
    void (*freefunc)(void *data, void *freefunc_data),
    void *freefunc_data)
{
  CtxList *new_ = (CtxList*) ctx_calloc (1, sizeof (CtxList));
  new_->data=data;
  new_->freefunc = freefunc;
  new_->freefunc_data = freefunc_data;
  ctx_list_concat (list, new_);
}

static inline void ctx_list_append (CtxList **list, void *data)
{
  ctx_list_append_full (list, data, NULL, NULL);
}

static inline void
ctx_list_insert_at (CtxList **list,
                    int       no,
                    void     *data)
{
  if (*list == NULL || no == 0)
    {
      ctx_list_prepend (list, data);
    }
  else
    {
      int pos = 0;
      CtxList *prev = NULL;
      CtxList *sibling = NULL;
      for (CtxList *l = *list; l && pos < no; l=l->next)
        {
          prev = sibling;
          sibling = l;
          pos ++;
        }
      if (prev)
        {
          CtxList *new_ = (CtxList*)ctx_calloc (1, sizeof (CtxList));
          new_->next = sibling;
          new_->data = data;
          prev->next=new_;
          return;
        }
      ctx_list_append (list, data);
    }
}

static CtxList*
ctx_list_merge_sorted (CtxList* list1,
                       CtxList* list2,
    int(*compare)(const void *a, const void *b, void *userdata), void *userdata
)
{
  if (list1 == NULL)
     return(list2);
  else if (list2==NULL)
     return(list1);

  if (compare (list1->data, list2->data, userdata) >= 0)
  {
    list1->next = ctx_list_merge_sorted (list1->next,list2, compare, userdata);
    /*list1->next->prev = list1;
      list1->prev = NULL;*/
    return list1;
  }
  else
  {
    list2->next = ctx_list_merge_sorted (list1,list2->next, compare, userdata);
    /*list2->next->prev = list2;
      list2->prev = NULL;*/
    return list2;
  }
}

static void
ctx_list_split_half (CtxList*  head,
                     CtxList** list1,
                     CtxList** list2)
{
  CtxList* fast;
  CtxList* slow;
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

static inline void ctx_list_sort (CtxList **head,
    int(*compare)(const void *a, const void *b, void *userdata),
    void *userdata)
{
  CtxList* list1;
  CtxList* list2;

  /* Base case -- length 0 or 1 */
  if ((*head == NULL) || ((*head)->next == NULL))
  {
    return;
  }

  ctx_list_split_half (*head, &list1, &list2);
  ctx_list_sort (&list1, compare, userdata);
  ctx_list_sort (&list2, compare, userdata);
  *head = ctx_list_merge_sorted (list1, list2, compare, userdata);
}

static inline void ctx_list_insert_sorted (CtxList **list,
                                           void     *item,
    int(*compare)(const void *a, const void *b, void *userdata),
                                           void     *userdata)
{
  ctx_list_prepend (list, item);
  ctx_list_sort (list, compare, userdata);
}


static inline CtxList *ctx_list_find_custom (CtxList *list,
                                         void    *needle,
                                         int(*compare)(const void *a, const void *b, void *userdata),
                                         void *userdata)
{
  CtxList *l;
  for (l = list; l; l = l->next)
  {
    if (compare (l->data, needle, userdata) == 0)
      return l;
  }
  return NULL;
}

#endif
