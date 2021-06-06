#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
	/* TODO: put a new process to queue [q] */	
	if(q->size == MAX_QUEUE_SIZE) {
		printf("Queue is full !\n");
		return;
	}
	q->proc[q->size] = proc;
	q->size++;
}

struct pcb_t * dequeue(struct queue_t * q) {
	/* TODO: return a pcb whose prioprity is the highest
	 * in the queue [q] and remember to remove it from q
	 * */
	if(q->size == 0) {
		return NULL;
	}
	int index = 0;
	struct pcb_t* temp = q->proc[0];
	for(int i = 1; i < q->size; ++i) {
		if(temp->priority < q->proc[i]->priority) {
			temp = q->proc[i];
			index = i;
		}
	}
	
	q->proc[index] = q->proc[q->size - 1];
	q->proc[q->size - 1] = NULL;
	q->size--;

	return temp;
}

