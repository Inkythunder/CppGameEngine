#include "open_gl.h"
#include "stb_image.h"
#include "engine.h"
#include "image.h"
#include <iostream>

GLTexture::GLTexture(const uint8_t* texture_data, int width, int height, int nrChannels) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,  0,  nrChannels == 3 ? GL_RGB : GL_RGBA,  width,  height,  0,  nrChannels == 3 ? GL_RGB : GL_RGBA,  GL_UNSIGNED_BYTE,  texture_data
    );
    glGenerateMipmap(GL_TEXTURE_2D);

    this->texture_id = texture;
}

void GLTexture::bind() {
    glBindTexture(GL_TEXTURE_2D, this->texture_id);
}

GLTexture::~GLTexture() {
    glDeleteTextures(1, &this->texture_id);
}

void GLMaterial::bind(GLUniformLocations& uniforms) {
    glUniform3f(
        uniforms.ka_location,
        this->ka.x,
        this->ka.y,
        this->ka.z
    );
    glUniform3f(
        uniforms.kd_location,
        this->kd.x,
        this->kd.y,
        this->kd.z
    );
    if(texture) {
        (*texture)->bind();
        glUniform1f(uniforms.textureless_location, 0);
    } else {
        glBindTexture(GL_TEXTURE_2D, 0);
        glUniform1f(uniforms.textureless_location, 1);
    }
}

GLMesh::GLMesh(
    const ObjVertex* vertices,
    size_t count,
    GLAttributeLocations attribLocs,
    std::shared_ptr<GLMaterial> material
): vertex_count(count), material(material) {

    GLuint vertex_array;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    GLuint vertex_buffer;
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ObjVertex) * count, vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(attribLocs.vpos_location);
    glVertexAttribPointer(attribLocs.vpos_location, 3, GL_FLOAT, GL_FALSE, sizeof(ObjVertex), (void*) offsetof(ObjVertex, position));

    glEnableVertexAttribArray(attribLocs.vnorm_location);
    glVertexAttribPointer(attribLocs.vnorm_location, 3, GL_FLOAT, GL_FALSE, sizeof(ObjVertex), (void*) offsetof(ObjVertex, normal));

    glEnableVertexAttribArray(attribLocs.vtex_location);
    glVertexAttribPointer(attribLocs.vtex_location, 2, GL_FLOAT, GL_FALSE, sizeof(ObjVertex), (void*) offsetof(ObjVertex, uv));

    this->vertex_buffer = vertex_buffer;
    this->vertex_array = vertex_array;
}

void GLMesh::bind(GLUniformLocations& uniforms) {
    glBindVertexArray(this->vertex_array);
    material->bind(uniforms);
}

GLMesh::~GLMesh() {

    glDeleteVertexArrays(1, &this->vertex_array);
    glDeleteBuffers(1, &this->vertex_buffer);
}

Entity::Entity(
    glm::vec3 pos,
    glm::quat rot,
    glm::vec3 scale,
    std::shared_ptr<GLModel> model
): model(model), pos(pos), rot(rot), scale(scale) {}

void Entity::convert_to_renderables(std::vector<RenderableThing>& output) {

    glm::mat4 model_transform = glm::translate(glm::mat4(1.f), pos) * glm::mat4_cast(rot) * glm::scale(glm::mat4(1.f), scale);

    for(size_t i = 0; i < this->model->meshes.size(); i++) {

        RenderableThing thing(this->model->meshes[i], model_transform);
        output.push_back(thing);
    }
}

void Entity::set_pos(glm::vec3 pos) {
    this->pos = pos;
}

void Entity::set_rot(glm::quat rot) {
    this->rot = rot;
}
