/**
 * arena.h - Arena allocator.
 *
 * This is a single-header-file library that provides an easy-to-use arena allocator for C.
 *
 * Author(s): Connor McLean <cmclean0201@gmail.com>
 */
#ifndef ARENA_H
#define ARENA_H

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifndef ARENA_DEFAULT_ALIGNMENT
#define ARENA_DEFAULT_ALIGNMENT (sizeof(void *) * 2)
#endif /* ARENA_DEFAULT_ALIGNMENT */

/**
 * Reports whether the provided value is a power of two.
 * @param v The value to check if it's a power of two.
 * @return `true` if v is a power of two; otherwise, `false`.
 */
static inline bool pow2(const uintptr_t v) {
	return (v & (v - 1)) == 0;
}

/**
 * Aligns the provided memory address to the specific alignment.
 * @param ptr       The address of the memory to align.
 * @param alignment The alignment to align to.
 * @return The address of the next aligned value.
 */
static uintptr_t align_forward(const uintptr_t ptr, const size_t alignment) {
	assert(pow2(alignment));
	return (ptr + alignment - 1) & ~(alignment - 1);
}

/**
 * Memory arena.
 */
typedef struct arena_t {
	uint8_t *buf;     // The backing memory buffer of the arena.
	size_t  cap;      // The capacity of the memory arena.
	size_t  curr_off; // The current offset within the arena.
	size_t  prev_off; // The previous offset in the arena.
} arena_t;

/**
 * Allocates memory from the arena with the specified alignment.
 * @param arena     Arena pointer.
 * @param size      The number of bytes to allocate from the arena.
 * @param alignment The alignment to use for the memory allocation.
 * @return Returns a pointer to the allocated space on success; if the
 *         additional size requested meets or exceeds the size of the arena,
 *         sets errno and returns NULL.
 */
static void *arena_aligned_alloc(arena_t *arena, size_t size, size_t alignment) {
	if (size == 0 || arena->buf == NULL || arena->cap == 0 || size >= arena->cap) {
		return NULL;
	}
	const uintptr_t curr_off = (uintptr_t)arena->buf + (uintptr_t)arena->curr_off;
	uintptr_t off = align_forward(curr_off, alignment);
	off -= (uintptr_t)arena->buf;
	if (off + size <= arena->cap) {
		void *ptr = &arena->buf[off];
		arena->prev_off = off;
		arena->curr_off = off + size;
		memset(ptr, 0, size);
		return ptr;
	}
	errno = ENOMEM;
	return NULL;
}

/**
 * Reallocates memory from the arena with the specified alignment.
 * @param arena     Arena pointer.
 * @param old_mem   Pointer to the old memory.
 * @param old_size  The old memory size.
 * @param new_size  The requested size of the new arena.
 * @param alignment The alignment to use for the memory allocation.
 * @return Returns a pointer to the allocated space on success; if the
 *         additional size requested meets or exceeds the size of the arena,
 *         sets errno and returns NULL.
 */
static void *arena_aligned_resize(arena_t *arena, void *ptr, const size_t old_size, const size_t new_size, size_t alignment) {
	uint8_t *old_mem = (uint8_t *)ptr;
	assert(pow2(alignment));
	if (old_mem == NULL || old_size == 0) {
		return arena_aligned_alloc(arena, new_size, alignment);
	} else if (arena->buf <= old_mem && old_mem < arena->buf + arena->cap) {
		if (arena->buf+arena->prev_off == old_mem) {
			arena->curr_off = arena->prev_off + new_size;
			if (new_size > old_size) {
				memset(arena->buf + arena->prev_off + old_size, 0, new_size - old_size);
			}
			return old_mem;
		} else {
			void *new_mem = arena_aligned_alloc(arena, new_size, alignment);
			size_t copy_size = old_size < new_size ? old_size : new_size;
			memmove(new_mem, old_mem, copy_size);
			return new_mem;
		}
	} else {
		errno = ENOMEM;
		return NULL;
	}
}

/**
 * Initializes the arena with the provided memory and capacity.
 * @param arena  Arena pointer.
 * @param buf    Backing memory for the arena.
 * @param bufsiz The capacity of the backing memory.
 */
void arena_init(arena_t *arena, void *buf, size_t bufsiz);

/**
 * Deinitializes the arena.
 * @param arena Arena pointer.
 */
void arena_deinit(arena_t *arena);

/**
 * Allocates memory from the arena.
 * @param arena Arena pointer.
 * @param size  The number of bytes to allocate from the arena.
 * @return Returns a pointer to the allocated space on success; if the
 *         additional size requested meets or exceeds the size of the arena,
 *         returns `NULL`.
 */
void *arena_alloc(arena_t *arena, size_t size);

/**
 * Allocates memory from the arena for count objects of size length.
 * @param arena Arena pointer.
 * @param count The number of objects.
 * @param size  The number of bytes to allocate from the arena.
 * @return Returns a pointer to the allocated space on success; if the
 *         additional size requested meets or exceeds the size of the arena,
 *         returns `NULL`.
 */
void *arena_calloc(arena_t *arena, size_t count, size_t size);

/**
 * Reallocates the arena memory.
 * @param arena      Arena pointer.
 * @param ptr        Pointer to the old memory.
 * @param old_size   The old memory size.
 * @param new_size   The requested size of the new arena.
 * @return Returns a pointer to the newly allocated space on success; if the
 *         additional size requested meets or exceeds the size of the arena,
 *         returns `NULL`.
 */
void *arena_resize(arena_t *arena, void *ptr, size_t old_size, size_t new_size);

/**
 * Returns a pointer to newly allocated memory, which is a duplicate of
 * size bytes from the object pointed to by src.
 * @param arena  Arena pointer.
 * @param buf    Source object.
 * @param bufsiz The number of bytes to duplicate from src.
 * @return Returns a pointer to the new object on success. On failure,
 *         sets errno and returns NULL.
 */
void *arena_memdup(arena_t *arena, const void *buf, size_t bufsiz);

/**
 * Returns a pointer to newly a newly allocated string, which is a duplicate of
 * the string pointed to by src. The new string is terminated with a null byte.
 * with a null byte.
 * @param arena Arena pointer.
 * @param s     Source string.
 * @return Returns a pointer to the new string on success. On failure,
 *         sets errno and returns NULL.
 */
char *arena_strdup(arena_t *arena, const char *s);

/**
 * Returns a pointer to newly a newly allocated string, which is a duplicate of
 * at most size bytes from the string pointed to by src, terminating the new string
 * with a null byte.
 * @param arena Arena pointer.
 * @param s     Source string.
 * @param size The number of bytes to duplicate from src.
 * @return Returns a pointer to the new string on success. On failure,
 *         sets errno and returns NULL.
 */
char *arena_strndup(arena_t *arena, const char *s, size_t size);

char *arena_sprintf(arena_t *arena, const char *format, ...);

char *arena_vsprintf(arena_t *arena, const char *format, va_list ap);

/**
 * Allocates and writes a formatted string to the location pointed at by ptr.
 * @param arena	Arena pointer.
 * @param ptr   Buffer pointer.
 * @param fmt   String format.
 * @return    The number of bytes written to the allocated string.
 */
int arena_asprintf(arena_t *arena, char **ptr, const char *fmt, ...);

/**
 * Allocates and writes a formatted string to the location pointed at by ptr.
 * @param arena  Arena pointer.
 * @param ptr    Buffer pointer.
 * @param format Format string.
 * @param ap     Format arguments.
 * @return     The number of bytes written to the allocated string.
 */
int arena_vasprintf(arena_t *arena, char **ptr, const char *format, va_list ap);

/**
 * "Frees" the arena's memory (sets the current offset to `0`).
 * @param arena Arena pointer.
 */
void arena_free_all(arena_t *arena);

typedef struct temp_arena_t {
	arena_t *arena;
	size_t prev_off;
	size_t curr_off;
} temp_arena_t;

temp_arena_t temp_arena_begin(arena_t *arena);
void temp_arena_end(temp_arena_t tmp);

#ifdef ARENA_IMPLEMENTATION

#include <stdio.h>

void arena_init(arena_t *arena, void *buf, const size_t bufsiz) {
	arena->buf = (uint8_t *)buf;
	arena->cap = bufsiz;
	arena->curr_off = 0;
	arena->prev_off = 0;
}

void arena_deinit(arena_t *arena) {
	arena->buf = NULL;
	arena->cap = 0;
	arena->curr_off = 0;
	arena->prev_off = 0;
}

void *arena_alloc(arena_t *arena, const size_t size) {
	return arena_aligned_alloc(arena, size, ARENA_DEFAULT_ALIGNMENT);
}

void *arena_calloc(arena_t *arena, size_t count, size_t size) {
	return arena_alloc(arena, count * size);
}

void *arena_resize(arena_t *arena, void *ptr, const size_t old_size, const size_t new_size) {
	return arena_aligned_resize(arena, ptr, old_size, new_size, ARENA_DEFAULT_ALIGNMENT);
}

void *arena_memdup(arena_t *arena, const void *buf, size_t bufsiz) {
	void *dup = arena_alloc(arena, bufsiz);
	if (dup != NULL) {
		memcpy(dup, buf, bufsiz);
	}
	return dup;
}

char *arena_strdup(arena_t *arena, const char *s) {
	size_t len = strlen(s);
	return arena_strndup(arena, s, len);
}

char *arena_strndup(arena_t *arena, const char *s, const size_t size) {
	size_t len = strnlen(s, size);
	char *dup = arena_alloc(arena, len+1);
	if (dup != NULL) {
		memcpy(dup, s, len);
		dup[len] = 0;
	}
	return dup;
}

char *arena_sprintf(arena_t *arena, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	char *s = arena_vsprintf(arena, format, ap);
	va_end(ap);
	return s;
}

char *arena_vsprintf(arena_t *arena, const char *format, va_list ap) {
	va_list apcpy;
	va_copy(apcpy, ap);
	int len = vsnprintf(NULL, 0, format, apcpy);
	va_end(apcpy);
	if (len < 0) {
		return NULL;
	}
	char *s = arena_alloc(arena, len + 1);
	if (s == NULL) {
		return NULL;
	}
	(void)vsnprintf(s, len + 1, format, ap);
	return s;
}

int arena_asprintf(arena_t *arena, char **ptr, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	int n = arena_vasprintf(arena, ptr, format, ap);
	va_end(ap);
	return n;
}

int arena_vasprintf(arena_t *arena, char **ptr, const char *format, va_list ap) {
	va_list apcpy;
	va_copy(apcpy, ap);
	int len = vsnprintf(NULL, 0, format, apcpy);
	va_end(apcpy);
	if (len < 0) { return -1; }
	*ptr = arena_alloc(arena, len + 1);
	if (*ptr == NULL) { return -1; }
	int n = vsnprintf(*ptr, len + 1, format, ap);
	return n;
}

void arena_free_all(arena_t *arena) {
	arena->curr_off = 0;
	arena->prev_off = 0;
}

temp_arena_t temp_arena_begin(arena_t *arena) {
	return (temp_arena_t){
		.arena    = arena,
		.prev_off = arena->prev_off,
		.curr_off = arena->curr_off,
	};
}

void temp_arena_end(temp_arena_t tmp) {
	tmp.arena->prev_off = tmp.prev_off;
	tmp.arena->curr_off = tmp.curr_off;
}

#endif /* ARENA_IMPLEMENTATION */

#endif /* ARENA_H */
