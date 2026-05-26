#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdlib.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include "obj_loader.h"
#include "open_gl.h"

struct Offset{
    float x;
    float y;
};

static void error_callback(int error, const char* description) {
    std::cerr << "Error: " << description << std::endl;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    Offset* offset_ptr = static_cast<Offset*>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
        offset_ptr->y += 0.1f;
    }

    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
        offset_ptr->y -= 0.1f;
    }

    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
        offset_ptr->x -= 0.1f;
    }

    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
        offset_ptr->x += 0.1f;
    }
}

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

void GLdraw_call(RenderableThing& thing) {
    thing.mesh->bind();
    thing.texture->bind();

    glUniform3f(
        thing.uniformLocs.ka_location,
        thing.material.light_ambient.x,
        thing.material.light_ambient.y,
        thing.material.light_ambient.z
    );
    glUniform3f(
        thing.uniformLocs.kd_location,
        thing.material.light_diffuse.x,
        thing.material.light_diffuse.y,
        thing.material.light_diffuse.z
    );

    glDrawArrays(GL_TRIANGLES, 0, thing.mesh->get_vertex_count());
}

int main(int argc, char* argv[]) {

    vec3 point_light = {0, 30, 50};

    if(argc < 2) {
        std::println(std::cerr, "No object file provided.");
        return 1;
    }

    std::map<std::string, ObjMaterial> materials;
    std::cout << "argv[1] = " << argv[1] << std::endl;
    ObjModel model = loadObjFile(argv[1], materials);

    // Loads the vertex and fragment shader from the glsl files I created.
    const std::string vertex_shader_text = get_shader_content("vertex.glsl");
    const std::string fragment_shader_text = get_shader_content("fragment.glsl");

    // Displays any errors in a human readable form
    glfwSetErrorCallback(error_callback);

    // You just have to call this at the start and terminate at the end
    if (!glfwInit()) exit(EXIT_FAILURE);

    // Sets the window hints for the next createwindow call
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Creates a window with the specified width, height, and name
    GLFWwindow* window = glfwCreateWindow(640, 480, "OpenGL C++", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    // makes an "offset" which I can move the triangle with
    Offset offset = {0.f, 0.f};
    glfwSetWindowUserPointer(window, &offset);
    // sets a function as the callback for a key press. Esc will close the window.
    glfwSetKeyCallback(window, key_callback);

    // Makes it so all the OpenGL code happens in this window
    glfwMakeContextCurrent(window);

    // can't find it anywhere
    gladLoadGL(glfwGetProcAddress);

    // vsync
    glfwSwapInterval(1);

    // Creates a shader object
    const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    // Sets the source code for the shader
    const GLchar* shader_text_c_str = vertex_shader_text.c_str();
    glShaderSource(vertex_shader, 1, &shader_text_c_str, nullptr);
    // Compiles the shader
    glCompileShader(vertex_shader);
    // same as above but for fragment shader
    const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar* fragment_shader_text_c_str = fragment_shader_text.c_str();
    glShaderSource(fragment_shader, 1, &fragment_shader_text_c_str, nullptr);
    glCompileShader(fragment_shader);

    if(!get_shader_compile_error(vertex_shader) || !get_shader_compile_error(fragment_shader)) {
        return 1;
    }

    // Create's program
    const GLuint program = glCreateProgram();
    // Use below 2 shaders in above program
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    // links it all together I think ???
    glLinkProgram(program);
    GLint isLinked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &isLinked);

    if (isLinked == GL_FALSE) {
        GLint maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> msg;
        msg.resize(maxLength);
        if(msg.empty()) {
            std::cout << "Empty msg" << std::endl;
            return 1;
        }
        glGetProgramInfoLog(program, maxLength, &maxLength, msg.data());
        std::cout << "Linking failed: " << msg.data() << std::endl;
        return 1;
    }

    // tells OpenGL which shader to use
    glUseProgram(program);

    // uniform variable called MVP (Model View Projection)
    const GLint vpos_location = glGetAttribLocation(program, "vPos");
    const GLint vtex_location = glGetAttribLocation(program, "vTex");
    const GLint vnorm_location = glGetAttribLocation(program, "vNorm");
    const GLint vp_location = glGetUniformLocation(program, "VP");
    const GLint model_location = glGetUniformLocation(program, "model");
    const GLint vpoint_location = glGetUniformLocation(program, "vPoint");
    const GLint ka_location = glGetUniformLocation(program, "ka");
    const GLint kd_location = glGetUniformLocation(program, "kd");

    GLAttributeLocations attribLocs(vpos_location, vtex_location, vnorm_location);
    GLUniformLocations uniformLocs(ka_location, kd_location);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    mat4x4 m, vp;
    // makes m an identity matrix which is all 0s except for a diagonal of 1s
    mat4x4_identity(m);

    mat4x4_translate_in_place(m, offset.x, offset.y - 1, -5.f);

    m[0][0] /= 300.0;
    m[1][1] /= 300.0;
    m[2][2] /= 300.0;
    //mat4x4_rotate_Y(m, m, (float) glfwGetTime()/2);
    mat4x4_rotate_X(m, m, -1.2f);

    std::vector<RenderableThing> list_of_things;

    for(std::map<std::string, ObjMesh>::iterator it = model.meshes.begin(); it != model.meshes.end(); it++) {

        ObjMaterial& material = materials[it->second.material_name];
        std::shared_ptr<GLTexture> texture = std::make_shared<GLTexture>(material.diffuse_texture);

        std::shared_ptr<GLMesh> mesh = std::make_shared<GLMesh>(it->second.vertices.data(), it->second.vertices.size(), attribLocs);

        RenderableThing thing(
            mesh,
            texture,
            material,
            m,
            uniformLocs
        );

        list_of_things.push_back(thing);
    }

    while (!glfwWindowShouldClose(window)) {
        point_light[0] = (float)cos(glfwGetTime()) * 50.f;
        point_light[2] = (float)sin(glfwGetTime()) * 50.f;
        glUniform3f(vpoint_location, point_light[0], point_light[1], point_light[2]);
        int width, height;
        // gets the number of pixels in the given window
        glfwGetFramebufferSize(window, &width, &height);
        const float ratio = width / (float) height;

        // set the viewport?
        // viewport is the rectangle that is actually displayed on the screen
        glViewport(0, 0, width, height);
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // perspective projection
        mat4x4_perspective(vp, 1.f, ratio, 0.1f, 100.f);

        // setting the uniform variable mvp_location to mvp(ram)
        glUniformMatrix4fv(vp_location, 1, GL_FALSE, (const GLfloat*) &vp);
        glUniformMatrix4fv(model_location, 1, GL_FALSE, (const GLfloat*) &m);

        for(size_t i = 0; i < list_of_things.size(); i++) {

            GLdraw_call(list_of_things[i]);
        }

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            printf("GL error: %d\n", err);
        }

        // brings the now finished frame to the foreground
        glfwSwapBuffers(window);
        // checks for "events" (mouse move, key press, etc)
        glfwPollEvents();
    }

    // cleans up the program at the end
    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
