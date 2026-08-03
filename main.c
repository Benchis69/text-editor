#define SDL_MAIN_USE_CALLBACKS 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "./la.h"
#include "./editor.h"

#define BUFFER_CAPACITY 1024
#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 900
#define FPS 60
#define DELTA_TIME (1.0f / FPS)

typedef struct {
	TTF_Font *font;
	int char_w;
	int char_h;
} Font;

typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	Font  font;
	int font_scale;
	char *file_path;
	
	Editor editor;

	Vec2f camera_pos;
	Vec2f camera_vel;
} Variables;

Vec2f window_size(SDL_Window *window) {
	
	int w, h;
	SDL_GetWindowSize(window, &w, &h);

	return vec2f((float) w, (float) h);
}

Vec2f camera_project_point(void *appstate, Vec2f point) {

	Variables *vars = (Variables *) appstate;
	
	return vec2f_add(vec2f_sub(point, vars->camera_pos), vec2f_mul(window_size(vars->window), vec2fs(0.5)));
}

void render_text(SDL_Renderer *renderer, Font font, const char *text, Vec2f pos, SDL_Color color, float scale) {

	if (text == NULL || text[0] == '\0') return;
	
	// Convert text to surface
	SDL_Surface *surface = TTF_RenderText_Blended(font.font, text, 0, color);

	// Convert surface to texture 
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

	// Set position and copy size from surface
	float text_x = pos.x;
	float text_y = pos.y;
	SDL_FRect dstRect = {text_x, text_y, (float) surface->w * scale, (float) surface->h * scale};

	// Render to screen
	SDL_RenderTexture(renderer, texture, NULL, &dstRect);

	// Clean up surface and texture
	SDL_DestroySurface(surface);
	SDL_DestroyTexture(texture);
}

void render_cursor(void *appstate) {

	Variables *vars = (Variables *) appstate;

	Line *line = &vars->editor.lines[vars->editor.cursor_row];

	int cursor_x = 0;
	int dummy_h = 0;

	// Measure exact distance from text to cursor
	if (line->chars != NULL && vars->editor.cursor_col > 0) {
		TTF_GetStringSize(vars->font.font, line->chars, vars->editor.cursor_col, &cursor_x, &dummy_h);
	}

	const Vec2f pos = camera_project_point(vars, vec2f(
		(float) cursor_x * vars->font_scale,
		(float) vars->editor.cursor_row * (float) vars->font.char_h * vars->font_scale));

	const SDL_FRect rect =  {
		.x = (int) floorf(pos.x),
		.y = (int) floorf(pos.y),
		.w = 2 * vars->font_scale, // Cursor width is 2 pixel * font scale
		.h = vars->font.char_h * vars->font_scale
	};
	
	SDL_SetRenderDrawColor(vars->renderer, 0xFF, 0xFF, 0xFF, 0xFF); // Set render draw color to white
	SDL_RenderFillRect(vars->renderer, &rect);
	
	/*
	Only if I increase cursor width to a full block

	SDL_Color color = {
		.r = 0xFF,
		.g = 0xFF,
		.b = 0xFF,
		.a = 0xFF
	};

	const char *c = editor_char_under_cursor(&vars->editor);
		
	if (c) {
		render_text(vars->renderer, vars->font, c, pos, color, vars->font_scale);
	}
	*/
}

int load_font_from_file(void *appstate, const char *file_path, int size) {
	
	Variables *vars = (Variables *) appstate;

	int w1, w2, h;
	
	vars->font.font = TTF_OpenFont(file_path, size);
	if (!vars->font.font) {
		SDL_Log("Couldn't load font: %s\n", SDL_GetError());
		return 1;
	}

	// Get values for char "A", works only for monospace fonts
	TTF_GetStringSize(vars->font.font, "A", 0,  &w1, &h);

	// Get values for AA to measure real distance
	TTF_GetStringSize(vars->font.font, "AA", 0, &w2, &h);

	vars->font.char_w = w2 - w1;
	vars->font.char_h = h;

	return 0;
}

void usage(FILE *stream) {

	fprintf(stream, "Usage: text editor [FILE-PATH]");
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char** argv) {

	char *file_path = NULL;
	
	// Set file path
	if (argc > 1) {
		file_path = argv[1];
	}

	// Initialize video-subsystem and font
	if(!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("Couldn't initialize SDL: %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (!TTF_Init()) {
		SDL_Log("Couldn't initialize TTF: %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	// Create window and renderer
	SDL_Window *window = NULL;
	SDL_Renderer *renderer = NULL;

	if(!SDL_CreateWindowAndRenderer("Text Editor", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
		SDL_Log("Couldn't create window and renderer : %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

		
	// Set Global Variables	
	Variables *vars = malloc(sizeof(Variables));

	vars->window = window;
	vars->renderer = renderer;
	// Load standard font
	if(load_font_from_file(vars, "./fonts/Roboto_Mono/RobotoMono-VariableFont_wght.ttf", 20) == 1) return SDL_APP_FAILURE;
	vars->font_scale = 1;
	vars->file_path = file_path;	

	vars->editor = (Editor) {0};
	editor_init(&vars->editor);

	vars->camera_pos = (Vec2f) {0};
	vars->camera_vel = (Vec2f) {0};
		
	*appstate = vars;
	
	// Load file
	if (vars->file_path) {
		FILE *f = fopen(file_path, "r");
		if (f) {		
			editor_load_from_file(&vars->editor, f);
			fclose(f);
		}
		
		else {
			editor_insert_new_line(&vars->editor);
		}
	}
	
	else {
		editor_insert_new_line(&vars->editor);
	}

	// Enable text input 
	if (!SDL_StartTextInput(vars->window)) {
		SDL_Log("Couldn't enable text input: %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
	
	Variables *vars = (Variables *) appstate;
	
	switch(event->type) {
		
		case SDL_EVENT_QUIT: {
			return SDL_APP_SUCCESS;		     
		} break;

		case SDL_EVENT_KEY_DOWN: {
			switch (event->key.key) {
				
				case SDLK_ESCAPE: {
					return SDL_APP_SUCCESS;
				} break;

				case SDLK_BACKSPACE: {
					editor_backspace(&vars->editor);
				} break;

				/* To be implemented
				case SDLK_PAGEDOWN: {
					editor_delete_line(&vars->editor, &vars->editor.cursor_row);
				} break;
				*/

				case SDLK_F2: {
					if (vars->file_path) {
						editor_save_to_file(&vars->editor, vars->file_path);
					}
				} break;

				case SDLK_DELETE: {
					editor_delete(&vars->editor);
				} break;
				
				case SDLK_TAB: {
					editor_insert_text_before_cursor(&vars->editor, "    ");
				} break;

				case SDLK_RETURN: {
					editor_insert_new_line(&vars->editor);
				} break;

				case SDLK_UP: {
					if (vars->editor.cursor_row > 0) {
						vars->editor.cursor_row -= 1;

						if (vars->editor.cursor_col >= vars->editor.lines[vars->editor.cursor_row + 1].size) {
							vars->editor.cursor_col = vars->editor.lines[vars->editor.cursor_row].size;
						}
					}
				} break;

				case SDLK_DOWN: {
					if (vars->editor.cursor_row + 1 < vars->editor.size) {
						vars->editor.cursor_row += 1;

						if (vars->editor.cursor_col == vars->editor.lines[vars->editor.cursor_row - 1].size) {
							vars->editor.cursor_col = vars->editor.lines[vars->editor.cursor_row].size;
						}

						else if (vars->editor.cursor_col >= vars->editor.lines[vars->editor.cursor_row].size) {
							vars->editor.cursor_col = vars->editor.lines[vars->editor.cursor_row].size;
						}
					}
				} break;

				case SDLK_LEFT: {
					if (vars->editor.cursor_col > 0) {
						vars->editor.cursor_col -= 1;
					}		
				} break;

				case SDLK_RIGHT: {
					if (vars->editor.cursor_col < vars->editor.lines[vars->editor.cursor_row].size) {
						vars->editor.cursor_col += 1;
					}
				} break;

				case SDLK_HOME: {
					vars->editor.cursor_col = 0;
				} break;
				
				case SDLK_END: {
					vars->editor.cursor_col = vars->editor.lines[vars->editor.cursor_row].size;
				} break;
			}
		} break;

		case SDL_EVENT_TEXT_INPUT: {
			editor_insert_text_before_cursor(&vars->editor, event->text.text);
		} break;
			
	}


	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
	
	const Uint32 start = SDL_GetTicks();

	Variables *vars = (Variables *) appstate;

	{
		int cursor_x = 0;
		int dummy_h = 0;
		Line *line = &vars->editor.lines[vars->editor.cursor_row];

		if (line->chars != NULL && vars->editor.cursor_col > 0) {
			TTF_GetStringSize(vars->font.font, line->chars, vars->editor.cursor_col, &cursor_x, &dummy_h);
		}

		const Vec2f cursor_pos = vec2f((float) cursor_x, (float) vars->editor.cursor_row * (float) vars->font.char_h);

		Vec2f target_pos = vars->camera_pos;
		target_pos.x = (window_size(vars->window).x / 2.0f) - 50.0f;

		vars->camera_vel = vec2f_mul(vec2f_sub(target_pos, vars->camera_pos), vec2fs(15.0f));

		vars->camera_pos = vec2f_add(vars->camera_pos, vec2f_mul(vars->camera_vel, vec2fs(DELTA_TIME)));
	}

	SDL_SetRenderDrawColor(vars->renderer, 30, 30, 30, 255);

	SDL_RenderClear(vars->renderer);
	
	SDL_Color color = {
		.r = 0xFF,
		.g = 0xFF,
		.b = 0xFF,
		.a = 0xFF
	};

	for (size_t row = 0; row < vars->editor.size; row++) {	
		Line *line = &vars->editor.lines[row];

		const Vec2f line_pos = camera_project_point(vars, vec2f(0.0f, (float) row * vars->font.char_h * vars->font_scale));

		if (line->chars != NULL && line->size > 0 ) {
			render_text(vars->renderer, vars->font, line->chars, line_pos, color, vars->font_scale);
		}
	}

	render_cursor(vars);

	SDL_RenderPresent(vars->renderer);

	const Uint32 duration = SDL_GetTicks() - start;
	const Uint32 delta_time_ms = 1000 / FPS;

	if (duration < delta_time_ms) {
		SDL_Delay(delta_time_ms - duration);
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
	
	Variables *vars = (Variables *) appstate;

	(void) result;

	if(vars) {
		if(vars->window) {
			SDL_StopTextInput(vars->window);
			SDL_DestroyWindow(vars->window);
		}
		
		if(vars->renderer) {
			SDL_DestroyRenderer(vars->renderer);
		}

		if(vars->font.font) {	
			TTF_CloseFont(vars->font.font);
			TTF_Quit();
		}

		free(vars);
	}

	SDL_Quit();
}



