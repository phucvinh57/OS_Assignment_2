#include <stdio.h>
#include <stdlib.h>
#include "queue.h"


int empty(struct queue_t * q) {
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
	/* TODO: put a new process to queue [q] */	
	if (q == NULL)
		return;

	if (q->size >= MAX_QUEUE_SIZE)
		return;

	q->proc[q->size] = proc;
	q->size += 1;
}

struct pcb_t * dequeue(struct queue_t * q) {
	/* TODO: return a pcb whose prioprity is the highest
	 * in the queue [q] and remember to remove it from q
	 * */

	int highestPriority = 0;
	for (int i = 0; i < q->size; i++)
		if (q->proc[i]->priority > highestPriority)
			highestPriority = q->proc[i]->priority;

	if (highestPriority == 0) // something wrong with the priority 
		return NULL;

	struct pcb_t * candidateProcess = NULL;
	int candidateIndex = 0;

	// Find the candidate proc
	while (q->proc[candidateIndex]->priority < highestPriority)
		candidateIndex++;

	// We should have found the candidate process
	candidateProcess = q->proc[candidateIndex];

	// Push the others process down the queue
	for (int i = candidateIndex + 1; i < q->size; i++)
		q->proc[i - 1] = q->proc[i];
	q->size -= 1;
	
	return candidateProcess;
}

