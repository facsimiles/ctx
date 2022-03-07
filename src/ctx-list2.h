#ifndef __CTX_LIST2__
#define __CTX_LIST2__

#include <stdlib.h>

/* these exist for emscripten integration of micropython, all
 * other bits of ctx seem OK, but the way js events inject
 * ctx events asynchronously with micropython the gc might
 * kick in and free the temporary hitlists, this will not be
 * a problem when things are done from micropython more, properly
 */

static inline void ctx_list2_prepend (CtxList **list, void *data)
{
  CtxList *new_ = (CtxList*) calloc (sizeof (CtxList), 1);
  new_->next = *list;
  new_->data =data;
  *list = new_;
}

static inline void ctx_list2_remove (CtxList **list, void *data)
{
  CtxList *iter, *prev = NULL;
  if ((*list)->data == data)
    {
      if ((*list)->freefunc)
        (*list)->freefunc ((*list)->data, (*list)->freefunc_data);
      prev = (*list)->next;
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

static inline void ctx_list2_free (CtxList **list)
{
  while (*list)
    ctx_list2_remove (list, (*list)->data);
}

static inline void
ctx_list2_reverse (CtxList **list)
{
  CtxList *new_ = NULL;
  CtxList *l;
  for (l = *list; l; l=l->next)
    ctx_list2_prepend (&new_, l->data);
  ctx_list2_free (list);
  *list = new_;
}

#endif
