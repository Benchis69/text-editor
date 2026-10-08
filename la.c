#include "./la.h"
#include <math.h>

Vec2f vec2f(float x, float y) {
	return (Vec2f) {
		.x = x,
		.y = y
	};
}

Vec2f vec2fs(float x) {
	return (Vec2f) {
		.x = x,
		.y = x
	};
}

Vec2f vec2f_add(Vec2f a, Vec2f b) {
	return vec2f(a.x + b.x, a.y + b.y);
}

Vec2f vec2f_sub(Vec2f a, Vec2f b) {
	return vec2f(a.x - b.x, a.y - b.y);
}

Vec2f vec2f_mul(Vec2f a, Vec2f b) {
	return vec2f(a.x * b.x, a.y * b.y);
}

Vec2f vec2f_div(Vec2f a, Vec2f b) {
	return vec2f(a.x / b.x, a.y / b.y);
}

float distance(Vec2f a, Vec2f b) {
	return (float) sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}
