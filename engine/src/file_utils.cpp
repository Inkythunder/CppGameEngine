#include <fstream>
#include <glad/gl.h>
#include <vector>
#include "file_utils.h"

std::string get_shader_content(const std::filesystem::path& filename) {

    std::cout << "Opening shader " << filename <<std::endl;
    std::ifstream file(filename);
    if(!file) {
        std::cout << "Failed to open shader file" << std::endl;
        throw std::runtime_error("Opening shader file failed");
    }
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    std::ostringstream file_contents;
    file_contents << file.rdbuf();
    return file_contents.str();
}

int get_shader_compile_error(GLuint shader) {
    GLint isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if(isCompiled == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        std::vector<GLchar> errorLog;
        errorLog.resize(maxLength);
        if(errorLog.empty()) {
            std::cout << "Empty errorLog" << std::endl;
            return 1;
        }
        glGetShaderInfoLog(shader, maxLength, &maxLength, errorLog.data());

        std::cout << "Shader compile error: " << errorLog.data() << std::endl;

        // Provide the infolog in whatever manor you deem best.
        // Exit with failure.
        glDeleteShader(shader); // Don't leak the shader.
        return 0;
    }
    return 1;
}
