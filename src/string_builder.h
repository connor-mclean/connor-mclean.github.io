/**
 * string_builder.h - String builder.
 *
 * This is a single-header-library that provides an easy-to-use string builder for C.
 *
 * Author(s): Connor McLean <cmclean0201@gmail.com>
 */
#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

#ifndef STRING_BUILDER_DEFAULT_CAP
#define STRING_BUILDER_DEFAULT_CAP 32
#endif // STRING_BUILDER_DEFAULT_CAP

/**
 * String builder structure.
 */
typedef struct string_builder_t {
	char *buf; // The string builder's buffer.

	size_t cap;  // The string builder's capacity.
	size_t len;  // The string builder' current length.
} string_builder_t;

/**
 * Initializes a string builder.
 * @param sb String builder pointer.
 */
void string_builder_init(string_builder_t *sb);

/**
 * Initializes a string builder with a specific cap.
 * @param sb  String builder pointer.
 * @param cap The initial capacity for the string builder.
 */
void string_builder_init_cap(string_builder_t *sb, size_t cap);

/**
 * Deinitializes a string builder. Frees the internal buffer and sets the
 * capacity and length to zero (0). The string builder must be reinitialized
 * with one of the `string_builder_init` procedures before it can be used again.
 * @param sb String builder pointer.
 */
void string_builder_deinit(string_builder_t *sb);

string_builder_t *string_builder_create();
string_builder_t *string_builder_create_cap(size_t cap);
void string_builder_destroy(string_builder_t *sb);

/**
 * Writes a string to the string builder's internal buffer.
 * @param sb String builder pointer.
 * @param s  String to write.
 * @return The number of bytes written.
 */
size_t string_builder_write(string_builder_t *sb, const char *s);

/**
 * Writes a formatted string to the string builder's internal buffer.
 * @param sb     String builder pointer.
 * @param format The format string.
 * @param ...    The arguments.
 * @return The number of bytes written.
 */
size_t string_builder_writef(string_builder_t *sb, const char *format, ...);

/**
 * Writes a formatted string to the string builder's internal buffer
 * from a `stdarg` list.
 * @param sb     String builder pointer.
 * @param format The format string.
 * @param args   `stdarg` list.
 * @return The number of bytes written.
 */
size_t string_builder_vwritef(string_builder_t *sb, const char *format, va_list args);

/**
 * Like string_builder_write but accepts the number of bytes, n, to write from s.
 * @param sb String builder pointer.
 * @param n  The number of bytes to write.
 * @param s  String to write.
 * @return The number of bytes written.
 */
size_t string_builder_nwrite(string_builder_t *sb, size_t n, const char *s);

/**
 * Like string_builder_writef but accepts the number of bytes, n, to write from the formatted string.
 * @param sb     String builder pointer.
 * @param n      The number of bytes to write.
 * @param format The format string.
 * @param ...    The arguments.
 * @return The number of bytes written.
 */
size_t string_builder_nwritef(string_builder_t *sb, size_t n, const char *format, ...);

/**
 * Like string_builder_vwritef but accepts the number of bytes, n, to write from the formatted string.
 * @param sb     String builder pointer.
 * @param n      The number of bytes to write.
 * @param format The format string.
 * @param args   `stdarg` list.
 * @return The number of bytes written.
 */
size_t string_builder_vnwritef(string_builder_t *sb, size_t n, const char *format, va_list args);

/**
 * Clears the string builder's internal buffer and sets the length to zero (0).
 * Capacity is maintained.
 * @param sb String builder pointer.
 */
void string_builder_clear(string_builder_t *sb);

/**
 * Grows the string builder's capacity by the provided amount.
 * @param sb   String builder pointer.
 * @param size The minimum size to grow.
 * @return `true` if the grow operation was successful; otherwise, `false.
 */
bool string_builder_grow(string_builder_t *sb, size_t size);

/**
 * Gets an allocated copy of the string builder's internal buffer.
 * The caller owns the returned string and is responsible for freeing it.
 * @param sb String builder pointer.
 * @return An allocated copy of the string builder's internal buffer.
 */
char *string_builder_to_string(const string_builder_t *sb);

#ifdef STRING_BUILDER_IMPLEMENTATION

#include <stdio.h>
#include <string.h>

void string_builder_init(string_builder_t *sb) {
	string_builder_init_cap(sb, STRING_BUILDER_DEFAULT_CAP);
}

void string_builder_init_cap(string_builder_t *sb, size_t cap) {
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

void string_builder_deinit(string_builder_t *sb) {
	if (sb == NULL) { return; }
	free(sb->buf);
	sb->cap = 0;
	sb->len = 0;
}

string_builder_t *string_builder_create() {
	return string_builder_create_cap(STRING_BUILDER_DEFAULT_CAP);
}

string_builder_t *string_builder_create_cap(size_t cap) {
	string_builder_t *sb = malloc(sizeof(string_builder_t));
	if (sb != NULL) {
		string_builder_init_cap(sb, cap);
	}
	return sb;
}

void string_builder_destroy(string_builder_t *sb) {
	if (sb != NULL) {
		string_builder_deinit(sb);
		free(sb);
	}
}

size_t string_builder_write(string_builder_t *sb, const char *s) {
	if (sb == NULL || s == NULL) { return 0; }
	const size_t len = strlen(s);
	if (len == 0) { return 0; }
	const size_t cap = sb->len + len + 1;
	if (cap >= sb->cap) {
		if (!string_builder_grow(sb, cap)) {
			return 0;
		}
	}
	memcpy(sb->buf + sb->len, s, len);
	sb->len += len;
	sb->buf[sb->len] = 0;
	return len;
}

size_t string_builder_nwrite(string_builder_t *sb, size_t n, const char *s) {
	if (sb == NULL || s == NULL) { return 0; }
	if (n == 0) { return 0; }
	const size_t cap = sb->len + n + 1;
	if (cap >= sb->cap) {
		if (!string_builder_grow(sb, cap)) {
			return 0;
		}
	}
	memcpy(sb->buf + sb->len, s, n);
	sb->len += n;
	sb->buf[sb->len] = 0;
	return n;
}

size_t string_builder_writef(string_builder_t *sb, const char *format, ...) {
	va_list args;
	va_start(args, format);
	const size_t n = string_builder_vwritef(sb, format, args);
	va_end(args);
	return n;
}

size_t string_builder_nwritef(string_builder_t *sb, size_t n, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	const size_t w = string_builder_vnwritef(sb, n, format, ap);
	va_end(ap);
	return w;
}

size_t string_builder_vwritef(string_builder_t *sb, const char *format, va_list ap) {
	va_list apcpy;
	va_copy(apcpy, ap);
	const int len = vsnprintf(NULL, 0, format, apcpy);
	va_end(apcpy);
	if (len < 0) {
		return 0;
	}
	size_t cap = sb->len + len + 1;
	if (cap >= sb->cap) {
		if (!string_builder_grow(sb, cap)) {
			return 0;
		}
	}
	char *dst = sb->buf;
	const int written = vsnprintf(dst + sb->len, len + 1, format, ap);
	if (written < 0) {
		return 0;
	}
	sb->len += written;
	sb->buf[sb->len] = 0;
	return written;
}

size_t string_builder_vnwritef(string_builder_t *sb, size_t n, const char *format, va_list ap) {
	va_list apcpy;
	va_copy(apcpy, ap);
	const int len = vsnprintf(NULL, 0, format, apcpy);
	va_end(apcpy);
	if (len < 0) {
		return 0;
	}
	size_t l = (size_t)len;
	if (l > n) {
		l = n;
	}
	size_t min_cap = sb->len+l+1;
	if (min_cap >= sb->cap) {
		if (!string_builder_grow(sb, min_cap)) {
			return 0;
		}
	}
	char *dst = sb->buf;
	const int written = vsnprintf(dst + sb->len, l+1, format, ap);
	if (written < 0) {
		return 0;
	}
	sb->len += written;
	sb->buf[sb->len] = 0;
	return written;
}

void string_builder_clear(string_builder_t *sb) {
	if (sb == NULL || sb->buf == NULL) { return; }
	memset(sb->buf, 0, sb->cap);
	sb->len = 0;
	if (sb->cap > 0) {
		sb->buf[0] = 0;
	}
}

bool string_builder_grow(string_builder_t *sb, const size_t size) {
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

char *string_builder_to_string(const string_builder_t *sb) {
	if (sb == NULL || sb->buf == NULL) { return NULL; }
	char *s = strndup(sb->buf, sb->len);
	return s;
}

#endif // STRING_BUILDER_IMPLEMENTATION

#endif // STRING_BUILDER_H
