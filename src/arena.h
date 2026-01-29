#ifndef ARENA_H
#define ARENA_H

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#ifndef ARENA_DEFAULT_ALIGNMENT
#define ARENA_DEFAULT_ALIGNMENT (2*sizeof(void *))
#endif /* ARENA_DEFAULT_ALIGNMENT */

/**
 * Reports whether the provided value is a power of two.
 * @param v The value to check if it's a power of two.
 * @return `true` if v is a power of two; otherwise, `false`.
 */
static inline bool pow_2(const uintptr_t v) { return (v&(v-1)) == 0; }

/**
 * Aligns the provided memory address to the specific alignment.
 * @param ptr       The address of the memory to align.
 * @param alignment The alignment to align to.
 * @return The address of the next aligned value.
 */
static uintptr_t align_forward(const uintptr_t ptr, const size_t alignment) {
	assert(pow_2(alignment));
	uintptr_t p = ptr;
	const uintptr_t a = (uintptr_t) alignment;
	const uintptr_t mod = p & (a - 1); /* faster than (p % a) because `a` is a power of two */
	if (mod != 0) {
		p += a - mod;
	}
	return p;
}

/**
 * Memory arena struct.
 */
typedef struct arena {
	unsigned char *mem; // The memory in the arena.

	size_t cap;         // The capacity of the memory arena.
	size_t curr_offset; // The current offset within the arena.
	size_t prev_offset; // The previous offset in the arena.
} arena;

/**
 * Allocates memory from the arena with the specified alignment.
 * @param a         Arena pointer.
 * @param alignment The alignment to use for the memory allocation.
 * @param size      The number of bytes to allocate from the arena.
 * @return Returns a pointer to the allocated space on success; if the
 *         additional size requested meets or exceeds the size of the arena,
 *         sets errno and returns NULL.
 */
static void *arena_aligned_alloc(arena *a, const size_t alignment, size_t size) {
	const uintptr_t curr = (uintptr_t)a->mem + (uintptr_t)a->curr_offset;
	uintptr_t offset = align_forward(curr, alignment);
	offset -= (uintptr_t)a->mem;
	if (offset+size <= a->cap) {
		void *ptr = &a->mem[offset];
		a->prev_offset = offset;
		a->curr_offset = offset + size;
		memset(ptr, 0, size);
		return ptr;
	}
	errno = ENOMEM;
	return NULL;
}

/**
 * Reallocates memory from the arena with the specified alignment.
 * @param a         Arena pointer.
 * @param alignment The alignment to use for the memory allocation.
 * @param old_mem   Pointer to the old memory.
 * @param old_size  The old memory size.
 * @param new_size  The requested size of the new arena.
 * @return Returns a pointer to the allocated space on success; if the
 *         additional size requested meets or exceeds the size of the arena,
 *         sets errno and returns NULL.
 */
static void *arena_aligned_realloc(arena *a, const size_t alignment, void *old_mem, const size_t old_size, const size_t new_size) {
	unsigned char *old = (unsigned char *)old_mem;
	assert(pow_2(alignment));
	if (old == NULL || old_size == 0) {
		return arena_aligned_alloc(a, alignment, new_size);
	} else if (a->mem <= old && old < a->mem+a->cap) {
		if (a->mem+a->prev_offset == old) {
			a->curr_offset = a->prev_offset+new_size;
			if (new_size > old_size) {
				memset(&a->mem[a->curr_offset], 0, new_size-old_size);
			}
			return old;
		} else {
			void *new_mem = arena_aligned_alloc(a, alignment, new_size);
			size_t copy_size = old_size < new_size ? old_size : new_size;
			memmove(new_mem, old, copy_size);
			return new_mem;
		}
	} else {
		/* assert(0 && "requested memory is outside the bounds of the arena"); */
		errno = ENOMEM;
		return NULL;
	}
}

/**
 * Initializes the arena with the provided memory and capacity.
 * @param a   Arena pointer.
 * @param mem Backing memory for the arena.
 * @param cap The capacity of the backing memory.
 */
void arena_init(arena *a, void *mem, size_t cap);

/**
 * Deinitializes the arena. This is "no-op".
 * @param a Arena pointer.
 */
void arena_deinit(arena *a);

/**
 * Allocates memory from the arena with the alignment defined by `DEFAULT_ALIGNMENT`.
 * @param a    Arena pointer.
 * @param size The number of bytes to allocate from the arena.
 * @return Returns a pointer to the allocated space on success; if the
 *         additional size requested meets or exceeds the size of the arena,
 *         returns `NULL`.
 */
void *arena_alloc(arena *a, size_t size);

/**
 * Reallocates the arena memory with the alignment defined by `DEFAULT_ALIGNMENT`.
 * @param a          Arena pointer.
 * @param old_memory Pointer to the old memory.
 * @param old_size   The old memory size.
 * @param new_size   The requested size of the new arena.
 * @return Returns a pointer to the newly allocated space on success; if the
 *         additional size requested meets or exceeds the size of the arena,
 *         returns `NULL`.
 */
void *arena_realloc(arena *a, void *old_memory, size_t old_size, size_t new_size);

/**
 * Returns a pointer to newly allocated memory, which is a duplicate of
 * size bytes from the object pointed to by src.
 * @param a    Arena pointer.
 * @param src  Source object.
 * @param size The number of bytes to duplicate from src.
 * @return Returns a pointer to the new object on success. On failure,
 *         sets errno and returns NULL.
 */
void *arena_memdup(arena *a, const void *src, size_t size);

/**
 * Returns a pointer to newly a newly allocated string, which is a duplicate of
 * the string pointed to by src. The new string is terminated with a null byte.
 * with a null byte.
 * @param a    Arena pointer.
 * @param src  Source string.
 * @return Returns a pointer to the new string on success. On failure,
 *         sets errno and returns NULL.
 */
char *arena_strdup(arena *a, const char *src);

/**
 * Returns a pointer to newly a newly allocated string, which is a duplicate of
 * at most size bytes from the string pointed to by src, terminating the new string
 * with a null byte.
 * @param a    Arena pointer.
 * @param src  Source string.
 * @param size The number of bytes to duplicate from src.
 * @return Returns a pointer to the new string on success. On failure,
 *         sets errno and returns NULL.
 */
char *arena_strndup(arena *a, const char *src, size_t size);

/**
 * Allocates a formatted string.
 * @param a Arena pointer.
 * @param fmt String format.
 * @return Returns a formatted string.
 */
char *arena_asprintf(arena *a, const char *fmt, ...);

/**
 * Resets the arena.
 * @param a Arena pointer.
 */
void arena_reset(arena *a);

/**
 * Like `arena_reset` but also zeroes the memory.
 * @param a Arena pointer.
 */
void arena_zero(arena *a);

#ifdef ARENA_IMPLEMENTATION

#include <stdio.h>
#include <stdarg.h>

void arena_init(arena *a, void *mem, const size_t cap) {
	a->mem = (unsigned char *)mem;
	a->cap = cap;
	a->curr_offset = 0;
	a->prev_offset = 0;
}

void arena_deinit(arena *a) {
	memset(a->mem, 0, a->curr_offset);
	a->mem = NULL;
	a->curr_offset = 0;
	a->prev_offset = 0;
	a->cap = 0;
}

void *arena_alloc(arena *a, const size_t size) {
	return arena_aligned_alloc(a, ARENA_DEFAULT_ALIGNMENT, size);
}

void *arena_realloc(arena *a, void *old_memory, const size_t old_size, const size_t new_size) {
	return arena_aligned_realloc(a, ARENA_DEFAULT_ALIGNMENT, old_memory, old_size, new_size);
}

void *arena_memdup(arena *a, const void *src, size_t size) {
	void *dup = arena_alloc(a, size);
	if (dup != NULL) {
		memcpy(dup, src, size);
	}
	return dup;
}

char *arena_strdup(arena *a, const char *src) {
	size_t len = strlen(src)+1;
	char *dup = arena_alloc(a, len);
	if (dup != NULL) {
		memcpy(dup, src, len);
		dup[len] = '\0';
	}
	return dup;
}

char *arena_strndup(arena *a, const char *src, const size_t size) {
	size_t len = strnlen(src, size);
	char *dup = arena_alloc(a, len+1);
	if (dup != NULL) {
		memcpy(dup, src, len);
		dup[len] = '\0';
	}
	return dup;
}

char *arena_asprintf(arena *a, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	va_list args_copy;
	va_copy(args_copy, args);
	int len = vsnprintf(NULL, 0, fmt, args_copy);
	va_end(args_copy);
	if (len < 0) {
		va_end(args);
		return NULL;
	}
	char *out = arena_alloc(a, len+1);
	if (out == NULL) {
		va_end(args);
		return NULL;
	}
	vsnprintf(out, len+1, fmt, args);
	va_end(args);
	return out;
}

void arena_reset(arena *a) {
	a->curr_offset = 0;
	a->prev_offset = 0;
}

void arena_zero(arena *a) {
	memset(a->mem, 0, a->curr_offset);
	arena_reset(a);
}

#endif /* ARENA_IMPLEMENTATION */

#endif /* ARENA_H */
