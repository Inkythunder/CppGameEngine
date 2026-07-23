#include "open_gl.h"
#include "stb_image.h"
#include <iostream>

GLTexture::GLTexture(const std::filesystem::path& texture_path) {
    GLuint texture;
    glGenTextures(1, &texture);

    std::cout << "Loading texture " << texture << ": " << texture_path << std::endl;

    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    stbi_set_flip_vertically_on_load(true);

    int width, height, nrChannels;
    unsigned char* texture_data = stbi_load(texture_path.c_str(), &width, &height, &nrChannels, 0);
    if(!texture_data) {
        throw std::runtime_error("stbi_load failed to load image");
    }

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, nrChannels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, texture_data
    );
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(texture_data);

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
    Vec3 pos,
    quat rot,
    float scale,
    std::shared_ptr<GLModel> model
): pos(pos), scale(scale), model(model) {
    memcpy(this->rot, rot, sizeof(quat));
}

void Entity::convert_to_renderables(std::vector<RenderableThing>& output) {

    mat4x4 m, rotation;
    mat4x4_from_quat(rotation, this->rot);
    mat4x4_translate(m, this->pos.x, this->pos.y, this->pos.z);
    mat4x4_mul(m, m, rotation);
    mat4x4_scale_aniso(m, m, scale, scale, scale);

    for(size_t i = 0; i < this->model->meshes.size(); i++) {

        RenderableThing thing(this->model->meshes[i], m);
        output.push_back(thing);
    }
}

void Entity::set_pos(Vec3 pos) {
    this->pos = pos;
}

void Entity::set_rot(quat rot) {
    memcpy(this->rot, rot, sizeof(quat));
}
