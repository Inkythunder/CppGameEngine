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
#include <set>
#include "obj_loader.h"
#include "open_gl.h"
#include "engine.h"

static const float speed = 0.1f;
static quat camera_angle = {0.f, 0.f, 0.f, 1.f};

static float camera_yaw = 0.f;
static float camera_pitch = 0.f;

class MyCallbacks : public EngineCallbacks {
public:
    virtual void on_frame_start() {
        vec3 move = {0, 0, 0};

        // WASD Movement
        if(engine_is_key_down(GLFW_KEY_W)) move[2] -= speed;
        if(engine_is_key_down(GLFW_KEY_A)) move[0] -= speed;
        if(engine_is_key_down(GLFW_KEY_S)) move[2] += speed;
        if(engine_is_key_down(GLFW_KEY_D)) move[0] += speed;
        if(engine_is_key_down(GLFW_KEY_SPACE)) move[1] += speed;
        if(engine_is_key_down(GLFW_KEY_LEFT_CONTROL)) move[1] -= speed;

        quat_mul_vec3(move, camera_angle, move);

        // Arrow key camera control
        const float look_speed = 0.05f;
        const float pitch_limit = 1.55334f;

        if(engine_is_key_down(GLFW_KEY_UP)) camera_pitch += look_speed;
        if(engine_is_key_down(GLFW_KEY_DOWN)) camera_pitch -= look_speed;
        if(engine_is_key_down(GLFW_KEY_LEFT)) camera_yaw += look_speed;
        if(engine_is_key_down(GLFW_KEY_RIGHT)) camera_yaw -= look_speed;

        if(camera_pitch > pitch_limit) camera_pitch = pitch_limit;
        if(camera_pitch < -pitch_limit) camera_pitch = -pitch_limit;

        quat yaw_q, pitch_q;
        quat_rotate(yaw_q, camera_yaw, (vec3){0, 1, 0});
        quat_rotate(pitch_q, camera_pitch, (vec3){1, 0, 0});
        quat_mul(camera_angle, yaw_q, pitch_q);

        camera_position = camera_position + Vec3(move);
        engine_set_camera(camera_position, camera_angle);
    }
    virtual void on_key_event(int key, int action) override {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            engine_stop();
        } else if(action == GLFW_PRESS) {
            keys_pressed.insert(key);
        } else if(action == GLFW_RELEASE) {
            keys_pressed.erase(key);
        }
    }
    virtual void on_mouse_move() {

    }
    virtual void on_mouse_button_change() {

    }

private:
    std::set<int> keys_pressed = {};
    Vec3 camera_position;
};

int main(int argc, char* argv[]) {

    MyCallbacks my_callbacks;

    engine_start(640, 480, my_callbacks);

    if(argc < 2) {
        std::println(std::cerr, "No object file provided.");
        return 1;
    } else {
        for(int i = 1; i < argc; i++) {
            std::cout << "argv[" << i << "] = " << argv[1] << std::endl;
            ModelId model_id = engine_model_load(argv[i]);

            Vec3 pos(i - 1 , 0, -5);
            quat rot;
            quat_identity(rot);

            engine_entity_create(model_id, pos, rot);
        }
    }

    engine_run_loop();

    exit(EXIT_SUCCESS);
}


// esc doesn't work
// car's too big
