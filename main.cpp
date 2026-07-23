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

class MyCallbacks : public EngineCallbacks {
public:
    virtual void on_frame_start() {
        Vec3 move;

        if(engine_is_key_down(GLFW_KEY_W)) move.z -= speed;
        if (engine_is_key_down(GLFW_KEY_A)) move.x -= speed;
        if (engine_is_key_down(GLFW_KEY_S)) move.z += speed;
        if (engine_is_key_down(GLFW_KEY_D)) move.x += speed;
        if (engine_is_key_down(GLFW_KEY_SPACE)) move.y += speed;
        if (engine_is_key_down(GLFW_KEY_LEFT_CONTROL)) move.y -= speed;

        camera_position = camera_position + move;
        engine_set_camera(camera_position, camera_angle);
    }
    virtual void on_key_event(int key, int action) override {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            engine_stop();

        // WASD Movement
        if (key == GLFW_KEY_W && action == GLFW_PRESS) {
            keys_pressed.insert(GLFW_KEY_W);
        }
        if (key == GLFW_KEY_W && action == GLFW_RELEASE) {
            keys_pressed.erase(GLFW_KEY_W);
        }
        if (key == GLFW_KEY_A && action == GLFW_PRESS) {
            keys_pressed.insert(GLFW_KEY_A);
        }
        if (key == GLFW_KEY_A && action == GLFW_RELEASE) {
            keys_pressed.erase(GLFW_KEY_A);
        }
        if (key == GLFW_KEY_S && action == GLFW_PRESS) {
            keys_pressed.insert(GLFW_KEY_S);
        }
        if (key == GLFW_KEY_S && action == GLFW_RELEASE) {
            keys_pressed.erase(GLFW_KEY_S);
        }
        if (key == GLFW_KEY_D && action == GLFW_PRESS) {
            keys_pressed.insert(GLFW_KEY_D);
        }
        if (key == GLFW_KEY_D && action == GLFW_RELEASE) {
            keys_pressed.erase(GLFW_KEY_D);
        }
        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
            keys_pressed.insert(GLFW_KEY_SPACE);
        }
        if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) {
            keys_pressed.erase(GLFW_KEY_SPACE);
        }
        if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_PRESS) {
            keys_pressed.insert(GLFW_KEY_LEFT_CONTROL);
        }
        if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_RELEASE) {
            keys_pressed.erase(GLFW_KEY_LEFT_CONTROL);
        }

        if (key == GLFW_KEY_UP && action == GLFW_PRESS) {

            quat angle, result;
            quat_rotate(angle, 0.05f, (vec3){1, 0, 0});
            quat_mul(result, camera_angle, angle);
            memcpy(camera_angle, result, sizeof(quat));
            engine_set_camera(camera_position, camera_angle);
        }
        if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {

            quat angle, result;
            quat_rotate(angle, -0.05f, (vec3){1, 0, 0});
            quat_mul(result, camera_angle, angle);
            memcpy(camera_angle, result, sizeof(quat));
            engine_set_camera(camera_position, camera_angle);
        }
        if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {

            quat angle, result;
            quat_rotate(angle, 0.05f, (vec3){0, 1, 0});
            quat_mul(result, camera_angle, angle);
            memcpy(camera_angle, result, sizeof(quat));
            engine_set_camera(camera_position, camera_angle);
        }
        if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {

            quat angle, result;
            quat_rotate(angle, -0.05f, (vec3){0, 1, 0});
            quat_mul(result, camera_angle, angle);
            memcpy(camera_angle, result, sizeof(quat));
            engine_set_camera(camera_position, camera_angle);
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
