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
	
	Editor editor;
} Variables;

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

	const Vec2f pos = vec2f((float) vars->editor.cursor_col * (float) vars->font.char_w, (float) vars->editor.cursor_row * (float) vars->font.char_h);

	const SDL_FRect rect =  {
		.x = (int) floorf(pos.x),
		.y = (int) floorf(pos.y), // maybe noch * vars->font_scale
		.w = 2 * vars->font_scale, // Cursor width is 2 pixel * font scale
		.h = vars->font.char_h * vars->font_scale
	};
	
	SDL_SetRenderDrawColor(vars->renderer, 0xFF, 0xFF, 0xFF, 0xFF); // Set render draw color to white
	SDL_RenderRect(vars->renderer, &rect);
	
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

bool load_font_from_file(void *appstate, const char *file_path, int size) {
	
	Variables *vars = (Variables *) appstate;

	int char_w, char_h;
	
	vars->font.font = TTF_OpenFont(file_path, size);
	if (!vars->font.font) {
		SDL_Log("Couldn't load font: %s\n", SDL_GetError());
		return false;
	}

	// Get values for char "A", works only for monospace fonts
	TTF_GetStringSize(vars->font.font, "A", 0,  &char_w, &char_h);
	vars->font.char_w = char_w;
	vars->font.char_h = char_h;

	return true;
}

void usage(FILE *stream) {

	fprintf(stream, "Usage: text editor [FILE-PATH]");
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char** argv) {

	const char *file_path = NULL;
	
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

	if(!SDL_CreateWindowAndRenderer("Text Editor", 1600, 900, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
		SDL_Log("Couldn't create window and renderer : %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	/* Create standard Font
	TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuMathTeXGyre.ttf", 20);
	if(!font) {
		SDL_Log("Couldn't load font: %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	} */
	
	// Set Global Variables	
	Variables *vars = malloc(sizeof(Variables));

	vars->window = window;
	vars->renderer = renderer;
	// Load standard font
	if(!load_font_from_file(vars, "./fonts/Space_Mono/SpaceMono-Regular.ttf", 20)) return SDL_APP_FAILURE;
	vars->font_scale = 1;
	vars->editor = (Editor) {0};
		
	*appstate = vars;
	
	// Load file
	if (file_path) {
		editor_load_from_file(&vars->editor, file_path);
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
				
				case SDLK_BACKSPACE: {
					editor_backspace(&vars->editor);
				} break;

				case SDLK_F2: {
					if (file_path) {
						editor_save_to_file(&vars->editor, file_path);
					}
				} break;

				case SDLK_DELETE: {
					editor_delete(&vars->editor);
				} break;

				case SDLK_RETURN: {
					editor_insert_new_line(&vars->editor);
				} break;

				case SDLK_UP: {
					if (vars->editor.cursor_row > 0) {
						vars->editor.cursor_row -= 1;
					}
				} break;

				case SDLK_DOWN: {
					vars->editor.cursor_row += 1;
				} break;

				case SDLK_LEFT: {
					if (vars->editor.cursor_col > 0) {
						vars->editor.cursor_col -= 1;
					}		
				} break;

				case SDLK_RIGHT: {
					vars->editor.cursor_col += 1;
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
	
	Variables *vars = (Variables *) appstate;

	SDL_SetRenderDrawColor(vars->renderer, 30, 30, 30, 255);

	SDL_RenderClear(vars->renderer);
	
	Vec2f pos = {0.0f, 0.0f};
	SDL_Color color = {
		.r = 0xFF,
		.g = 0xFF,
		.b = 0xFF,
		.a = 0xFF
	};

	for (size_t row = 0; row < vars->editor.size; row++) {
		pos = vec2f(0.0f, (float) row * vars->font.char_h * vars->font_scale);
		
		Line *line = &vars->editor.lines[row];
		if (line->chars != NULL && line->size > 0 ) {
			render_text(vars->renderer, vars->font, line->chars, pos, color, vars->font_scale);
		}
	}

	render_cursor(vars);

	SDL_RenderPresent(vars->renderer);

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



