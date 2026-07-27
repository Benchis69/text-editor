#ifndef BUFFER_H
#define BUFFER_H

#include <stdlib.h>

typedef struct {
	size_t capacity;
	size_t size;
	char *chars;
} Line;

void line_insert_text_before(Line *line, const char *text, size_t col);
void line_backspace(Line *line, size_t col);
void line_delete(Line *line, size_t col);

#endif
