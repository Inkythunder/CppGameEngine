#pragma once

#include <glad/gl.h>
#include <filesystem>
#include "engine/engine.h"
#include "obj_loader.h"
#include "image.h"

struct GLAttributeLocations {
    GLint vpos_location;
    GLint vtex_location;
    GLint vnorm_location;

    GLAttributeLocations(
        GLuint program
    ):
        vpos_location(glGetAttribLocation(program, "vPos")),
        vtex_location(glGetAttribLocation(program, "vTex")),
        vnorm_location(glGetAttribLocation(program, "vNorm")){}

    GLAttributeLocations():
        vpos_location(0),
        vtex_location(0),
        vnorm_location(0) {}
};

struct GLUniformLocations {
    GLint ka_location;
    GLint kd_location;
    GLint textureless_location;
    GLint vpoint_location;
    GLint vp_location;
    GLint model_location;
    GLint unlit_location;
    GLint alpha_location;

    GLUniformLocations (
        GLuint program
    ):
        ka_location(glGetUniformLocation(program, "ka")),
        kd_location(glGetUniformLocation(program, "kd")),
        textureless_location(glGetUniformLocation(program, "textureless")),
        vpoint_location(glGetUniformLocation(program, "vPoint")),
        vp_location(glGetUniformLocation(program, "VP")),
        model_location(glGetUniformLocation(program, "model")),
        unlit_location(glGetUniformLocation(program, "unlit")),
        alpha_location(glGetUniformLocation(program, "alpha")){}

    GLUniformLocations():
        ka_location(0),
        kd_location(0),
        textureless_location(0),
        vpoint_location(0),
        vp_location(0),
        model_location(0),
        unlit_location(0),
        alpha_location(0) {}
};

class GLTexture {
public:
    // takes the file path to the texture and uploads it to the GPU. Saves texture as this->texture_id
    GLTexture(const uint8_t* texture_data, int width, int height, int nrChannels);
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
    glm::vec3 get_pos();
    glm::quat get_rot();
    void set_pos(glm::vec3 pos);
    void set_rot(glm::quat rot);
    std::shared_ptr<GLModel> model;

private:
    glm::vec3 pos;
    glm::quat rot;
    glm::vec3 scale;
};
