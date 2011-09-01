#ifndef __QUEUE_H__
#define __QUEUE_H__

typedef struct QueueItem
{
	struct QueueItem *next;
	void *item;
} QueueItem;

typedef struct
{
	QueueItem *first;
	QueueItem *last;
	int size;
	int free_items;
} Queue;

//-----------------------------------------------------------------------------
Queue *new_queue(int freeItems);
void queue_push(Queue *q, void *it);
void *queue_first(Queue *q);
void *queue_pop(Queue *q);
void queue_clear(Queue *q);
void queue_free(Queue **q);

#endif