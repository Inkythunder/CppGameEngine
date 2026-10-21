#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/PlaneShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

#include <cstdarg>
#include <cstdio>
#include <print>
#include <algorithm>
#include <unordered_map>

#include "engine/engine.h"
#include "physics.h"


namespace {

    const float FIXED_DT = 1.f / 60.f;

    JPH::Vec3 to_jolt(glm::vec3 v) {
        return JPH::Vec3(v.x, v.y, v.z);
    }

    JPH::Quat to_jolt(glm::quat q) {
        return JPH::Quat(q.x, q.y, q.z, q.w);
    }

    glm::vec3 to_glm(JPH::Vec3 v) {
        return glm::vec3(v.GetX(), v.GetY(), v.GetZ());
    }

    glm::quat to_glm(JPH::Quat q) {
        return glm::quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
    }

    void trace_impl(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        std::vfprintf(stderr, fmt, args);
        va_end(args);
        std::fputc('\n', stderr);
    }

    #ifdef JPH_ENABLE_ASSERTS
    bool assert_failed_impl(const char* expr, const char* msg, const char* file, JPH::uint line) {
        std::println(stderr, "{}:{}: ({}) {}", file, line, expr, msg != nullptr ? msg : "");
        return true;
    }
    #endif
}

physics_types::JoltGlobals::JoltGlobals() {
    JPH::RegisterDefaultAllocator();
    JPH::Trace = trace_impl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = assert_failed_impl;)
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();
}

physics_types::JoltGlobals::~JoltGlobals() {
    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

PhysicsWorld::PhysicsWorld() {
    physics.Init(
        1024,   // max bodies
        0,      // body mutexes
        1024,   // max body pairs
        1024,   // max contact constraints
        bp_layer_interface, object_vs_bp_filter, object_pair_filter
    );
}
PhysicsWorld::~PhysicsWorld() {
    JPH::BodyInterface& body_interface = physics.GetBodyInterface();
    for(const auto& body : bodies) {
        body_interface.RemoveBody(body.second);
        body_interface.DestroyBody(body.second);
    }
};

void PhysicsWorld::add_body(
    EntityId entity,
    glm::vec3 position,
    glm::quat rotation,
    EngineShape shape,
    EngineBodyType type
) {

    if (has_body(entity)) {
        std::println(stderr, "engine_entity_add_body: entity {} already has a body", entity);
        return;
    }

    if (shape.type == ENGINE_SHAPE_PLANE && type != ENGINE_BODY_STATIC) {
        std::println(stderr, "engine_entity_add_body: plane shapes must be static (entity {})", entity);
        return;
    }

    // Jolt's version of a smart pointer
    JPH::Ref<JPH::Shape> jolt_shape;

    switch (shape.type) {
        case ENGINE_SHAPE_BOX:
            jolt_shape = new JPH::BoxShape(to_jolt(shape.half_extents));
            break;
        case ENGINE_SHAPE_SPHERE:
            jolt_shape = new JPH::SphereShape(shape.radius);
            break;
        case ENGINE_SHAPE_PLANE:
            jolt_shape = new JPH::PlaneShape(
                JPH::Plane(JPH::Vec3::sAxisY(), 0.f),
                nullptr,
                std::max(shape.half_extents.x, shape.half_extents.z)
            );
            break;
    }

    // Static, kinematic, or dynamic motion
    JPH::EMotionType motion_type = JPH::EMotionType::Dynamic;

    // Layer = sorted objects by how they move
    JPH::ObjectLayer layer = physics_types::Layers::MOVING;

    // Simulate the object or not
    JPH::EActivation activation = JPH::EActivation::Activate;
    switch (type) {
        // switch statement controls what collision shape the object has
        case ENGINE_BODY_STATIC:
            motion_type = JPH::EMotionType::Static;
            layer = physics_types::Layers::NON_MOVING;
            activation = JPH::EActivation::DontActivate;
            break;
        case ENGINE_BODY_DYNAMIC:
            motion_type = JPH::EMotionType::Dynamic;
            layer = physics_types::Layers::MOVING;
            activation = JPH::EActivation::Activate;
            break;
        case ENGINE_BODY_KINEMATIC:
            motion_type = JPH::EMotionType::Kinematic;
            layer = physics_types::Layers::MOVING;
            activation = JPH::EActivation::Activate;
            break;
    }

    JPH::BodyCreationSettings settings(
        jolt_shape,
        JPH::RVec3(to_jolt(position)),
        to_jolt(rotation),
        motion_type,
        layer
    );

    JPH::BodyInterface& body_interface = physics.GetBodyInterface();
    bodies[entity] = body_interface.CreateAndAddBody(settings, activation);
}

bool PhysicsWorld::has_body(EntityId entity) const {
    return bodies.contains(entity);
}

void PhysicsWorld::set_position(EntityId entity, glm::vec3 position) {
    JPH::BodyInterface& body_interface = physics.GetBodyInterface();
    body_interface.SetPosition(bodies.at(entity), JPH::Vec3(to_jolt(position)), JPH::EActivation::Activate);
}

void PhysicsWorld::set_rotation(EntityId entity, glm::quat rotation) {
    JPH::BodyInterface& body_interface = physics.GetBodyInterface();
    body_interface.SetRotation(bodies.at(entity), to_jolt(rotation), JPH::EActivation::Activate);
}

void PhysicsWorld::set_velocity(EntityId entity, glm::vec3 velocity) {
    JPH::BodyInterface& body_interface = physics.GetBodyInterface();
    body_interface.SetLinearVelocity(bodies.at(entity), to_jolt(velocity));
}

void PhysicsWorld::add_impulse(EntityId entity, glm::vec3 impulse) {
    JPH::BodyInterface& body_interface = physics.GetBodyInterface();
    body_interface.AddImpulse(bodies.at(entity), to_jolt(impulse));
}

void PhysicsWorld::set_gravity(glm::vec3 gravity) {
    physics.SetGravity(to_jolt(gravity));
}

void PhysicsWorld::update(double elapsed_seconds) {
    accumulator += std::min(elapsed_seconds, 0.25);

    while (accumulator >= FIXED_DT) {
        physics.Update(FIXED_DT, 1, &temp_allocator, &job_system);
        accumulator -= FIXED_DT;
    }
}

void PhysicsWorld::for_each_moving_body(const std::function<void(EntityId, glm::vec3, glm::quat)>& fn) const {
    const JPH::BodyInterface& body_interface = physics.GetBodyInterface();

    for(const auto& [entity, body_id] : bodies) {
        if (body_interface.GetMotionType(body_id) == JPH::EMotionType::Static) continue;
        if (!body_interface.IsActive(body_id)) continue;

        glm::vec3 position = to_glm(JPH::Vec3(body_interface.GetPosition(body_id)));
        glm::quat rotation = to_glm(body_interface.GetRotation(body_id));
        fn(entity, position, rotation);
    }
}

