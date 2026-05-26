#!/bin/bash

set -e
g++ --std=c++23 -I include -lglfw main.cpp obj_loader.cpp open_gl.cpp gl.c -o proj -g
