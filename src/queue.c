#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
	/* TODO: put a new process to queue [q] */
	if (q->size == MAX_QUEUE_SIZE) {
		return;
	}
	q->proc[q->size] = proc;
	q->size++;
}

struct pcb_t * dequeue(struct queue_t * q) {
	/* TODO: return a pcb whose prioprity is the highest
	 * in the queue [q] and remember to remove it from q
	 * */
	if (empty(q)) {
		return NULL;
	}

	int stepIndex = 1;
	int maxPriorityIndex = 0;
	struct pcb_t* proc = NULL;
	//Find the largest priority process:
	while(stepIndex < q->size) {
		if (q->proc[maxPriorityIndex]->priority < q->proc[stepIndex]->priority)
			maxPriorityIndex = stepIndex;
		++stepIndex;
	}
	//Get maxPriority process:
	proc = q->proc[maxPriorityIndex];
	//Shift the process to left.
	stepIndex = maxPriorityIndex;
	while (stepIndex < q->size - 1) {
		q->proc[stepIndex] = q->proc[stepIndex + 1];
		stepIndex++;
	}
	//Decrease size of queue if max priority exist: 
	if (proc) {
		q->proc[q->size - 1] = NULL;
		q->size--;
	}
	return proc;
}

