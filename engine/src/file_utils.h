#pragma once
#include <filesystem>
#include <iostream>

std::string get_shader_content(const std::filesystem::path& filename);

int get_shader_compile_error(GLuint shader);
