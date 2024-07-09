#ifndef SB_H
#define SB_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

#ifndef SB_DEFAULT_CAP
#define SB_DEFAULT_CAP 32
#endif // SB_DEFAULT_CAP

/**
 * String builder structure.
 */
typedef struct string_builder {
    char *buf;  // The string builder's buffer.
	size_t cap; // The string builder's capacity.
	size_t len; // The string builder' current length.
} string_builder;

/**
 * Initializes a string builder.
 * @param sb String builder pointer.
 */
void sb_init(string_builder *sb);

/**
 * Initializes a string builder with a specific cap.
 * @param sb String builder pointer.
 * @param cap The initial capacity for the string builder.
 */
void sb_init_cap(string_builder *sb, size_t cap);

/**
 * Deinitializes a string builder. Frees the internal buffer and sets the
 * capacity and length to zero (0). The string builder must be reinitialized
 * with one of the `sb_init` procedures before it can be used again.
 * @param sb String builder pointer.
 */
void sb_deinit(string_builder *sb);

/**
 * Writes a string to the string builder's internal buffer.
 * @param sb String builder pointer.
 * @param s String to write.
 * @return The number of bytes written.
 */
size_t sb_write(string_builder *sb, const char *s);

/**
 * Writes a formatted string to the string builder's internal buffer.
 * @param sb String builder pointer.
 * @param format The format string.
 * @param ... The arguments.
 * @return The number of bytes written.
 */
size_t sb_writef(string_builder *sb, const char *format, ...);

/**
 * Writes a formatted string to the string builder's internal buffer
 * from a `stdarg` list.
 * @param sb String builder pointer.
 * @param format The format string.
 * @param args `stdarg` list.
 * @return The number of bytes written.
 */
size_t sb_vwritef(string_builder *sb, const char *format, va_list args);

/**
 * Clears the string builder's internal buffer and sets the length to zero (0).
 * Capacity is maintained.
 * @param sb String builder pointer.
 */
void sb_clear(string_builder *sb);

/**
 * Grows the string builder's capacity by the provided amount.
 * @param sb String builder pointer.
 * @param size The minimum size to grow.
 * @return `true` if the grow operation was successful; otherwise, `false.
 */
bool sb_grow(string_builder *sb, size_t size);

/**
 * Gets an allocated copy of the string builder's internal buffer.
 * The caller owns the returned string and is responsible for freeing it.
 * @param sb String builder pointer.
 * @return An allocated copy of the string builder's internal buffer.
 */
char *sb_to_string(const string_builder *sb);

#ifdef SB_IMPLEMENTATION

#include <stdio.h>
#include <string.h>

void sb_init(string_builder *sb) {
	sb_init_cap(sb, SB_DEFAULT_CAP);
}

void sb_init_cap(string_builder *sb, size_t cap) {
	cap = cap > 0 ? cap : 1;
	sb->buf = malloc(cap);
	if (sb->buf == NULL) {
		sb->cap = 0;
		sb->len = 0;
		return;
	}
	memset(sb->buf, 0, cap);
	sb->cap = cap;
	sb->len = 0;
}

void sb_deinit(string_builder *sb) {
	if (sb == NULL) { return; }
	free(sb->buf);
	sb->cap = 0;
	sb->len = 0;
}

size_t sb_write(string_builder *sb, const char *s) {
	if (sb == NULL || s == NULL) { return 0; }
	return sb_writef(sb, "%s", s);
}

size_t sb_writef(string_builder *sb, const char *format, ...) {
	va_list args;
	va_start(args, format);
	const size_t n = sb_vwritef(sb, format, args);
	va_end(args);
	return n;
}

size_t sb_vwritef(string_builder *sb, const char *format, va_list args) {
	va_list args_copy;
	va_copy(args_copy, args);
	const int len = vsnprintf(NULL, 0, format, args_copy);
	va_end(args_copy);
	if (len < 0) {
		return 0;
	}
	size_t min_cap = sb->len+len+1;
	if (min_cap >= sb->cap) {
		if (!sb_grow(sb, min_cap)) {
			return 0;
		}
	}
	char *dst = sb->buf;
	const int written = vsnprintf(dst + sb->len, len+1, format, args);
	if (written < 0) {
		return 0;
	}
	sb->len += written;
	return written;
}

void sb_clear(string_builder *sb) {
	if (sb == NULL || sb->buf == NULL) { return; }
	memset(sb->buf, 0, sb->cap);
	sb->len = 0;
	if (sb->cap > 0) {
		sb->buf[0] = 0;
	}
}

bool sb_grow(string_builder *sb, const size_t size) {
	if (sb == NULL || size == 0) { return false; }
	size_t new_cap = size;
	if (new_cap < sb->cap) { return false; }
	char *buf = realloc(sb->buf, new_cap);
	if (buf == NULL) { return false; }
	memset(buf + sb->len, 0, new_cap - sb->len);
	sb->buf = buf;
	sb->cap = new_cap;
	return true;
}

char *sb_to_string(const string_builder *sb) {
	if (sb == NULL || sb->buf == NULL) { return NULL; }
	char *s = strndup(sb->buf, sb->len);
	return s;
}

#endif // SB_IMPLEMENTATION

#endif // SB_H
