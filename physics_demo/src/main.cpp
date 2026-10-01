#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

#include <cstdarg>
#include <cstdio>
#include <print>
#include <thread>
#include <vector>

#include "engine/engine.h"


namespace Layers {
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}

namespace BroadPhaseLayers {
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::uint NUM_LAYERS(2);
}


class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override {
        if (a == Layers::NON_MOVING) return b == Layers::MOVING;
        return true;
    }
};


// maps each object layer to a broad phase layer
class BPLayerInterfaceImpl : public JPH::BroadPhaseLayerInterface {
public:
    JPH::uint GetNumBroadPhaseLayers() const override {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override {
        return layer == Layers::NON_MOVING ? BroadPhaseLayers::NON_MOVING : BroadPhaseLayers::MOVING;
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override {
        return layer == BroadPhaseLayers::NON_MOVING ? "NON_MOVING" : "MOVING";
    }
#endif
};


// object layer vs broad phase layer
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer bp_layer) const override {
        if (layer == Layers::NON_MOVING) return bp_layer == BroadPhaseLayers::MOVING;
        return true;
    }
};


static void trace_impl(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::vfprintf(stderr, fmt, args);
    va_end(args);
    std::fputc('\n', stderr);
}

#ifdef JPH_ENABLE_ASSERTS
static bool assert_failed_impl(const char* expr, const char* msg, const char* file, JPH::uint line) {
    std::println(stderr, "{}:{}: ({}) {}", file, line, expr, msg != nullptr ? msg : "");
    return true;
}
#endif


static JPH::Vec3 to_jolt(glm::vec3 v) {
    return JPH::Vec3(v.x, v.y, v.z);
}

static glm::vec3 to_glm(JPH::Vec3 v) {
    return glm::vec3(v.GetX(), v.GetY(), v.GetZ());
}

static glm::quat to_glm(JPH::Quat q) {
    return glm::quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
}

struct PhysicsObject {
    JPH::BodyID body;
    EntityId entity;
};


static const int KEY_F = 70; // GLFW key code for F
static const float FIXED_DT = 1.f / 60.f;
static const glm::vec3 CAMERA_POSITION(0.f, 2.f, 12.f);

class PhysicsDemo : public EngineCallbacks {
public:
    PhysicsDemo(
        JPH::PhysicsSystem& physics,
        JPH::TempAllocator& temp,
        JPH::JobSystem& jobs
    ) : physics(physics), temp_allocator(temp), job_system(jobs) {}

    void on_game_start() override {
        last_time = engine_get_time();
    }

    void on_frame_start() override {
        double now = engine_get_time();
        accumulator += now - last_time;
        last_time = now;

        // step physics at a fixed rate - not framerate
        while (accumulator >= FIXED_DT) {
            physics.Update(FIXED_DT, 1, &temp_allocator, &job_system);
            accumulator -= FIXED_DT;
        }

        // copy each body's transform onto its rendered entity
        JPH::BodyInterface& bodies = physics.GetBodyInterface();
        for(const PhysicsObject& obj : objects) {
            engine_entity_set_position(obj.entity, to_glm(JPH::Vec3(bodies.GetPosition(obj.body))));
            engine_entity_set_rotation(obj.entity, to_glm(bodies.GetRotation(obj.body)));
        }

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
        if (key == KEY_F && action == ENGINE_PRESS) {
            glm::vec3 spawn = camera_position + camera_angle * glm::vec3(0.f, -0.5f, -1.5f);
            glm::vec3 velocity = camera_angle * glm::vec3(0.f, 2.f, -20.f);
            JPH::BodyID id = spawn_box(spawn, 0.5f);
            physics.GetBodyInterface().SetLinearVelocity(id, to_jolt(velocity));
        }
    }

    void on_mouse_move() override {}
    void on_mouse_button_change() override {}
    void on_window_resolution_change(glm::vec2) override {}

    JPH::BodyID spawn_box(glm::vec3 position, float half_extent) {
        JPH::BodyCreationSettings settings(
            new JPH::BoxShape(JPH::Vec3::sReplicate(half_extent)),
            JPH::RVec3(to_jolt(position)),
            JPH::Quat::sRotation(JPH::Vec3::sAxisY(),
            0.3f * objects.size()),
            JPH::EMotionType::Dynamic,
            Layers::MOVING
        );

        JPH::BodyID id = physics.GetBodyInterface().CreateAndAddBody(settings, JPH::EActivation::Activate);

        EntityId entity = engine_entity_create(MODEL_ID_CUBE, position, glm::quat(1.f, 0.f, 0.f, 0.f), glm::vec3(half_extent));

        objects.push_back({id, entity});
        return id;
    }

    std::vector<PhysicsObject> objects;

private:
    JPH::PhysicsSystem& physics;
    JPH::TempAllocator& temp_allocator;
    JPH::JobSystem& job_system;
    double last_time = 0.0;
    double accumulator = 0.0;
    const float speed = 0.1f;
    glm::quat camera_angle = {1.f, 0.f, 0.f, 0.f};
    float camera_yaw = 0.f;
    float camera_pitch = 0.f;
    glm::vec3 camera_position = CAMERA_POSITION;
    glm::vec2 last_mouse{0.0f};
    bool first_mouse = true;
};


int main(int argc, char* argv[]) {

    JPH::RegisterDefaultAllocator();
    JPH::Trace = trace_impl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = assert_failed_impl;)
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    JPH::TempAllocatorImpl temp_allocator(10 * 1024 * 1024);
    JPH::JobSystemThreadPool job_system(
        JPH::cMaxPhysicsJobs,
        JPH::cMaxPhysicsBarriers,
        std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1)
    );

    BPLayerInterfaceImpl bp_layer_interface;
    ObjectVsBroadPhaseLayerFilterImpl object_vs_bp_filter;
    ObjectLayerPairFilterImpl object_pair_filter;

    JPH::PhysicsSystem physics;
    physics.Init(
        1024,   // max bodies
        0,      // body mutexes
        1024,   // max body pairs
        1024,   // max contact constraints
        bp_layer_interface, object_vs_bp_filter, object_pair_filter
    );

    PhysicsDemo demo(physics, temp_allocator, job_system);


    engine_start(1280, 720, "Physics demo", demo);

    JPH::BodyInterface& bodies = physics.GetBodyInterface();

    // floor is a static box whose top face sits at y = 0
    JPH::BodyCreationSettings floor_settings(
        new JPH::BoxShape(JPH::Vec3(50.f, 1.f, 50.f)),
        JPH::RVec3(0.f, -1.f, 0.f),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Static,
        Layers::NON_MOVING
    );

    JPH::BodyID floor_id = bodies.CreateAndAddBody(floor_settings, JPH::EActivation::DontActivate);

    engine_entity_create(
        MODEL_ID_PLANE,
        glm::vec3(0.f),
        glm::quat(1.f, 0.f, 0.f, 0.f),
        glm::vec3(50.f, 1.f, 50.f)
    );

    // tower of boxes
    for (int i = 0; i < 10; i++) {
        demo.spawn_box(glm::vec3(0.f, 2.f + i * 1.5f, 0.f), 0.5f);
    }

    // build the broad phase once after adding many bodies as startup
    physics.OptimizeBroadPhase();

    engine_run_loop();


    for(const PhysicsObject& obj : demo.objects) {
        bodies.RemoveBody(obj.body);
        bodies.DestroyBody(obj.body);
    }
    bodies.RemoveBody(floor_id);
    bodies.DestroyBody(floor_id);

    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

    return 0;
}
















