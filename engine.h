#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdint>
#include <filesystem>

typedef uint64_t EntityId;
typedef uint64_t ModelId;
typedef uint64_t TextureId;

static const ModelId MODEL_ID_PLANE = 1;
static const ModelId MODEL_ID_CUBE = 2;
static const ModelId MODEL_ID_SPHERE = 3;
static const TextureId TEXTURE_ID_NONE = 0;

struct Offset{
    float x;
    float y;
};

struct LoadedTexture {
    TextureId texture_id;
    int width;
    int height;

    LoadedTexture(
        TextureId texture_id,
        int width,
        int height): texture_id(texture_id), width(width), height(height) {}
};

struct TextureUvs {
    glm::vec2 top_left;
    glm::vec2 top_right;
    glm::vec2 bottom_left;
    glm::vec2 bottom_right;
};

class EngineCallbacks {
public:
    virtual void on_game_start() = 0;

    virtual void on_frame_start() = 0;

    virtual void on_key_event(int key, int action) = 0;

    virtual void on_mouse_move() = 0;

    virtual void on_mouse_button_change() = 0;

    virtual ~EngineCallbacks() = default;

    virtual void on_window_resolution_change(glm::vec2 res) = 0;
};

void engine_start(uint16_t window_width, uint16_t window_height, EngineCallbacks& callbacks);

ModelId engine_model_load(const std::filesystem::path& path);

EntityId engine_entity_create(ModelId model, glm::vec3 position, glm::quat rotation, glm::vec3 scale);

void engine_entity_set_position(EntityId entity, glm::vec3 position);

void engine_entity_set_rotation(EntityId entity, glm::quat rotation);

void engine_set_camera(glm::vec3 position, glm::quat rotation);

bool engine_is_key_down(int key);

glm::vec2 engine_get_mouse_position();

void engine_run_loop();

void engine_stop();

void engine_ui_draw_rect(
    glm::vec2 position,
    glm::vec2 dimensions,
    glm::vec4 colour,
    TextureId texture,
    TextureUvs uvs
);

LoadedTexture engine_load_texture_from_file(const std::filesystem::path& path);

LoadedTexture engine_load_texture_from_bytes(const uint8_t* texture_data, int width, int height, int nrChannels);

void engine_display_text(const char* text, glm::vec2 position, float scale);
