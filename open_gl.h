#pragma once

#include <glad/gl.h>
#include <filesystem>
#include "utils.h"
#include "obj_loader.h"

struct GLAttributeLocations {
    GLint vpos_location;
    GLint vtex_location;
    GLint vnorm_location;

    GLAttributeLocations(
        GLint vpos_location,
        GLint vtex_location,
        GLint vnorm_location
    ): vpos_location(vpos_location), vtex_location(vtex_location), vnorm_location(vnorm_location) {}
};

struct GLUniformLocations {
    GLint ka_location;
    GLint kd_location;

    GLUniformLocations (
        GLint ka_location,
        GLint kd_location
    ): ka_location(ka_location), kd_location(kd_location) {}
};

class GLTexture {
public:
    // takes the file path to the texture and uploads it to the GPU. Saves texture as this->texture_id
    GLTexture(const std::filesystem::path& texture_path);
    void bind();
    ~GLTexture();
    GLTexture(const GLTexture&) = delete;
    GLTexture operator=(const GLTexture&) = delete;

private:
    GLuint texture_id;
};

class GLMesh {
public:
    GLMesh(const ObjVertex* vertices, size_t count, GLAttributeLocations attribLocs);
    void bind();
    ~GLMesh();
    GLMesh(const GLMesh&) = delete;
    GLMesh operator=(const GLMesh&) = delete;

    size_t get_vertex_count() {
        return vertex_count;
    }

private:
    size_t vertex_count;
    GLuint vertex_buffer;
    GLuint vertex_array;
};

struct RenderableThing {
    std::shared_ptr<GLMesh> mesh;
    std::shared_ptr<GLTexture> texture;
    ObjMaterial material;
    mat4x4 model_transform;
    GLUniformLocations uniformLocs;

    RenderableThing(
        std::shared_ptr<GLMesh> mesh,
        std::shared_ptr<GLTexture> texture,
        ObjMaterial material,
        mat4x4 model_transform,
        GLUniformLocations uniformLocs
    ): mesh(mesh), texture(texture), material(material), uniformLocs(uniformLocs) {
        memcpy(this->model_transform, model_transform, sizeof(mat4x4));
    }
};

// change objloader to make a vector of meshes
