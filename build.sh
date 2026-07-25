#!/bin/bash

set -e

gcc -Wall -Wextra -o main main.c $(pkg-config --cflags --libs sdl3) -lSDL3_ttf -lm
