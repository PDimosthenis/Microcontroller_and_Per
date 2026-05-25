#ifndef PRESSURE_QUEUE_H
#define PRESSURE_QUEUE_H
#include <stdint.h>

typedef struct {
	float* data;   // ????TOS?: ?p? uint8_t* t? ?????µe float*
	uint32_t head; 
	uint32_t tail; 
	uint32_t size; 
} PressureQueue;   // ????TOS?: ?et???µas?a se PressureQueue ??a ap?f??? conflict

int pressure_queue_init(PressureQueue *queue, uint32_t size);
int pressure_queue_enqueue(PressureQueue *queue, float item);
int pressure_queue_dequeue(PressureQueue *queue, float *item);
int pressure_queue_is_full(PressureQueue *queue);
int pressure_queue_is_empty(PressureQueue *queue);

#endif