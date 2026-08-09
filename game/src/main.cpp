#define GLFW_INCLUDE_NONE

#include <stdlib.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <print>
#include "engine/engine.h"

static const float speed = 0.1f;
static glm::quat camera_angle(1.f, 0.f, 0.f, 0.f);

static float camera_yaw = 0.f;
static float camera_pitch = 0.f;

class MyCallbacks : public EngineCallbacks {
public:
    virtual void on_game_start() {

        wolf = engine_load_texture_from_file("wolf.jpg");
    }

    virtual void on_frame_start() {

        double now = engine_get_time();
        glm::quat spin = glm::angleAxis((float)(90.0f * (3.141f / 180.0f) * now), glm::vec3(0,1,0));
        engine_entity_set_rotation(1, spin);

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

        // making a new rect on screen
        engine_ui_draw_rect(
            glm::vec2(wolf_x, wolf_y),
            glm::vec2(wolf->width/2, wolf->height/2),
            glm::vec4(1.f),
            wolf->texture_id,
            TextureUvs(
                glm::vec2(0, 1),
                glm::vec2(1, 1),
                glm::vec2(0, 0),
                glm::vec2(1, 0)
            )
        );

        // draw text
        engine_display_text("test text", glm::vec2(100.f, 100.f), 50.f);
    }

    virtual void on_key_event(int key, int action) override {
        if (key == ENGINE_KEY_ESCAPE && action == ENGINE_PRESS) {
            engine_stop();
        }
    }

    virtual void on_mouse_move() {

    }

    virtual void on_mouse_button_change() {

    }

    virtual void on_window_resolution_change(glm::vec2 res) {

        wolf_x = res.x - wolf->width;
        wolf_y = res.y - wolf->height;
    }

private:
    glm::vec3 camera_position{0.0f};
    glm::vec2 last_mouse{0.0f};
    bool first_mouse = true;
    std::optional<LoadedTexture> wolf;
    float wolf_x, wolf_y;
};

int main(int argc, char* argv[]) {

    MyCallbacks my_callbacks;

    engine_start(640, 480, "OpenGL C++", my_callbacks);

    if(argc < 2) {
        std::println(std::cerr, "No object file provided.");
    } else {
        for(int i = 1; i < argc; i++) {
            std::cout << "argv[" << i << "] = " << argv[1] << std::endl;
            ModelId model_id = engine_model_load(argv[i]);

            glm::vec3 pos(i - 1, 0, -5);
            glm::quat rot(1.f, 0.f, 0.f, 0.f);
            glm::vec3 scale(1.f/2.f);

            engine_entity_create(model_id, pos, rot, scale);
        }
    }

    glm::vec3 default_size(1);
    glm::quat default_rotation(1.f, 0.f, 0.f, 0.f);

    ModelId chair_id = engine_model_load("chair/Philippe Hurel_Peggy_Chair N040226.obj");
    glm::quat rot(glm::angleAxis(glm::radians(-90.f), glm::vec3(1.f, 0.f, 0.f)));
    engine_entity_create(chair_id, glm::vec3(5, 0, 0), rot, glm::vec3(1.f/300.f));

    ModelId car_id = engine_model_load("car/bugatti.obj");
    engine_entity_create(car_id, glm::vec3(5, 0, -5), default_rotation, glm::vec3(1.f/2.f));

    engine_entity_create(MODEL_ID_PLANE, glm::vec3{0.f, 0.f, 0.f}, default_rotation, default_size);
    engine_entity_create(MODEL_ID_CUBE, glm::vec3{0.f, 0.f, 5.f}, default_rotation, default_size);

    engine_run_loop();

    exit(EXIT_SUCCESS);
}
