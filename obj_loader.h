#pragma once

#include "linmath.h"
#include "utils.h"
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <iostream>
#include <filesystem>

struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
    uint32_t material_index; //index in Model::materials
};

struct Material {
    std::string name;
    Vec3 light_ambient;
    Vec3 light_diffuse;
    Vec3 light_specular;
    float specular_exponent;
    std::filesystem::path diffuse_texture;

    Material();
};

struct Mesh {
    std::string name;
    std::vector<Vertex> vertices;
};

struct Model {
    std::map<std::string, Mesh> meshes;
    std::vector<Material> materials;
};

Model loadObjFile(const std::filesystem::path& filename);
