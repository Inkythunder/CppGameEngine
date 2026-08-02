#pragma once

#include <glad/gl.h>
#include <filesystem>
#include "engine.h"
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

    GLAttributeLocations() {}
};

struct GLUniformLocations {
    GLint ka_location;
    GLint kd_location;
    GLint textureless_location;
    GLint vpoint_location;
    GLint vp_location;
    GLint model_location;
    GLint unlit_location;

    GLUniformLocations (
        GLuint program
    ):
        ka_location(glGetUniformLocation(program, "ka")),
        kd_location(glGetUniformLocation(program, "kd")),
        textureless_location(glGetUniformLocation(program, "textureless")),
        vpoint_location(glGetUniformLocation(program, "vPoint")),
        vp_location(glGetUniformLocation(program, "VP")),
        model_location(glGetUniformLocation(program, "model")),
        unlit_location(glGetUniformLocation(program, "unlit")){}

    GLUniformLocations() {}
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

class GLMaterial {
public:

    void bind(GLUniformLocations& uniforms);

    GLMaterial(
        std::optional<std::shared_ptr<GLTexture>> texture,
        glm::vec3 kd,
        glm::vec3 ka
    ): texture(texture), kd(kd), ka(ka) {}

private:
    std::optional<std::shared_ptr<GLTexture>> texture;
    glm::vec3 kd;
    glm::vec3 ka;
};

class GLMesh {
public:
    GLMesh(
        const ObjVertex* vertices,
        size_t count,
        GLAttributeLocations attribLocs,
        std::shared_ptr<GLMaterial> material
    );
    void bind(GLUniformLocations& uniforms);
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
    std::shared_ptr<GLMaterial> material;
};

struct RenderableThing {
    std::shared_ptr<GLMesh> mesh;
    glm::mat4x4 model_transform;

    RenderableThing(
        std::shared_ptr<GLMesh> mesh,
        glm::mat4x4 model_transform
    ): mesh(mesh), model_transform(model_transform) {}
};

struct GLModel {
public:
    std::vector<std::shared_ptr<GLMesh>> meshes;
    glm::vec3 dimensions;
};

class Entity {
public:
    Entity(glm::vec3 pos, glm::quat rot, glm::vec3 scale, std::shared_ptr<GLModel> model);
    void convert_to_renderables(std::vector<RenderableThing>& output);
    void set_pos(glm::vec3 pos);
    void set_rot(glm::quat rot);
    std::shared_ptr<GLModel> model;

private:
    glm::vec3 pos;
    glm::quat rot;
    glm::vec3 scale;
};

// change objloader to make a vector of meshes

/* 1. Unbind textures when there is no texture for a material
 * 2. When no texture bound, texColour needs to be white not black
 * To do this, have a uniform called textureless which is a float: 0 = texture, 1 = no texture
 * add textureless to texColour.rgb (done)
 * 3. in ObjMaterial, wrap path in std::option
 * if path is empty string, don't create GLTexture
 * 4. wrap texture in GLMaterial parameters in option to make optional
 *
 *
 * obj parser
*/
