cmake_minimum_required(VERSION 3.20)
project(opengl_engine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # gives you compile_commands.json for clangd

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

include(FetchContent)
  FetchContent_Declare(glm
      GIT_REPOSITORY https://github.com/g-truc/glm.git
      GIT_TAG        1.0.1
  )
  FetchContent_MakeAvailable(glm)

add_executable(proj
    main.cpp
    obj_loader.cpp
    open_gl.cpp
    engine.cpp
    gl.c
)
target_include_directories(proj PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(proj PRIVATE glfw OpenGL::GL glm::glm)
