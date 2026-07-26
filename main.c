#define SDL_MAIN_USE_CALLBACKS 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef __ANDROID__
#undef __ANDROID__
#define SDL_PLATFORM_LINUX 1
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "./la.h"

#define BUFFER_CAPACITY 1024

typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	TTF_Font  *font;
	int font_scale;
	char buffer[BUFFER_CAPACITY];
	size_t buffer_cursor;
	size_t buffer_size;
} Variables;

void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, Vec2f pos, SDL_Color color, float scale) {
	
	// Convert text to surface
	SDL_Surface *surface = TTF_RenderText_Blended(font, text, 0, color);

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

	int char_w = 0;
	int char_h = 0;

	// Get values for char "A", works only for monospace fonts
	TTF_GetStringSize(vars->font, "A", 0,  &char_w, &char_h);

	const SDL_FRect rect =  {
		.x = (int) floorf(vars->buffer_cursor * (float) char_w ),
		.y = 0,
		.w = 2 * vars->font_scale, // Cursor width is 2 pixel * font scale
		.h = char_h * vars->font_scale
	};
	
	SDL_SetRenderDrawColor(vars->renderer, 0xFF, 0xFF, 0xFF, 0xFF); // Set render draw color to white
	SDL_RenderRect(vars->renderer, &rect);
}

bool load_font_from_file(void *appstate, const char *file_path, int size) {
	
	Variables *vars = (Variables *) appstate;
	
	vars->font = TTF_OpenFont(file_path, size);
	if (!vars->font) {
		SDL_Log("Couldn't load font: %s\n", SDL_GetError());
		return false;
	}

	return true;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char** argv) {

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
	vars->buffer[0] = '\0';
	vars->buffer_size = 0;
	vars->buffer_cursor = 0;
		
	*appstate = vars;

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
					if(vars->buffer_size > 0) {
						vars->buffer_size--;
						vars->buffer[vars->buffer_size] = '\0';
						vars->buffer_cursor = vars->buffer_size;
					}
				} break;
			}
		} break;

		case SDL_EVENT_TEXT_INPUT: {
			size_t text_size = strlen(event->text.text);
			const size_t free_space = BUFFER_CAPACITY - vars->buffer_size - 1;
			
			if(text_size > free_space) {
				text_size = free_space;
			}

			memcpy(vars->buffer + vars->buffer_size, event->text.text, text_size); 
			vars->buffer_size += text_size; 
			vars->buffer_cursor = vars->buffer_size;

			vars->buffer[vars->buffer_size] = '\0';

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

	if(vars->buffer_size > 0) {
		render_text(vars->renderer, vars->font, vars->buffer, pos, color, vars->font_scale);
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

		if(vars->font) {	
			TTF_CloseFont(vars->font);
			TTF_Quit();
		}

		free(vars);
	}

	SDL_Quit();
}



