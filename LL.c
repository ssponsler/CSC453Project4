/*
 * Singly Linked List Data Structure implemented in C
 *
*/

#include <stdlib.h>
#include <assert.h>

#include "LL.h"
#include "libTinyFS.h"

int init(LinkedList **list, int (*compare)(const void *, const int))
{

   *list = malloc(sizeof(LinkedList));
   if (!*list)
      return -1;

   (*list)->head = NULL;
   (*list)->tail = NULL;
   (*list)->count = 0;
   (*list)->compare = compare;

   return 0;
}

int destroy(LinkedList **list)
{
   Node *cur;
   assert(list);
   assert(*list);

   while ((*list)->head) {
      cur = (*list)->head;
      (*list)->head = (*list)->head->next;
      free(cur);
      cur = NULL;
   }

   free(*list);
   *list = NULL;

   return 0;
}


void *get(LinkedList *list, int idx)
{
   int i = 0;
   Node *cur;
   void *ret = NULL;
   assert(list);
   assert(idx >= 0);

   cur = list->head;

   while (cur && i < idx) {
      cur = cur->next;
      i++;
   }

   if (cur)
      ret = cur->data;

   return ret;
}

int insert(LinkedList *list, void *data)
{
   Node *newNode = NULL;
   assert(list);
   assert(data);

   newNode = malloc(sizeof(Node));
   if (!newNode)
      return -1;

   newNode->data = data;
   newNode->next = NULL;
   if (list->tail)
      list->tail->next = newNode;
   if (list->count == 0)
      list->head = newNode;
   list->tail = newNode;

   list->count++;

   return 0;
}

int indexLL(LinkedList *list, int data)
{
   int idx = 0;
   Node *cur;
   assert(list);
   assert(data);

   cur = list->head;
   while (cur) {
      if (list->compare(cur->data, data) == 0)
         return idx;
      cur = cur->next;
      idx++;
   }

   return -1;
}

int empty(LinkedList *list)
{
   assert(list);

   return list->count == 0;
}

void *pop(LinkedList *list, int idx)
{
   int i = 0;
   void *ret = NULL;
   Node *cur, *prev = NULL;
   assert(list);
   assert(idx >= 0);

   if (idx == 0)
      return popHead(list);
   if (idx == list->count)
      return popTail(list);
   if (idx > list->count)
      return ret;

   cur = list->head;
   while (cur && i <= (idx - 1)) {
      prev = cur;
      cur = cur->next;
      i++;
   }

   ret = cur->data;
   prev->next = cur->next;
   free(cur);
   cur = NULL;
   list->count--;

   return ret;
}

void *popHead(LinkedList *list)
{
   void *ret = NULL;
   Node *cur;
   assert(list);

   if (list->head) {
      ret = list->head->data;
      cur = list->head;
      list->head = list->head->next;
      list->count--;
      free(cur);
      cur = NULL;
   }

   return ret;
}

void *popTail(LinkedList *list)
{
   void *ret = NULL;
   Node *cur, *prev = NULL;
   assert(list);

   cur = list->head;
   while (cur && cur->next) {
      prev = cur;
      cur = cur->next;
   }

   if (list->tail) {
      ret = list->tail->data;
      cur = list->tail;
      list->tail = prev;
      list->tail->next = NULL;
      list->count--;
      free(cur);
      cur = NULL;
   }

   return ret;
}

int size(LinkedList *list)
{
   assert(list);

   return list->count;
}

int comparator(const void *data1, const int data2)
{
   if (((FTE *)data1)->fd == data2)
      return 0;
   else
      return 1;
}