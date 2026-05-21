#pragma once

#include <map>
#include <glad/gl.h>
#include <filesystem>
#include "utils.h"

struct texture_uvs {
    Vec2 top_left;
    Vec2 bottom_right;

    texture_uvs(
        Vec2 top_left,
        Vec2 bottom_right
    ): top_left(top_left), bottom_right(bottom_right) {}

    texture_uvs() {}
};

struct combined_texture_type {
    GLuint combined_texture;
    std::map<size_t, texture_uvs> material_index_to_uvs;
};

combined_texture_type load_texture(std::vector<std::filesystem::path> textures);
