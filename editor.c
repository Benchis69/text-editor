#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include "./editor.h"

#define SV_IMPLEMENTATION
#include "./sv.h"

#define LINE_INIT_CAPACITY 1024
#define EDITOR_INIT_CAPACITY 128

static void line_expand(Line *line, size_t n) {
	
	size_t new_capacity = line->capacity;
	
	assert(new_capacity >= line->size);
	while (new_capacity - line->size < n + 1) {
		if (new_capacity == 0) {
			new_capacity = LINE_INIT_CAPACITY;
		}
		else {
			new_capacity *= 2;
		}
	}
	
	if (new_capacity != line->capacity) {
		line->chars = realloc(line->chars, new_capacity);
		line->capacity = new_capacity;
	}
}

void line_append_text(Line *line, const char *text) {

	line_append_text_sized(line, text, strlen(text));
}

void line_append_text_sized(Line *line, const char *text, size_t text_size)  {

	size_t col = line->size;
	line_insert_text_sized_before(line, text, text_size, &col);
}

void line_insert_text_sized_before(Line *line, const char *text, size_t text_size, size_t *col) {
	
	if (*col > line->size) *col = line->size;

	line_expand(line, text_size);

	memmove(line->chars + *col + text_size, line->chars + *col, line->size - *col);
	memcpy(line->chars + *col, text, text_size); 
	line->size += text_size;
	*col += text_size;

	line->chars[line->size] = '\0';
}

void line_insert_text_before(Line *line, const char *text, size_t *col) {

	line_insert_text_sized_before(line, text, strlen(text), col);
}

void line_backspace(Line *line, size_t *col) {
	
	if (*col > line->size) *col = line->size;

	if (*col > 0 && line->size > 0) {
		memmove(line->chars + *col - 1, line->chars + *col, line->size - *col);
		line->size -= 1;
		
		line->chars[line->size] = '\0';
		*col -= 1;
	}
}

void line_delete(Line *line, size_t *col) {
	
	if (*col > line->size) *col = line->size;

	size_t chars_to_move = line->size - *col;
	
	if(*col < line->size && line->size > 0) {
		memmove(line->chars + *col, line->chars + *col + 1, chars_to_move);
		line->size -= 1;

		line->chars[line->size] = '\0';
	}
}

static void editor_expand(Editor *editor, size_t n) {
	
	size_t new_capacity = editor->capacity;
	
	assert(new_capacity >= editor->size);
	while (new_capacity - editor->size < n + 1) {
		if (new_capacity == 0) {
			new_capacity = EDITOR_INIT_CAPACITY;
		}
		else {
			new_capacity *= 2;
		}
	}
	
	if (new_capacity != editor->capacity) {
		editor->lines = realloc(editor->lines, new_capacity * sizeof(editor->lines[0]));
		editor->capacity = new_capacity;
	}
}

void editor_init(Editor *editor) {
	memset(editor, 0, sizeof(*editor));

	editor_expand(editor, 1);

	memset(&editor->lines[0], 0, sizeof(Line));

	editor->size = 1;
	editor->cursor_row = 0;
	editor->cursor_col = 0;
}

void editor_insert_new_line(Editor *editor) {
	
	if (editor->cursor_row > editor->size) editor->cursor_row = editor->size;
	
	editor_expand(editor, 1);
	
	const size_t line_size = sizeof(editor->lines[0]);
	
	memmove(editor->lines + editor->cursor_row + 1, editor->lines + editor->cursor_row, (editor->size - editor->cursor_row) * line_size);
	memset(&editor->lines[editor->cursor_row + 1], 0, line_size);

	Line *current_line = &editor->lines[editor->cursor_row];
	Line *new_line = &editor->lines[editor->cursor_row + 1];

	if (editor->cursor_col > current_line->size) {
		editor->cursor_col = current_line->size;
	}

	size_t chars_to_move = current_line->size - editor->cursor_col;

	if (chars_to_move > 0) {
		line_expand(new_line, chars_to_move);
		memcpy(new_line->chars, current_line->chars + editor->cursor_col, chars_to_move);
		new_line->size = chars_to_move;
		new_line->chars[new_line->size] = '\0';
	}

	current_line->size = editor->cursor_col;

	if (current_line->chars != NULL) {
		current_line->chars[current_line->size] = '\0';
	}

	editor->cursor_row += 1;
	editor->cursor_col = 0;
	editor->size += 1;
}

static void editor_create_first_new_line(Editor *editor) {

	if (editor->cursor_row >= editor->size) {
		if (editor->size > 0) {
			editor->cursor_row = editor->size - 1;
		}

		else {
			editor_expand(editor, 1);
			memset(&editor->lines[editor->size], 0, sizeof(editor->lines[0]));
			editor->size += 1;
		}
	}
}

void editor_insert_text_before_cursor(Editor *editor, const char *text) {
	
	editor_create_first_new_line(editor);

	for (size_t i = 0; text[i] != '\0'; i++) {
		if (text[i] == '\t') {
			line_insert_text_before(&editor->lines[editor->cursor_row], "    ", &editor->cursor_col);
		}
		else if (text[i] != '\r') {
			char c_str[2] = {text[i], '\0'};
			line_insert_text_before(&editor->lines[editor->cursor_row], c_str, &editor->cursor_col);
		}
	}

}

void editor_backspace(Editor *editor) {

	if (editor->size == 0) return;	

	editor_create_first_new_line(editor);
	
	line_backspace(&editor->lines[editor->cursor_row], &editor->cursor_col);
}

void editor_delete(Editor *editor) {

	if (editor->size == 0) return;	

	editor_create_first_new_line(editor);
	
	line_delete(&editor->lines[editor->cursor_row], &editor->cursor_col);
}

void editor_delete_line(Editor *editor, size_t *row) {
	
	if (editor->lines == NULL || editor->size == 1) return;

	if (editor->lines[*row].chars != NULL) {
        	free(editor->lines[*row].chars);
    	}
	
	size_t lines_to_move = editor->size - *row -1;

	if (lines_to_move > 0) {
		memmove(editor->lines + *row, editor->lines + *row + 1, lines_to_move * sizeof(Line));
	}

	editor->size -= 1;
	editor->cursor_row -= 1; // kann vielleicht weg

	if (editor->cursor_row >= editor->size) {
        	editor->cursor_row = editor->size > 0 ? editor->size - 1 : 0;
	}

	if (editor->cursor_col > editor->lines[editor->cursor_row].size) {
        	editor->cursor_col = editor->lines[editor->cursor_row].size;
	}
}

const char *editor_char_under_cursor(const Editor *editor) {

	if (editor->cursor_row < editor->size) {
		if (editor->cursor_col < editor->lines[editor->cursor_row].size) {
			return &editor->lines[editor->cursor_row].chars[editor->cursor_col];
		}
	}

	return NULL;
}

void editor_save_to_file(const Editor *editor, const char *file_path) {

	FILE *f = fopen(file_path, "w");
	if (!f) {
		fprintf(stdout, "ERROR: could not open file `%s`: %s\n", file_path, strerror(errno));
		exit(1);
	}

	for (size_t row = 0; row < editor->size; row++) {
		fwrite(editor->lines[row].chars, 1, editor->lines[row].size, f);
		fputc('\n', f);
	}
	
	fclose(f);
}

void line_append_filtered(Line *line, const char *text, size_t text_size) {
	for (size_t i = 0; i < text_size; i++) {
		if (text[i] == '\t') {
			line_append_text_sized(line, "    ", 4);
		}
		else if (text[i] != '\r') {
			line_append_text_sized(line, &text[i], 1);
		}
	}
}

void editor_load_from_file(Editor *editor, FILE *file) {

	editor_create_first_new_line(editor);

	static char chunk[1024 * 640];
	
	while (!feof(file)) {
		size_t n = fread(chunk, 1, sizeof(chunk), file);
		
		String_View chunk_sv = {
			.data = chunk,
			.count = n
		};

		while (chunk_sv.count > 0) {
			String_View chunk_line = {0};
			Line *line = &editor->lines[editor->size - 1];

			if (sv_try_chop_by_delim(&chunk_sv, '\n', &chunk_line)) {

				line_append_filtered(line, chunk_line.data, chunk_line.count);

				editor->cursor_col = line->size;
				editor_insert_new_line(editor);
			}

			else {
				line_append_filtered(line, chunk_sv.data, chunk_sv.count);
				chunk_sv = SV_NULL;
			}
			
		}
	}

	editor->cursor_row = 0;
	editor->cursor_col = 0;
}




