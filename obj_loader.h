#pragma once

#include "linmath.h"
#include "engine.h"
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <iostream>
#include <filesystem>

struct ObjVertex {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
};

struct ObjMaterial {
    std::string name;
    Vec3 light_ambient;
    Vec3 light_diffuse;
    Vec3 light_specular;
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
    Vec3 dimensions;
};

ObjModel loadObjFile(
    const std::filesystem::path& filename,
    std::map<std::string, ObjMaterial>& materials
);
