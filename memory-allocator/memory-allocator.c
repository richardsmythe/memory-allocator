#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>

#define UNIMPLEMENTED() \
			do{\
				fprintf(stderr, "%s:%d: TODO: %s is not implemented yet\n",\
						__FILE__, __LINE__, __func__);\
				abort();\
			}while(0);

#define HEAP_CAPACITY 640000
#define CHUNK_LIST_CAPACITY 1024
#define HEAP_FREED_CAPACITY 1024
#define HEADER_SIZE sizeof(size_t)

typedef struct {
	char* start;
	size_t size;
} Chunk;

typedef struct {
	size_t count;
	Chunk chunks[CHUNK_LIST_CAPACITY];
} Chunk_List;

char heap[HEAP_CAPACITY] = { 0 };

Chunk heap_freed[HEAP_FREED_CAPACITY] = { 0 };
size_t heap_freed_size = 0;

Chunk_List freed_chunks = {
	.count = 1,
	.chunks = {
		[0] = {.start = heap, .size = sizeof(heap)}
	},
};

Chunk_List tmp_chunks = { 0 };

void chunk_list_insert(Chunk_List* list, void* start, size_t size) {
	assert(list->count < CHUNK_LIST_CAPACITY);
	list->chunks[list->count].start = start;
	list->chunks[list->count].size = size;

	// Sorting the chunks based on the starting address
	for (size_t i = list->count; i > 0 && list->chunks[i].start < list->chunks[i - 1].start; --i) {
		Chunk t = list->chunks[i];
		list->chunks[i] = list->chunks[i - 1];
		list->chunks[i - 1] = t;
	}

	list->count += 1;
}

void chunk_list_merge(Chunk_List* merged_list, Chunk_List* freed_chunks_list) {
	merged_list->count = 0;
	size_t merged = 0;

	for (size_t i = 0; i < freed_chunks_list->count; i++) {
		const Chunk current_chunk = freed_chunks_list->chunks[i];

		if (merged_list->count > 0) {
			Chunk* last_chunk_in_merged_list = &merged_list->chunks[merged_list->count - 1];

			// if they are adjacent, it merges them by increasing the size of the previous chunk.
			if ((size_t)last_chunk_in_merged_list->start + last_chunk_in_merged_list->size == (size_t)current_chunk.start) {
				last_chunk_in_merged_list->size += current_chunk.size;
				++merged;
			}
			else {
				
				chunk_list_insert(merged_list, current_chunk.start, current_chunk.size);
			}
		}
		else {
			// insert first chunk, if merged_list is empty
			chunk_list_insert(merged_list, current_chunk.start, current_chunk.size);
		}
	}

	if (merged > 0) {
		printf("[MERGE] Coalesced %zu adjacent chunk%s. Total free chunks: %zu\n",
			merged, merged > 1 ? "s" : "", merged_list->count);
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
	for (size_t i = index; i < list->count - 1; ++i) {
		list->chunks[i] = list->chunks[i + 1];
	}
	list->count -= 1;
}

void* heap_alloc(size_t size) {
	if (size == 0) {
		printf("[ALLOC] Failed: Cannot allocate 0 bytes\n");
		return NULL;
	}

	chunk_list_merge(&tmp_chunks, &freed_chunks);
	freed_chunks = tmp_chunks;
	tmp_chunks.count = 0;

	const size_t total_size = size + HEADER_SIZE;

	for (size_t i = 0; i < freed_chunks.count; ++i) {
		const Chunk chunk = freed_chunks.chunks[i];

		if (chunk.size >= total_size) {
			void* allocated_block_start = chunk.start;
			chunk_list_remove(&freed_chunks, i);

			size_t* header = (size_t*)allocated_block_start;
			*header = size;

			void* ptr = (char*)allocated_block_start + HEADER_SIZE;
			size_t remaining_space = chunk.size - total_size;

			if (remaining_space > 0) {
				void* unused_chunk_start = (char*)allocated_block_start + total_size;
				chunk_list_insert(&freed_chunks, unused_chunk_start, remaining_space);
			}

			printf("[ALLOC] %zu bytes allocated at %p (raw: %p). Remaining in chunk: %zu bytes\n",
				size, ptr, allocated_block_start, remaining_space);
			return ptr;
		}
	}

	printf("[ALLOC] Failed: Not enough memory for %zu bytes\n", size);
	return NULL;
}

void heap_free(void* ptr) {
	if (ptr != NULL) {
		void* chunk_start = (uint8_t*)ptr - HEADER_SIZE;
		size_t chunk_size = *(size_t*)chunk_start + HEADER_SIZE;

		chunk_list_insert(&freed_chunks, chunk_start, chunk_size);

		chunk_list_merge(&tmp_chunks, &freed_chunks);
		freed_chunks = tmp_chunks;
		tmp_chunks.count = 0;

		printf("[FREE] Freed %zu bytes at %p (raw: %p)\n", chunk_size - HEADER_SIZE, ptr, chunk_start);
	}
}

void chunk_list_dump(const Chunk_List* list) {
	printf("======== Free List Dump (Total: %zu) ========\n", list->count);
	for (size_t i = 0; i < list->count; ++i) {
		printf("  Chunk %zu: Start = %p, Size = %zu bytes\n", i, list->chunks[i].start, list->chunks[i].size);
	}
	printf("=============================================\n");
}

int main() {
	void* ptrs[3];
	ptrs[0] = heap_alloc(1);
	ptrs[1] = heap_alloc(2);
	//ptrs[2] = heap_alloc(3);

	heap_free(ptrs[0]);
	heap_free(ptrs[1]);
	//heap_free(ptrs[2]);


	chunk_list_dump(&freed_chunks);
	return 0;
}
