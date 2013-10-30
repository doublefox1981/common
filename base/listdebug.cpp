
#include "list.h"
#include <assert.h>
#include <stdio.h>
/*
* Insert a new entry between two known consecutive entries.
*
* This is only for internal list manipulation where we know
* the prev/next entries already!
*/

#ifdef CONFIG_DEBUG_LIST

void __list_add(struct list_head *new_,
struct list_head *prev,
struct list_head *next)
{
  if(next->prev != prev)
  {
    printf("list_add corruption. next->prev should be "
      "prev (%p), but was %p. (next=%p).\n",prev, next->prev, next);
    assert(0);
  }
  if(prev->next != next)
  {
    printf("list_add corruption. prev->next should be "
      "next (%p), but was %p. (prev=%p).\n",
      next, prev->next, prev);
    assert(0);
  }
  next->prev = new_;
  new_->next = next;
  new_->prev = prev;
  prev->next = new_;
}
//EXPORT_SYMBOL(__list_add);

/**
* list_del - deletes entry from list.
* @entry: the element to delete from the list.
* Note: list_empty on entry does not return true after this, the entry is
* in an undefined state.
*/
void list_del(struct list_head *entry)
{
  if(entry->prev->next != entry)
  {
    printf("list_del corruption. prev->next should be %p, "
    "but was %p\n", entry, entry->prev->next);
    assert(0);
  }
  if(entry->next->prev != entry)
  {
    printf("list_del corruption. next->prev should be %p, "
    "but was %p\n", entry, entry->next->prev);
    assert(0);
  }
  __list_del(entry->prev, entry->next);
  entry->next = (struct list_head *)LIST_POISON1;
  entry->prev = (struct list_head *)LIST_POISON2;
}
//EXPORT_SYMBOL(list_del);

#endif//CONFIG_DEBUG_LIST