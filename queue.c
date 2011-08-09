#include <stdlib.h>

#include "queue.h"

//-----------------------------------------------------------------------------
Queue *new_queue(int freeItems)
{
	Queue *q = malloc(sizeof(Queue));
	q->first = NULL;
	q->last = NULL;
	q->size = 0;
	q->free_items = freeItems;
	return q;
}

//-----------------------------------------------------------------------------
void queue_push(Queue *q, void *it)
{
	QueueItem *qitem = malloc(sizeof(QueueItem));
	qitem->next = NULL;
	qitem->item = it;
	if (!q->first) {
		q->first = qitem;
		q->last = qitem;			
	} else {
		q->last->next = qitem;
		q->last = qitem;
	}
	q->size++;
}

//-----------------------------------------------------------------------------
void *queue_first(Queue *q)
{
	if (q->first) {				
		return q->first->item;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
void *queue_pop(Queue *q)
{
	if (q->first) {	
		QueueItem *remitem = q->first;
		q->first = q->first->next;
		q->size--;
		if (q->free_items) {
			free(remitem->item);
			free(remitem);
			return NULL;
		} else {
			void *item = remitem->item;		
			free(remitem);
			return item;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
void queue_clear(Queue *q)
{
	while (q->first) {	
		queue_pop(q);
	}
	q->size = 0;
	q->last = NULL;
}

//-----------------------------------------------------------------------------
void queue_free(Queue **q)
{
	queue_clear(*q);
	free(*q);
	*q = NULL;
}