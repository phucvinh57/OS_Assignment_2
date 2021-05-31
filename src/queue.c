#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
	/* TODO: put a new process to queue [q] */	
	if (q->size == MAX_QUEUE_SIZE) {
		perror("OUT OF QUEUE");
		return;
	}
	q->proc[q->size] = malloc(sizeof(struct pcb_t));

	int seeker = q->size;
	for (int i = 0; i < q->size; i++) {
		if (proc->priority > q->proc[i]->priority) {
			seeker = i;
			break;
		}
	}

	for (int i = q->size - 1; i >= seeker ; i--) {
		q->proc[i + 1] = q->proc[i];
	}
	q->proc[seeker] = proc;
	q->size++;
}

struct pcb_t * dequeue(struct queue_t * q) {
	/* TODO: return a pcb whose prioprity is the highest
	 * in the queue [q] and remember to remove it from q
	 * */
	if (q->size == 0) return NULL;
	struct pcb_t * h_pri_pcb = q->proc[0];
	for (int i = 0; i < q->size - 1; i++) {
		q->proc[i] = q->proc[i + 1];
	}
	q->size--;
	q->proc[q->size] = NULL;
	return h_pri_pcb;
}