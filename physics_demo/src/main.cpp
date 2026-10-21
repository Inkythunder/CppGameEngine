#include <vector>
#include "engine/engine.h"


static const int KEY_F = 70;

static const glm::vec3 CAMERA_POSITION(0.f, 2.f, 12.f);
static const glm::quat IDENTITY(1.f, 0.f, 0.f ,0.f);

class PhysicsDemo : public EngineCallbacks {
public:
    void on_game_start() override {}

    void on_frame_start() override {

        glm::vec3 move(0.f);

        // WASD Movement
        if(engine_is_key_down(ENGINE_KEY_W)) move[2] -= speed;
        if(engine_is_key_down(ENGINE_KEY_A)) move[0] -= speed;
        if(engine_is_key_down(ENGINE_KEY_S)) move[2] += speed;
        if(engine_is_key_down(ENGINE_KEY_D)) move[0] += speed;
        if(engine_is_key_down(ENGINE_KEY_SPACE)) move[1] += speed;
        if(engine_is_key_down(ENGINE_KEY_LEFT_CONTROL)) move[1] -= speed;

        move = camera_angle * move;

        // Limits looking angle to above head and below feet.
        const float look_speed = 0.003f;
        const float pitch_limit = glm::radians(89.0f);

        glm::vec2 mouse = engine_get_mouse_position();
        if(first_mouse) {
            last_mouse = mouse;
            first_mouse = false;
        }
        glm::vec2 delta = mouse - last_mouse;
        last_mouse = mouse;

        camera_yaw -= delta.x * look_speed;
        camera_pitch -= delta.y * look_speed;

        if(camera_pitch > pitch_limit) camera_pitch = pitch_limit;
        if(camera_pitch < -pitch_limit) camera_pitch = -pitch_limit;

        glm::quat yaw_q = glm::angleAxis(camera_yaw, glm::vec3(0, 1, 0));
        glm::quat pitch_q = glm::angleAxis(camera_pitch, glm::vec3(1, 0, 0));
        camera_angle = yaw_q * pitch_q;

        camera_position += move;
        engine_set_camera(camera_position, camera_angle);
    }

    virtual void on_key_event(int key, int action) override {

        if (key == ENGINE_KEY_ESCAPE && action == ENGINE_PRESS) {
            engine_stop();
        }

        // F to throw a box from camera position
        if (key == KEY_F && action == ENGINE_PRESS) {
            glm::vec3 spawn = camera_position + camera_angle * glm::vec3(0.f, -0.5f, -1.5f);
            glm::vec3 velocity = camera_angle * glm::vec3(0.f, 2.f, -20.f);
            EntityId box = spawn_box(spawn, IDENTITY);
            engine_entity_set_velocity(box, velocity);
        }
    }

    void on_mouse_move() override {}
    void on_mouse_button_change() override {}
    void on_window_resolution_change(glm::vec2) override {}

    EntityId spawn_box(glm::vec3 position, glm::quat rotation) {
        const float half_extent = 0.5f;
        EntityId box = engine_entity_create(MODEL_ID_CUBE, position, rotation, glm::vec3(half_extent));
        engine_entity_enable_physics(box, engine_shape_box(glm::vec3(half_extent)), ENGINE_BODY_DYNAMIC);
        return box;
    }

    std::vector<EntityId> tower;

private:
    const float speed = 0.1f;
    glm::quat camera_angle = {1.f, 0.f, 0.f, 0.f};
    float camera_yaw = 0.f;
    float camera_pitch = 0.f;
    glm::vec3 camera_position = CAMERA_POSITION;
    glm::vec2 last_mouse{0.0f};
    bool first_mouse = true;
};


int main(int argc, char* argv[]) {

    PhysicsDemo demo;
    engine_start(1280, 720, "Physics demo", demo);

    EntityId floor = engine_entity_create(
        MODEL_ID_PLANE,
        glm::vec3(0.f),
        IDENTITY,
        glm::vec3(50.f, 1.f, 50.f)
    );
    engine_entity_enable_physics(
        floor,
        engine_shape_plane(50.f),
        ENGINE_BODY_STATIC
    );

    // tower of boxes
    for (int i = 0; i < 10; i++) {
        glm::vec3 position(0.f, 2.f + i * 1.5f, 0.f);
        glm::quat rotation = glm::angleAxis(0.3f * i, glm::vec3(0.f, 1.f, 0.f));
        demo.tower.push_back(demo.spawn_box(position, rotation));
    }

    engine_run_loop();

    return 0;
}
















