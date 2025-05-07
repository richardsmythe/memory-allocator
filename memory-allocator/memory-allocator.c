#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#define UNIMPLEMENTED() \
			do{\
				fprintf(stderr, "%s:%d: TODO: %s is not implemented yet\n",\
						__FILE__, __LINE__, __func__);\
				abort();\
			}while(0);

#define HEAP_CAPACITY 640000
#define CHUNK_LIST_CAPACITY 1024
#define HEAP_FREED_CAPACITY 1024

typedef struct {
	char* start;
	size_t size;
} Chunk;

typedef struct {
	size_t count;
	Chunk chunks[CHUNK_LIST_CAPACITY];
} Chunk_List;

void chunk_list_insert(Chunk_List* list, void* start, size_t size) {
	assert(list->count < CHUNK_LIST_CAPACITY);
	list->chunks[list->count].start = start;
	list->chunks[list->count].size = size;

	//printf("[INSERT] Adding chunk: Start = %p, Size = %zu bytes\n", start, size);

	for (size_t i = list->count; i > 0 && list->chunks[i].start < list->chunks[i - 1].start; --i) {
		Chunk t = list->chunks[i];
		list->chunks[i] = list->chunks[i - 1];
		list->chunks[i - 1] = t;
	}

	list->count += 1;
}

void chunk_list_merge(Chunk_List* merged_list, Chunk_List* freed_chunks_list) {
	merged_list->count = 0;

	for (size_t i = 0; i < freed_chunks_list->count; i++) {
		const Chunk current_chunk = freed_chunks_list->chunks[i];

		if (merged_list->count > 0) {
			Chunk* last_chunk_in_merged = &merged_list->chunks[merged_list->count - 1];

			// If current chunk is adjacent to the last one, merge them
			if ((size_t)last_chunk_in_merged->start + last_chunk_in_merged->size == (size_t)current_chunk.start) {
				last_chunk_in_merged->size += current_chunk.size;
			//	printf("[MERGE] Merged with previous: New size = %zu bytes\n", last_chunk_in_merged->size);
			}
			else {
				chunk_list_insert(merged_list, current_chunk.start, current_chunk.size);
			}
		}
		else {
			chunk_list_insert(merged_list, current_chunk.start, current_chunk.size);
		}
	}
}



void chunk_list_dump(const Chunk_List* list) {
	printf("Chunks (Total: %zu):\n", list->count);
	for (size_t i = 0; i < list->count; ++i) {
		printf("[DUMP] Chunk %zu: Start = %p, Size = %zu bytes\n", i, list->chunks[i].start, list->chunks[i].size);
	}
}

int chunk_list_find(const Chunk_List* list, void* ptr) {
	for (size_t i = 0; i < list->count; ++i) {
		if (list->chunks[i].start == ptr) {
			return (int)i;
		}
	}
	return -1;
}

void chunk_list_remove(Chunk_List* list, size_t index) {
	assert(index < list->count);

	//printf("[REMOVE] Removing chunk from allocated_chunks: Start = %p, Size = %zu bytes\n", list->chunks[index].start, list->chunks[index].size);
	for (size_t i = index; i < list->count - 1; ++i) {
		list->chunks[i] = list->chunks[i + 1];
	}
	list->count -= 1;
}

char heap[HEAP_CAPACITY] = { 0 };

Chunk heap_freed[HEAP_FREED_CAPACITY] = { 0 };
size_t heap_freed_size = 0;

Chunk_List allocated_chunks = { 0 };
Chunk_List freed_chunks = {
	.count = 1,
	.chunks = {
		[0] = {.start = heap, .size = sizeof(heap)}
	},
};

Chunk_List tmp_chunks = { 0 };

void* heap_alloc(size_t size) {
	if (size > 0)
	{
		chunk_list_merge(&tmp_chunks, &freed_chunks);
		freed_chunks = tmp_chunks;
		for (size_t i = 0; i < freed_chunks.count; ++i) {
			const Chunk chunk = freed_chunks.chunks[i];
			if (chunk.size >= size) {
				chunk_list_remove(&freed_chunks, i);
				const size_t tail_size = chunk.size - size;
				chunk_list_insert(&allocated_chunks, chunk.start, size);

				//printf("[ALLOC] Allocated chunk: Start = %p, Size = %zu bytes\n", chunk.start, size);

				if (tail_size > 0) {
					chunk_list_insert(&freed_chunks, chunk.start + size, tail_size);
					//printf("[ALLOC] Remaining freed chunk: Start = %p, Size = %zu bytes\n", chunk.start + size, tail_size);
				}
				return chunk.start;
			}
		}
	}
	printf("[ALLOC] Failed to allocate %zu bytes\n", size);
	return NULL;
}

void heap_free(void* ptr) {
	if (ptr != NULL) {
		const int index = chunk_list_find(&allocated_chunks, ptr);
		assert(index >= 0);
		assert(ptr == allocated_chunks.chunks[index].start);

		chunk_list_insert(&freed_chunks,
			allocated_chunks.chunks[index].start,
			allocated_chunks.chunks[index].size);

		//printf("[FREE] release memory to freed_chunks, de-allocate chunk: Start = %p, Size = %zu bytes\n", ptr, allocated_chunks.chunks[index].size);

		chunk_list_remove(&allocated_chunks, (size_t)index);

		chunk_list_merge(&tmp_chunks, &freed_chunks);
		freed_chunks = tmp_chunks;
	}
}

#define N 10
void* ptrs[N] = { 0 };

int main() {

	for (size_t i = 0; i < N; ++i) {
		ptrs[i] = heap_alloc(i);
	}

	// test fragmentaiton
	for (size_t i = 0; i < N; ++i) {
		if (i % 2 == 0) {
			heap_free(ptrs[i]);
		}
	}
	chunk_list_dump(&allocated_chunks);
	chunk_list_dump(&freed_chunks);


	// test merging
	//void* a = heap_alloc(16);
	//void* b = heap_alloc(16);

	//chunk_list_dump(&allocated_chunks);

	//heap_free(a);
	//heap_free(b);

	//chunk_list_dump(&freed_chunks);

}
