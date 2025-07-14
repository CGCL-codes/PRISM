#include <common.h>
#include <stdlib.h>
#include <stdbool.h>

Heap *heap_create(uint32_t capacity) {
    Heap *heap = malloc(sizeof(Heap));
    heap->elements = malloc(sizeof(ElementType) * capacity);
    heap->size = 0;
    heap->capacity = capacity;
    return heap;
}

void heap_free(Heap *heap) {
    if (heap) {
        free(heap->elements);
        free(heap);
    }
}

void heap_init(Heap *heap) {
    heap->size = heap->capacity;
    for (uint32_t i = 0; i < heap->capacity; i++) {
        heap->elements[i].dpu_id = i;
        heap->elements[i].workload = 0;
    }
}

void heap_push(Heap *heap, uint32_t dpu_id, double workload) {
    uint32_t i = heap->size++;
    heap->elements[i].dpu_id = dpu_id;
    heap->elements[i].workload = workload;
    while (i) {
        uint32_t j = (i - 1) >> 1;
        if (heap->elements[i].workload < heap->elements[j].workload ||
            (heap->elements[i].workload == heap->elements[j].workload &&
             heap->elements[i].dpu_id < heap->elements[j].dpu_id)) {
            ElementType tmp = heap->elements[i];
            heap->elements[i] = heap->elements[j];
            heap->elements[j] = tmp;
            i = j;
        } else break;
    }
}

uint32_t heap_pop(Heap *heap) {
    uint32_t ans = heap->elements[0].dpu_id;
    heap->elements[0] = heap->elements[--heap->size];
    uint32_t i = 0;
    while (1) {
        uint32_t left = (i << 1) + 1;
        uint32_t right = (i << 1) + 2;
        if (left >= heap->size) break;
        uint32_t min_element = i;
        if (heap->elements[left].workload < heap->elements[min_element].workload ||
            (heap->elements[left].workload == heap->elements[min_element].workload &&
             heap->elements[left].dpu_id < heap->elements[min_element].dpu_id)) {
            min_element = left;
        }
        if (right < heap->size &&
            (heap->elements[right].workload < heap->elements[min_element].workload ||
             (heap->elements[right].workload == heap->elements[min_element].workload &&
              heap->elements[right].dpu_id < heap->elements[min_element].dpu_id))) {
            min_element = right;
        }
        if (min_element == i) break;
        ElementType tmp = heap->elements[i];
        heap->elements[i] = heap->elements[min_element];
        heap->elements[min_element] = tmp;
        i = min_element;
    }
    return ans;
}