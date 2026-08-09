#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <map>

#include "engine/engine.h"
#include "open_gl.h"
#include "image.h"
#include "file_utils.h"
#include "standard_shapes.h"


struct UIDrawCall {
    TextureId texture;
    glm::vec4 colour;
    size_t first_vertex;
    size_t vertex_count;
};

struct EngineState {
    GLFWwindow* window;

    GLAttributeLocations attribLocs;
    GLUniformLocations uniformLocs;

    std::map<ModelId, std::shared_ptr<GLModel>> models;
    std::map<EntityId, Entity> entities;
    std::map<std::string, ObjMaterial> materials;
    std::map<TextureId, std::shared_ptr<GLTexture>> textures;

    std::vector<ObjVertex> ui_vertices;
    GLuint ui_vao, ui_vbo;
    //size_t ui_vbo_capacity = 0;
    std::optional<LoadedTexture> font_atlas;

    ModelId current_model_id = 0;
    EntityId current_entity_id = 0;
    TextureId current_texture_id = 0;

    glm::vec3 camera_position;
    glm::quat camera_angle = {1.f, 0.f, 0.f, 0.f};

    EngineCallbacks* engine_callbacks = nullptr;

    std::vector<UIDrawCall> ui_calls;

    void build_model(ModelId id, const std::vector<ObjVertex>& vertices, glm::vec3 dimensions);

    void create_standard_models(glm::vec3 dimensions);

    EngineState(uint16_t window_width, uint16_t window_height, const char* window_title, EngineCallbacks& callbacks);
    ~EngineState();
};

static EngineState* state = nullptr;

static void error_callback(int error, const char* description) {
    std::cerr << "Error: " << description << std::endl;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    state->engine_callbacks->on_key_event(key, action);
}

EngineState::EngineState(uint16_t window_width, uint16_t window_height, const char* window_title, EngineCallbacks& callbacks) {

    const std::string vertex_shader_text = get_shader_content("vertex.glsl");
    const std::string fragment_shader_text = get_shader_content("fragment.glsl");

    engine_callbacks = &callbacks;

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(window_width, window_height, window_title, nullptr, nullptr);
    if (!window) {
        std::cout << "Error: " << window << "\n Window could not be initialised." << std::endl;
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

    attribLocs = GLAttributeLocations(program);
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
    // Set standard blending function
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    create_standard_models(glm::vec3(2, 0, 2));
}

void EngineState::build_model(ModelId id, const std::vector<ObjVertex>& vertices, glm::vec3 dimensions) {

    std::shared_ptr<GLMaterial> material = std::make_shared<GLMaterial>(std::nullopt, glm::vec3(0.8f), glm::vec3(0.2f));
    std::shared_ptr<GLModel> model = std::make_shared<GLModel>();

    model->meshes.push_back(std::make_shared<GLMesh>(vertices.data(), vertices.size(), attribLocs, material));
    model->dimensions = dimensions;

    models[id] = model;
}

void EngineState::create_standard_models(glm::vec3 dimensions) {

    std::vector<ObjVertex> vertices;

    construct_plane(vertices);
    build_model(MODEL_ID_PLANE, vertices, glm::vec3(2, 0, 2));
    vertices.clear();

    construct_cube(vertices);
    build_model(MODEL_ID_CUBE, vertices, glm::vec3(2, 2, 2));
    vertices.clear();

    // The standard models consume some of the possible IDs. The highest ID used by one of the
    // standard models is MODEL_ID_SPHERE.
    current_model_id = MODEL_ID_SPHERE;
}

EngineState::~EngineState() {
    entities.clear();
    models.clear();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void engine_start(uint16_t window_width, uint16_t window_height, const char* window_title, EngineCallbacks& callbacks) {

    // called when the game starts to set up all the opengl stuff
    state = new EngineState(window_width, window_height, window_title, callbacks);
}

ModelId engine_model_load(const std::filesystem::path& path) {

    state->current_model_id++;
    ObjModel model = loadObjFile(path, state->materials);
    std::shared_ptr<GLModel> gl_model = std::make_shared<GLModel>();

    for(std::map<std::string, ObjMesh>::iterator it = model.meshes.begin(); it != model.meshes.end(); it++) {

        ObjMaterial& material = state->materials[it->second.material_name];
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
            state->attribLocs,
            glMat
        );

        gl_model->meshes.push_back(mesh);
    }

    gl_model->dimensions = model.dimensions;

    state->models[state->current_model_id] = gl_model;
    return state->current_model_id;
}

EntityId engine_entity_create(ModelId model, glm::vec3 position, glm::quat rotation, glm::vec3 scale) {

    state->current_entity_id++;
    std::shared_ptr<GLModel> gl_model = state->models[model];

    Entity entity(position, rotation, scale, gl_model);

    state->entities.insert({state->current_entity_id, entity});
    return state->current_entity_id;
}

void engine_entity_set_position(EntityId entity, glm::vec3 position) {

    state->entities.at(entity).set_pos(position);
}

void engine_entity_set_rotation(EntityId entity, glm::quat rotation) {

    state->entities.at(entity).set_rot(rotation);
}

void engine_set_camera(glm::vec3 position, glm::quat rotation) {

    state->camera_position = position;
    state->camera_angle = rotation;
}

bool engine_is_key_down(int key) {
    return glfwGetKey(state->window, key) == GLFW_PRESS;
}

glm::vec2 engine_get_mouse_position() {
    double x, y;
    glfwGetCursorPos(state->window, &x, &y);
    return glm::vec2((float)x, (float)y);
}

void engine_run_loop() {

    std::vector<RenderableThing> renderable_thing_list;

    glm::vec3 point_light(0, 30, 50);
    glm::mat4x4 vp;
    int width = 0, height = 0;

    state->font_atlas = engine_load_texture_from_file("font_atlas.png");
    state->engine_callbacks->on_game_start();

    while (!glfwWindowShouldClose(state->window)) {
        // check if the window has changed size since last frame
        int temp_width, temp_height;
        glfwGetFramebufferSize(state->window, &temp_width, &temp_height);
        if(temp_width != width || temp_height != height) {
            width = temp_width;
            height = temp_height;
            state->engine_callbacks->on_window_resolution_change(glm::vec2(width, height));
        }
        state->engine_callbacks->on_frame_start();
        glBindBuffer(GL_ARRAY_BUFFER, state->ui_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(ObjVertex) * state->ui_vertices.size(), state->ui_vertices.data(), GL_STREAM_DRAW);

        // TODO Point light should be made in the game, not as part of the engine itself.
        point_light[0] = (float)cos(glfwGetTime()) * 50.f;
        point_light[2] = (float)sin(glfwGetTime()) * 50.f;
        glUniform3f(state->uniformLocs.vpoint_location, point_light[0], point_light[1], point_light[2]);

        const float ratio = (float) width / (float) height;

        // set the viewport
        // viewport is the rectangle that is actually displayed on the screen
        glViewport(0, 0, width, height);
        // Clear the screen

        // calculate camera location
        glm::mat4x4 projection, view;
        projection = glm::perspective(1.f, ratio, 0.1f, 100.f);

        // Turn the camera angle (quaternion) into a 4x4 matrix so that it can be multiplied with the camera position.
        // Invert the camera angle and camera position because we are moving the world, not the camera.
        view = glm::mat4_cast(glm::conjugate(state->camera_angle)) * glm::translate(glm::mat4(1.f), -(state->camera_position));
        // Combine the projection and view matrices.
        vp = projection * view;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // setting the uniform variable mvp_location to mvp(ram)
        glUniformMatrix4fv(state->uniformLocs.vp_location, 1, GL_FALSE, (const GLfloat*) &vp);

        renderable_thing_list.clear();
        for(std::map<EntityId, Entity>::iterator it = state->entities.begin(); it != state->entities.end(); it++) {
            it->second.convert_to_renderables(renderable_thing_list);
        }

        for(size_t i = 0; i < renderable_thing_list.size(); i++) {

            glUniformMatrix4fv(
                state->uniformLocs.model_location,
                1,
                GL_FALSE,
                (const GLfloat*) &renderable_thing_list[i].model_transform
            );
            renderable_thing_list[i].mesh->bind(state->uniformLocs);
            glDrawArrays(
                GL_TRIANGLES,
                0,
                renderable_thing_list[i].mesh->get_vertex_count()
            );
        }

        // UI drawing:
        // create new vp matrix for the UI with orthoganal projection
        glm::mat4 ui_vp = glm::ortho(0.f, (float)width, (float)height, 0.f, -1.f, 1.f);
        glm::mat4 ui_model(1.f);
        glUniformMatrix4fv(state->uniformLocs.vp_location, 1, GL_FALSE, (const GLfloat*)&ui_vp);
        glUniformMatrix4fv(state->uniformLocs.model_location, 1, GL_FALSE, (const GLfloat*)&ui_model);

        glBindTexture(GL_TEXTURE_2D, 0);
        glUniform1f(state->uniformLocs.textureless_location, 1.f);
        glUniform3f(state->uniformLocs.kd_location, 1.f, 1.f, 1.f);
        glUniform1f(state->uniformLocs.unlit_location, 1.f);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);

        glBindVertexArray(state->ui_vao);

        // Loop through all of the UIDrawCalls
        for(const UIDrawCall& call : state->ui_calls) {

            // send the colour from this draw call (all shapes using the same colour) to the shader
            glUniform3f(state->uniformLocs.kd_location, call.colour.r, call.colour.g, call.colour.b);
            glUniform1f(state->uniformLocs.alpha_location, call.colour.a);

            std::map<TextureId, std::shared_ptr<GLTexture>>::iterator it = state->textures.find(call.texture);
            if(it != state->textures.end()) {
                // if the texture in this call is in the textures map, bind it.
                it->second->bind();
                glUniform1f(state->uniformLocs.textureless_location, 0.f);
            } else {
                // if the texture in this call is not in the textures map, bind blank texture.
                glBindTexture(GL_TEXTURE_2D, 0);
                glUniform1f(state->uniformLocs.textureless_location, 1.f);
            }

            glDrawArrays(GL_TRIANGLES, call.first_vertex, call.vertex_count);
        }

        glUniform1f(state->uniformLocs.alpha_location, 1.f);
        glUniform1f(state->uniformLocs.unlit_location, 0.f);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        state->ui_calls.clear();
        state->ui_vertices.clear();

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            printf("GL error: %d\n", err);
        }

        // brings the now finished frame to the foreground
        glfwSwapBuffers(state->window);
        // checks for "events" (mouse move, key press, etc)
        glfwPollEvents();
    }
    // cleans up the program at the end
    renderable_thing_list.clear();
    delete state;
    state = nullptr;
}

void engine_stop() {
    glfwSetWindowShouldClose(state->window, GLFW_TRUE);
}

// creates vertices and adds them to a global vector (go to line 363)
void engine_ui_draw_rect(glm::vec2 position, glm::vec2 dimensions, glm::vec4 colour, TextureId texture, TextureUvs uvs) {

    const glm::vec3 n(0, 0, 1);
    const size_t first = state->ui_vertices.size();

    // set the positions of the four vertices in the rectangle
    ObjVertex top_left(glm::vec3(position.x, position.y, 0), n, uvs.top_left);
    ObjVertex top_right(glm::vec3(position.x + dimensions.x, position.y, 0), n, uvs.top_right);
    ObjVertex bottom_left(glm::vec3(position.x, position.y + dimensions.y, 0), n, uvs.bottom_left);
    ObjVertex bottom_right(glm::vec3(position.x + dimensions.x, position.y + dimensions.y, 0), n, uvs.bottom_right);

    // push vertices to the UI vertices array to form triangles going anti-clockwise and sharing a line
    state->ui_vertices.push_back(bottom_left);
    state->ui_vertices.push_back(top_right);
    state->ui_vertices.push_back(top_left);
    state->ui_vertices.push_back(bottom_left);
    state->ui_vertices.push_back(bottom_right);
    state->ui_vertices.push_back(top_right);

    // if this rectangle shares a texture with the previous one, increase the number of vertices that draw call
    // will draw by 6 (one rectangle). Else, make a new UIDrawCall and push it.
    if(!state->ui_calls.empty() &&
        state->ui_calls.back().texture == texture &&
        state->ui_calls.back().colour == colour) {

        state->ui_calls.back().vertex_count += 6;
    } else {
        state->ui_calls.push_back({texture, colour, first, 6});
    }
}

LoadedTexture engine_load_texture_from_file(const std::filesystem::path& path) {

    std::cout << "Loading texture : " << path << std::endl;
    StbiImage image(path);
    return engine_load_texture_from_bytes(
        image.texture_data,
        image.width,
        image.height,
        image.nrChannels
    );
}

LoadedTexture engine_load_texture_from_bytes(const uint8_t* texture_data, int width, int height, int nrChannels) {
    state->current_texture_id++;

    state->textures[state->current_texture_id] = std::make_shared<GLTexture>(texture_data, width, height, nrChannels);
    return LoadedTexture(
        state->current_texture_id,
        width,
        height
    );
}

void engine_display_text(const char* text, glm::vec2 position, float scale) {

    // Font atlas has 16 columns and 6 rows.
    constexpr int cols = 16.f;
    constexpr int rows = 6.f;

    size_t i = 0;
    while(text[i] != '\0') {

        // Ascii code of the char we're looking at minus 32 to allow us to iterate through.
        int code = text[i] - 32;

        float top = rows - (code / cols);
        float bottom = top - 1;
        float left = code % cols;
        float right = left + 1;

        TextureUvs uvs(
            glm::vec2(left / cols, top / rows),
            glm::vec2(right / cols, top / rows),
            glm::vec2(left / cols, bottom / rows),
            glm::vec2(right / cols, bottom / rows)
        );

        glm::vec2 pos(position.x + scale * i, position.y);
        glm::vec2 size(scale, scale);
        glm::vec4 colour(1.f);
        engine_ui_draw_rect(
            pos,
            size,
            colour,
            state->font_atlas->texture_id,
            uvs
        );
        i++;
    }
}

double engine_get_time() {
    return glfwGetTime();
}
