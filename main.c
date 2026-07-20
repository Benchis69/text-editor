#include <stdio.h>
#include <raylib.h>

int main(void) {

	InitWindow(800, 450, "text editor");

	SetTargetFPS(60);

	while (!WindowShouldClose()) {
		
		BeginDrawing();

		ClearBackground(BLACK);

		EndDrawing();
	}
}
