# Game Engine

C++ / OpenGL game engine library with demo app included.

<img src=screenshots/screenshot2.png width="400" alt="Screenshot of engine">

This project is currently a work in progress.

## Features

- Renders the scene using OpenGL
- GLSL shaders with lighting and texturing
- .obj file parser with material support
- Text rendering using a font atlas
- Keyboard and mouse input
- API for creating and modifying entities
- Callbacks for input events

## Dependencies

You need the following on your system:

- CMake >= 3.20
- GLFW 3
- GLM
- OpenGL development headers

Fedora:
```sh
sudo dnf install cmake gcc-c++ glfw-devel glm-devel mesa-libGL-devel
```

Debian / Ubuntu:
```sh
sudo apt install cmake g++ libglfw3-dev libglm-dev libgl-dev
```

## Building

```sh
cmake -S . -B build
cmake --build build -j
```

Debug build:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

The binary is output to `build/bin/game`.

## Running

```sh
cd build/bin
./game
```

Run from the directory containing the copied assets (`build/bin/`), otherwise asset loading will fail.

### Controls

| Key             | Action        |
|-----------------|---------------|
| `W` `A` `S` `D` | Move          |
| `Space`         | Move up       |
| `Left Ctrl`     | Move down     |
| Mouse           | Look          |
| `Esc`           | Quit          |
