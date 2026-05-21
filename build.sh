#!/bin/bash

set -e
g++ --std=c++23 -I include -lglfw main.cpp obj_loader.cpp load_texture.cpp gl.c -o proj -g
