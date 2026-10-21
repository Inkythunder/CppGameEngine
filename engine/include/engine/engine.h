#pragma once
#include <engine/engine_export.h>

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

enum EngineKey : int {
    ENGINE_KEY_SPACE = 32,
    ENGINE_KEY_A = 65,
    ENGINE_KEY_D = 68,
    ENGINE_KEY_S = 83,
    ENGINE_KEY_W = 87,
    ENGINE_KEY_ESCAPE = 256,
    ENGINE_KEY_LEFT_CONTROL = 341,
};

enum EngineKeyAction : int {
    ENGINE_RELEASE = 0,
    ENGINE_PRESS = 1,
    ENGINE_REPEAT = 2
};

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

ENGINE_EXPORT void engine_start(uint16_t window_width, uint16_t window_height, const char* window_title, EngineCallbacks& callbacks);

ENGINE_EXPORT ModelId engine_model_load(const std::filesystem::path& path);

ENGINE_EXPORT EntityId engine_entity_create(ModelId model, glm::vec3 position, glm::quat rotation, glm::vec3 scale);

ENGINE_EXPORT void engine_entity_set_position(EntityId entity, glm::vec3 position);

ENGINE_EXPORT void engine_entity_set_rotation(EntityId entity, glm::quat rotation);

ENGINE_EXPORT void engine_set_camera(glm::vec3 position, glm::quat rotation);

ENGINE_EXPORT bool engine_is_key_down(int key);

ENGINE_EXPORT glm::vec2 engine_get_mouse_position();

ENGINE_EXPORT void engine_run_loop();

ENGINE_EXPORT void engine_stop();

ENGINE_EXPORT void engine_ui_draw_rect(
    glm::vec2 position,
    glm::vec2 dimensions,
    glm::vec4 colour,
    TextureId texture,
    TextureUvs uvs
);

ENGINE_EXPORT LoadedTexture engine_load_texture_from_file(const std::filesystem::path& path);

ENGINE_EXPORT LoadedTexture engine_load_texture_from_bytes(const uint8_t* texture_data, int width, int height, int nrChannels);

ENGINE_EXPORT void engine_display_text(const char* text, glm::vec2 position, float scale);

ENGINE_EXPORT double engine_get_time();

enum EngineBodyType: int {
    ENGINE_BODY_STATIC = 0,
    ENGINE_BODY_DYNAMIC = 1,
    ENGINE_BODY_KINEMATIC = 2,
};

enum EngineShapeType : int {
    ENGINE_SHAPE_BOX = 0,
    ENGINE_SHAPE_SPHERE = 1,
    ENGINE_SHAPE_PLANE = 2,
};

struct EngineShape {
    EngineShapeType type;
    glm::vec3 half_extents{0.5f};
    float radius = 0.5f;
};

inline EngineShape engine_shape_box(glm::vec3 half_extents) {
    return EngineShape{ENGINE_SHAPE_BOX, half_extents, 0.f};
}

inline EngineShape engine_shape_sphere(float radius) {
    return EngineShape{ENGINE_SHAPE_SPHERE, glm::vec3(0.f), radius};
}

inline EngineShape engine_shape_plane(float half_size) {
    return EngineShape(ENGINE_SHAPE_PLANE, glm::vec3(half_size, 0.f, half_size), 0.f);
}

ENGINE_EXPORT void engine_entity_enable_physics(EntityId entity, EngineShape shape, EngineBodyType type);

ENGINE_EXPORT void engine_entity_set_velocity(EntityId entity, glm::vec3 velocity);

ENGINE_EXPORT void engine_entity_add_impulse(EntityId entity, glm::vec3 impulse);

ENGINE_EXPORT void engine_physics_set_gravity(glm::vec3 gravity);














