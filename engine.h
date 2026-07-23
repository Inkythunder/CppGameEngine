#pragma once

#include "linmath.h"

#include <cstdint>
#include <filesystem>

typedef uint64_t EntityId;
typedef uint64_t ModelId;

struct Offset{
    float x;
    float y;
};

struct Vec3 {
    float x;
    float y;
    float z;

    Vec3(): x(0.0), y(0.0), z(0.0) {}

    Vec3(float x, float y, float z): x(x), y(y), z(z) {}

    Vec3 max(const Vec3& other) {
        return Vec3(
            std::max(x, other.x),
                    std::max(y, other.y),
                    std::max(z, other.z)
        );
    }

    Vec3 min(const Vec3& other) {
        return Vec3(
            std::min(x, other.x),
                    std::min(y, other.y),
                    std::min(z, other.z)
        );
    }

    Vec3 operator-(Vec3 other) {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }
    Vec3 operator+(Vec3 other) {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }
};

struct Vec2 {
    float x;
    float y;

    Vec2 operator*(float scalar) {
        return Vec2(x * scalar, y * scalar);
    }

    Vec2 operator/(float scalar) {
        return Vec2(x / scalar, y / scalar);
    }

    Vec2 operator+(Vec2 other) {
        return Vec2(x + other.x, y + other.y);
    }

    Vec2 operator-(Vec2 other) {
        return Vec2(x - other.x, y - other.y);
    }

    Vec2 operator*(Vec2 other) {
        return Vec2(x * other.x, y * other.y);
    }

    Vec2(
        float x,
         float y
    ): x(x), y(y) {}

    Vec2(): x(0.0), y(0.0) {}
};

class EngineCallbacks {
public:
    virtual void on_frame_start() = 0;

    virtual void on_key_event(int key, int action) = 0;

    virtual void on_mouse_move() = 0;

    virtual void on_mouse_button_change() = 0;

    virtual ~EngineCallbacks() = default;
};

void engine_start(uint16_t window_width, uint16_t window_height, EngineCallbacks& callbacks);

ModelId engine_model_load(const std::filesystem::path& path);

EntityId engine_entity_create(ModelId model, Vec3 position, quat rotation);

void engine_entity_set_position(EntityId entity, Vec3 position);

void engine_entity_set_rotation(EntityId entity, quat rotation);

void engine_set_camera(Vec3 position, quat rotation);

bool engine_is_key_down(int key);

void engine_run_loop();

void engine_stop();
