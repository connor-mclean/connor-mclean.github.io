#ifndef SB_H
#define SB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#ifndef SB_DEFAULT_CAP
#define SB_DEFAULT_CAP 32
#endif // SB_DEFAULT_CAP

/**
 * String builder structure.
 */
typedef struct string_builder {
	size_t cap; // The string builder's capacity.
	size_t len; // The string builder' current length.

	char *buf;  // The string builder's buffer.
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
 * Initializes a string builder from a string.
 * @param sb String builder pointer.
 * @param s The string to initialize the string builder with.
 */
void sb_init_from(string_builder *sb, const char *s);

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
 * @param size The additional size to grow the capacity by.
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

void sb_init(string_builder *sb) {
	sb_init_cap(sb, SB_DEFAULT_CAP);
}

void sb_init_cap(string_builder *sb, size_t cap) {
	sb->buf = malloc(cap);
	memset(sb->buf, 0, cap);
	sb->cap = cap;
	sb->len = 0;
}

void sb_init_from(string_builder *sb, const char *s) {
	size_t len = strlen(s);
	sb->cap = len * 2;
	sb->buf = malloc(sb->cap);
	memset(sb->buf, 0, sb->cap);
	sb_write(sb, s);
}

void sb_deinit(string_builder *sb) {
	free(sb->buf);
	sb->cap = 0;
	sb->len = 0;
}

size_t sb_write(string_builder *sb, const char *s) {
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
		va_end(args);
		return 0;
	}
	if (sb->len + len + 1 >= sb->cap) {
		if (!sb_grow(sb, len*2)) { return 0; }
	}
	char *dst = sb->buf;
	const int written = vsnprintf(dst + sb->len, len+1, format, args);
	va_end(args);
	if (written < 0) { return 0; }
	sb->len += written;
	return written;
}

void sb_clear(string_builder *sb) {
	memset(sb->buf, 0, sb->cap);
	sb->len = 0;
}

bool sb_grow(string_builder *sb, const size_t size) {
	size_t cap = sb->cap + size;
	if (size < sb->cap) { return false; }
	char *buf;
	buf = realloc(sb->buf, cap);
	if (buf == NULL) { return false; }
	memset(buf + sb->len, 0, cap - sb->len);
	sb->buf = buf;
	sb->cap = cap;
	return true;
}

char *sb_to_string(const string_builder *sb) {
	char *s = strndup(sb->buf, sb->len);
	return s;
}

#endif // SB_IMPLEMENTATION

#endif // SB_H
