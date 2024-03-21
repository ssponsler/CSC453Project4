
#ifndef __LL_H_
#define __LL_H_

struct Node {
   void *data;
   struct Node *next;
};

typedef struct Node Node;

struct LinkedList {
   Node *head;
   Node *tail;
   int count;
   int (*compare)(const void *, const int);
};

typedef struct LinkedList LinkedList;

int init(LinkedList **l, int (*compare)(const void *, const int));
int destroy(LinkedList **l);

void *get(LinkedList *l, int idx);
int insert(LinkedList *l, void *data);
int indexLL(LinkedList *l, int data);
int empty(LinkedList *l);
void *pop(LinkedList *l, int idx);
void *popHead(LinkedList *l);
void *popTail(LinkedList *l);
int size(LinkedList *l);

int comparator(const void *data1, const int data2);

#endif // __LL_H_