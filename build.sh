#!/bin/bash

set -e

gcc -Wall -Wextra main.c buffer.c la.c -o main $(pkg-config --cflags --libs sdl3) -lSDL3_ttf -lm
