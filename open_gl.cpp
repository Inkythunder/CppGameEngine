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

GLMesh::GLMesh(
    const ObjVertex* vertices,
    size_t count,
    GLAttributeLocations attribLocs
): vertex_count(count) {

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

void GLMesh::bind() {

    glBindVertexArray(this->vertex_array);
}

GLMesh::~GLMesh() {

    glDeleteVertexArrays(1, &this->vertex_array);
    glDeleteBuffers(1, &this->vertex_buffer);
}
