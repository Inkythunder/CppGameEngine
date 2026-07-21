#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <map>

#include "engine.h"
#include "open_gl.h"

// static "window" global variable
// don't use window in main. it's all gonna go
// code is clay :)

// replace mesh with model in the api
// implement model load function to load a model :O
// it should save it to a global map of models. modelId -> model
// do same for create_entity

static GLFWwindow* window;
static GLAttributeLocations attribLocs;
static GLUniformLocations uniformLocs;

static std::map<ModelId, std::shared_ptr<GLModel>> models;
static std::map<EntityId, Entity> entities;

static ModelId current_model_id = 0;
static EntityId current_entity_id = 0;
static std::map<std::string, ObjMaterial> materials;

static Vec3 camera_position;
static quat camera_angle = {0.f, 0.f, 0.f, 1.f};

static EngineCallbacks* engine_callbacks = nullptr;

static int get_shader_compile_error(GLuint shader) {
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

static std::string get_shader_content(const std::filesystem::path& filename) {

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

static void error_callback(int error, const char* description) {
    std::cerr << "Error: " << description << std::endl;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

    engine_callbacks->on_key_event(key, action);
}

static void GLdraw_call(RenderableThing& thing, GLUniformLocations& uniforms) { // add parameter here for a const reference to uniformLocs
    thing.mesh->bind(uniforms);

    glDrawArrays(GL_TRIANGLES, 0, thing.mesh->get_vertex_count());
}

void engine_start(uint16_t window_width, uint16_t window_height, EngineCallbacks& callbacks) {

    // call when the game starts to set up all the opengl stuff, shaders, and anything else you can think of

    const std::string vertex_shader_text = get_shader_content("vertex.glsl");
    const std::string fragment_shader_text = get_shader_content("fragment.glsl");

    engine_callbacks = &callbacks;

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(window_width, window_height, "OpenGL C++", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1);

    const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    const GLchar* vertex_shader_text_c_str = vertex_shader_text.c_str();
    glShaderSource(vertex_shader, 1, &vertex_shader_text_c_str, nullptr);
    glCompileShader(vertex_shader);

    const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar* fragment_shader_text_c_str = fragment_shader_text.c_str();
    glShaderSource(fragment_shader, 1, &fragment_shader_text_c_str, nullptr);
    glCompileShader(fragment_shader);

    if(!get_shader_compile_error(vertex_shader) || !get_shader_compile_error(fragment_shader)) {
        return;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
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
            return;
        }
        glGetProgramInfoLog(program, maxLength, &maxLength, msg.data());
        std::cout << "Linking failed: " << msg.data() << std::endl;
        return;
    }

    glUseProgram(program);

    const GLint vpos_location = glGetAttribLocation(program, "vPos");
    const GLint vtex_location = glGetAttribLocation(program, "vTex");
    const GLint vnorm_location = glGetAttribLocation(program, "vNorm");

    attribLocs = GLAttributeLocations(vpos_location, vtex_location, vnorm_location);
    uniformLocs = GLUniformLocations(program);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // don't render the backs of polygons
    glEnable(GL_CULL_FACE);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);
}

ModelId engine_model_load(const std::filesystem::path& path) {

    current_model_id++;
    ObjModel model = loadObjFile(path, materials);
    std::shared_ptr<GLModel> gl_model = std::make_shared<GLModel>();

    for(std::map<std::string, ObjMesh>::iterator it = model.meshes.begin(); it != model.meshes.end(); it++) {

        ObjMaterial& material = materials[it->second.material_name];
        std::optional<std::shared_ptr<GLTexture>> texture;
        if(material.diffuse_texture) {
            texture = std::make_shared<GLTexture>(*material.diffuse_texture);
        }

        std::shared_ptr<GLMaterial> glMat = std::make_shared<GLMaterial>(
            std::move(texture),
            material.light_diffuse,
            material.light_ambient
        );

        std::shared_ptr<GLMesh> mesh = std::make_shared<GLMesh>(
            it->second.vertices.data(),
            it->second.vertices.size(),
            attribLocs,
            glMat
        );

        gl_model->meshes.push_back(mesh);
    }

    gl_model->dimensions = model.dimensions;

    models[current_model_id] = gl_model;
    return current_model_id;
}

EntityId engine_entity_create(ModelId model, Vec3 position, quat rotation) {

    current_entity_id++;
    std::shared_ptr<GLModel> gl_model = models[model];

    float model_scalar = std::max(
        gl_model->dimensions.x,
        std::max(
            gl_model->dimensions.y,
            gl_model->dimensions.z
        )
    );

    //
    Entity entity(position, rotation, 3.0/model_scalar, gl_model);

    entities.insert({current_entity_id, entity});
    return current_entity_id;
}

void engine_entity_set_position(EntityId entity, Vec3 position) {


}

void engine_entity_set_rotation(EntityId entity, quat rotation) {


}

void engine_set_camera(Vec3 position, quat rotation) {

    camera_position = position;
    memcpy(camera_angle, rotation, sizeof(quat));
}

void engine_run_loop() {

    std::vector<RenderableThing> list_of_things;

    vec3 point_light = {0, 30, 50};
    mat4x4 vp;

    while (!glfwWindowShouldClose(window)) {
        point_light[0] = (float)cos(glfwGetTime()) * 50.f;
        point_light[2] = (float)sin(glfwGetTime()) * 50.f;
        glUniform3f(uniformLocs.vpoint_location, point_light[0], point_light[1], point_light[2]);
        int width, height;
        // gets the number of pixels in the given window
        glfwGetFramebufferSize(window, &width, &height);
        const float ratio = width / (float) height;

        // set the viewport?
        // viewport is the rectangle that is actually displayed on the screen
        glViewport(0, 0, width, height);
        // Clear the screen

        // calculate camera location
        mat4x4 projection, view;
        mat4x4_perspective(projection, 1.f, ratio, 0.1f, 100.f);

        quat inverse_rotation;
        quat_conj(inverse_rotation, camera_angle);
        mat4x4_from_quat(view, inverse_rotation);
        mat4x4_translate_in_place(view, -camera_position.x, -camera_position.y, -camera_position.z);

        mat4x4_mul(vp, projection, view);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // setting the uniform variable mvp_location to mvp(ram)
        glUniformMatrix4fv(uniformLocs.vp_location, 1, GL_FALSE, (const GLfloat*) &vp);

        list_of_things.clear();
        for(std::map<EntityId, Entity>::iterator it = entities.begin(); it != entities.end(); it++) {
            it->second.convert_to_renderables(list_of_things);
        }

        for(size_t i = 0; i < list_of_things.size(); i++) {

            glUniformMatrix4fv(uniformLocs.model_location, 1, GL_FALSE, (const GLfloat*) &list_of_things[i].model_transform);
            GLdraw_call(list_of_things[i], uniformLocs);
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
    entities.clear();
    models.clear();
    list_of_things.clear();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void engine_stop() {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}
