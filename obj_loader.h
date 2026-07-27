#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "engine.h"
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <iostream>
#include <filesystem>

struct ObjVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct ObjMaterial {
    std::string name;
    glm::vec3 light_ambient;
    glm::vec3 light_diffuse;
    glm::vec3 light_specular;
    float specular_exponent;
    std::optional<std::filesystem::path> diffuse_texture;

    ObjMaterial();
};

struct ObjMesh {
    std::vector<ObjVertex> vertices;
    std::string material_name;
};

struct ObjModel {
    std::map<std::string, ObjMesh> meshes;
    glm::vec3 dimensions;
};

ObjModel loadObjFile(
    const std::filesystem::path& filename,
    std::map<std::string, ObjMaterial>& materials
);
