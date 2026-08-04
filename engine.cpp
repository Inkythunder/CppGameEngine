#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <map>

#include "engine.h"
#include "open_gl.h"
#include "image.h"

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
static std::map<std::string, ObjMaterial> materials;
static std::map<TextureId, std::shared_ptr<GLTexture>> textures;

static std::vector<ObjVertex> ui_vertices;
static GLuint ui_vao, ui_vbo;
static size_t ui_vbo_capacity = 0;

static ModelId current_model_id = 0;
static EntityId current_entity_id = 0;
static TextureId current_texture_id = 0;

static glm::vec3 camera_position;
static glm::quat camera_angle = {1.f, 0.f, 0.f, 0.f};

static EngineCallbacks* engine_callbacks = nullptr;

static std::vector<UIDrawCall> ui_calls;

static void construct_plane(std::vector<ObjVertex>& vertices) {

    const glm::vec3 n(0, 1, 0);
    ObjVertex c0{ {-1, 0, -1}, n, {0, 1} };
    ObjVertex c1{ {-1, 0,  1}, n, {0, 0} };
    ObjVertex c2{ { 1, 0,  1}, n, {1, 0} };
    ObjVertex c3{ { 1, 0, -1}, n, {1, 1} };

    vertices.push_back(c0);
    vertices.push_back(c1);
    vertices.push_back(c2);
    vertices.push_back(c0);
    vertices.push_back(c2);
    vertices.push_back(c3);
};

static void construct_cube(std::vector<ObjVertex>& vertices) {

    glm::vec3 cube_corners[8] = {
        {-1, -1, -1}, // left top back
        {-1, -1, 1}, // left top front
        {-1, 1, -1}, // left bottom back
        {-1, 1, 1}, // left bottom front
        {1, -1, -1}, // right top back
        {1, -1, 1}, // right top front
        {1, 1, -1}, // right bottom back
        {1, 1, 1} // right bottom front
    };

    struct Face {
        glm::vec3 normal;
        int corners[4];
    };

    const Face faces[6] = {
        {{ 1,  0,  0}, {5, 4, 6, 7}}, // right
        {{-1,  0,  0}, {0, 1, 3, 2}}, // left
        {{ 0,  1,  0}, {3, 7, 6, 2}}, // top
        {{ 0, -1,  0}, {0, 4, 5, 1}}, // bottom
        {{ 0,  0,  1}, {1, 5, 7, 3}}, // front
        {{ 0,  0, -1}, {4, 0, 2, 6}}  // back
    };

    const glm::vec2 uvs[4] = { {0, 0}, {1, 0}, {1, 1}, {0, 1} };

    for(const Face& face : faces) {
        ObjVertex c0{ cube_corners[face.corners[0]], face.normal, uvs[0] };
        ObjVertex c1{ cube_corners[face.corners[1]], face.normal, uvs[1] };
        ObjVertex c2{ cube_corners[face.corners[2]], face.normal, uvs[2] };
        ObjVertex c3{ cube_corners[face.corners[3]], face.normal, uvs[3] };

        vertices.push_back(c0);
        vertices.push_back(c1);
        vertices.push_back(c2);
        vertices.push_back(c0);
        vertices.push_back(c2);
        vertices.push_back(c3);
    }

}

static void construct_sphere(std::vector<ObjVertex>& vertices) {


}

static void build_model(ModelId id, const std::vector<ObjVertex>& vertices, glm::vec3 dimensions) {

    std::shared_ptr<GLMaterial> material = std::make_shared<GLMaterial>(std::nullopt, glm::vec3(0.8f), glm::vec3(0.2f));
    std::shared_ptr<GLModel> model = std::make_shared<GLModel>();

    model->meshes.push_back(std::make_shared<GLMesh>(vertices.data(), vertices.size(), attribLocs, material));
    model->dimensions = dimensions;

    models[id] = model;
}

static void create_standard_models(glm::vec3 dimensions) {

    std::vector<ObjVertex> vertices;

    construct_plane(vertices);
    build_model(MODEL_ID_PLANE, vertices, glm::vec3(2, 0, 2));
    vertices.clear();

    construct_cube(vertices);
    build_model(MODEL_ID_CUBE, vertices, glm::vec3(2, 2, 2));
    vertices.clear();

    //construct_sphere(vertices);
    //build_model(MODEL_ID_SPHERE, vertices, glm::vec3(2, 2, 2));

    current_model_id = MODEL_ID_SPHERE;
}

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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported()) glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
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

    glGenVertexArrays(1, &ui_vao);
    glBindVertexArray(ui_vao);
    glGenBuffers(1, &ui_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);

    glEnableVertexAttribArray(attribLocs.vpos_location);
    glVertexAttribPointer(
        attribLocs.vpos_location, 3, GL_FLOAT, GL_FALSE, sizeof(ObjVertex), (void*)offsetof(ObjVertex, position)
    );
    glEnableVertexAttribArray(attribLocs.vnorm_location);
    glVertexAttribPointer(
        attribLocs.vnorm_location, 3, GL_FLOAT, GL_FALSE, sizeof(ObjVertex), (void*)offsetof(ObjVertex, normal)
    );
    glEnableVertexAttribArray(attribLocs.vtex_location);
    glVertexAttribPointer(
        attribLocs.vtex_location, 2, GL_FLOAT, GL_FALSE, sizeof(ObjVertex), (void*)offsetof(ObjVertex, uv)
    );

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // don't render the backs of polygons
    glEnable(GL_CULL_FACE);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    create_standard_models(glm::vec3(2, 0, 2));
    engine_callbacks->on_game_start();
}

ModelId engine_model_load(const std::filesystem::path& path) {

    current_model_id++;
    ObjModel model = loadObjFile(path, materials);
    std::shared_ptr<GLModel> gl_model = std::make_shared<GLModel>();

    for(std::map<std::string, ObjMesh>::iterator it = model.meshes.begin(); it != model.meshes.end(); it++) {

        ObjMaterial& material = materials[it->second.material_name];
        std::optional<std::shared_ptr<GLTexture>> texture;
        if(material.diffuse_texture) {
            StbiImage image(*material.diffuse_texture);
            texture = std::make_shared<GLTexture>(image.texture_data, image.width, image.height, image.nrChannels);
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

EntityId engine_entity_create(ModelId model, glm::vec3 position, glm::quat rotation, glm::vec3 scale) {

    current_entity_id++;
    std::shared_ptr<GLModel> gl_model = models[model];

    Entity entity(position, rotation, scale, gl_model);

    entities.insert({current_entity_id, entity});
    return current_entity_id;
}

void engine_entity_set_position(EntityId entity, glm::vec3 position) {

    entities.at(entity).set_pos(position);
}

void engine_entity_set_rotation(EntityId entity, glm::quat rotation) {

    entities.at(entity).set_rot(rotation);
}

void engine_set_camera(glm::vec3 position, glm::quat rotation) {

    camera_position = position;
    camera_angle = rotation;
}

bool engine_is_key_down(int key) {
    return glfwGetKey(window, key) == GLFW_PRESS;
}

glm::vec2 engine_get_mouse_position() {
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    return glm::vec2((float)x, (float)y);
}

void engine_run_loop() {

    std::vector<RenderableThing> list_of_things;

    glm::vec3 point_light(0, 30, 50);
    glm::mat4x4 vp;
    int width = 0, height = 0;

    while (!glfwWindowShouldClose(window)) {
        // check if the window has changed size since last frame
        int temp_width, temp_height;
        glfwGetFramebufferSize(window, &temp_width, &temp_height);
        if(temp_width != width || temp_height != height) {
            width = temp_width;
            height = temp_height;
            engine_callbacks->on_window_resolution_change(glm::vec2(width, height));
        }
        engine_callbacks->on_frame_start();
        // all rects are built - upload and bind all 2d vertices
        // make a new mvp matrix (identity)
        // draw ui
        glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(ObjVertex) * ui_vertices.size(), ui_vertices.data(), GL_STREAM_DRAW);

        point_light[0] = (float)cos(glfwGetTime()) * 50.f;
        point_light[2] = (float)sin(glfwGetTime()) * 50.f;
        glUniform3f(uniformLocs.vpoint_location, point_light[0], point_light[1], point_light[2]);

        const float ratio = width / (float) height;

        // set the viewport?
        // viewport is the rectangle that is actually displayed on the screen
        glViewport(0, 0, width, height);
        // Clear the screen

        // calculate camera location
        glm::mat4x4 projection, view;
        projection = glm::perspective(1.f, ratio, 0.1f, 100.f);

        view = glm::mat4_cast(glm::conjugate(camera_angle)) * glm::translate(glm::mat4(1.f), -camera_position);
        vp = projection * view;

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

        // UI drawing:
        // create new vp matrix for the UI with orthoganal projection
        glm::mat4 ui_vp = glm::ortho(0.f, (float)width, (float)height, 0.f, -1.f, 1.f);
        glm::mat4 ui_model(1.f);
        glUniformMatrix4fv(uniformLocs.vp_location, 1, GL_FALSE, (const GLfloat*)&ui_vp);
        glUniformMatrix4fv(uniformLocs.model_location, 1, GL_FALSE, (const GLfloat*)&ui_model);

        glBindTexture(GL_TEXTURE_2D, 0);
        glUniform1f(uniformLocs.textureless_location, 1.f);
        glUniform3f(uniformLocs.kd_location, 1.f, 1.f, 1.f);
        glUniform1f(uniformLocs.unlit_location, 1.f);

        glDisable(GL_DEPTH_TEST);

        glBindVertexArray(ui_vao);

        // Loop through all of the UIDrawCalls
        for(const UIDrawCall& call : ui_calls) {

            // send the colour from this draw call (all shapes using the same colour) to the shader
            glUniform3f(uniformLocs.kd_location, call.colour.r, call.colour.g, call.colour.b);
            glUniform1f(uniformLocs.alpha_location, call.colour.a);

            std::map<TextureId, std::shared_ptr<GLTexture>>::iterator it = textures.find(call.texture);
            if(it != textures.end()) {
                // if the texture in this call is in the textures map, bind it.
                it->second->bind();
                glUniform1f(uniformLocs.textureless_location, 0.f);
            } else {
                // if the texture in this call is not in the textures map, bind blank texture.
                glBindTexture(GL_TEXTURE_2D, 0);
                glUniform1f(uniformLocs.textureless_location, 1.f);
            }

            glDrawArrays(GL_TRIANGLES, 0, call.vertex_count);
        }

        glUniform1f(uniformLocs.alpha_location, 1.f);
        glUniform1f(uniformLocs.unlit_location, 0.f);
        glEnable(GL_DEPTH_TEST);

        ui_calls.clear();
        ui_vertices.clear();

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

// creates vertices and adds them to a global vector (go to line 363)
void engine_ui_draw_rect(glm::vec2 position, glm::vec2 dimensions, glm::vec4 colour, TextureId texture) {

    const glm::vec3 n(0, 0, 1);
    const size_t first = ui_vertices.size();

    // set the positions of the four vertices in the rectangle
    ObjVertex top_left(glm::vec3(position.x, position.y, 0), n, glm::vec2(0, 1));
    ObjVertex top_right(glm::vec3(position.x + dimensions.x, position.y, 0), n, glm::vec2(1, 1));
    ObjVertex bottom_left(glm::vec3(position.x, position.y + dimensions.y, 0), n, glm::vec2(0, 0));
    ObjVertex bottom_right(glm::vec3(position.x + dimensions.x, position.y + dimensions.y, 0), n, glm::vec2(1, 0));

    // push vertices to the UI vertices array to form triangles going anti-clockwise and sharing a line
    ui_vertices.push_back(bottom_left);
    ui_vertices.push_back(top_right);
    ui_vertices.push_back(top_left);
    ui_vertices.push_back(bottom_left);
    ui_vertices.push_back(bottom_right);
    ui_vertices.push_back(top_right);

    // if this rectangle shares a texture with the previous one, increase the number of vertices that draw call
    // will draw by 6 (one rectangle). Else, make a new UIDrawCall and push it.
    if(!ui_calls.empty() &&
        ui_calls.back().texture == texture &&
        ui_calls.back().colour == colour) {

        ui_calls.back().vertex_count += 6;
    } else {
        ui_calls.push_back({texture, colour, first, 6});
    }
}

TextureId engine_load_texture_from_file(const std::filesystem::path& path) {

    std::cout << "Loading texture : " << path << std::endl;
    StbiImage image(path);
    return engine_load_texture_from_bytes(
        image.texture_data,
        image.width,
        image.height,
        image.nrChannels
    );
}

TextureId engine_load_texture_from_bytes(const uint8_t* texture_data, int width, int height, int nrChannels) {
    current_texture_id++;

    textures[current_texture_id] = std::make_shared<GLTexture>(texture_data, width, height, nrChannels);
    return current_texture_id;
}
