#define SDL_MAIN_USE_CALLBACKS 1

#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "./la.h"

typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	TTF_Font  *font;
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

	// Create Font
	TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuMathTeXGyre.ttf", 20);
	if(!font) {
		SDL_Log("Couldn't load font: %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	
	// Set Global Variables	
	Variables *vars = malloc(sizeof(Variables));

	vars->window = window;
	vars->renderer = renderer;
	vars->font = font;

	*appstate = vars;

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
	
	Variables *vars = (Variables *) appstate;

	if (event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
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
	render_text(vars->renderer, vars->font, "Hello World!", pos, color, 1);

	SDL_RenderPresent(vars->renderer);

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
	printf("App Closing...\n");
}



