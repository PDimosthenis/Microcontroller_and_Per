#include "pressure_queue.h"
#include <stdlib.h>

int pressure_queue_init(PressureQueue *queue, uint32_t size) {
	queue->data = (float*)malloc(sizeof(float) * size);
	queue->head = 0;
	queue->tail = 0;
	queue->size = size;
	
	return queue->data != 0;
}

int pressure_queue_enqueue(PressureQueue *queue, float item) {
	if (!pressure_queue_is_full(queue)) {
		queue->data[queue->tail++] = item;
		queue->tail %= queue->size;
		return 1;
	} else {
		return 0;
	}
}

int pressure_queue_dequeue(PressureQueue *queue, float *item) {
	if (!pressure_queue_is_empty(queue)) {
		*item = queue->data[queue->head++];
		queue->head %= queue->size;
		return 1;
	} else {
		return 0;
	}
}

int pressure_queue_is_full(PressureQueue *queue) {
	return ((queue->tail + 1) % queue->size) == queue->head;
}

int pressure_queue_is_empty(PressureQueue *queue) {
	return queue->tail == queue->head;
}