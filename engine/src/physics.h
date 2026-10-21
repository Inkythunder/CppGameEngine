#pragma once
#include <functional>
#include <memory>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include "engine/engine.h"

namespace physics_types {
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

    class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
    public:
        bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override {
            if (a == Layers::NON_MOVING) return b == Layers::MOVING;
            return true;
        }
    };

    struct JoltGlobals {
        JoltGlobals();
        ~JoltGlobals();
    };
}

class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();
    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    void add_body(
        EntityId entity,
        glm::vec3 position,
        glm::quat rotation,
        EngineShape shape,
        EngineBodyType type
    );

    bool has_body(EntityId entity) const;

    void set_position(EntityId entity, glm::vec3 position);

    void set_rotation(EntityId entity, glm::quat rotation);

    void set_velocity(EntityId entity, glm::vec3 velocity);

    void add_impulse(EntityId entity, glm::vec3 impulse);

    void set_gravity(glm::vec3 gravity);

    void update(double elapsed_seconds);

    void for_each_moving_body(const std::function<void(EntityId, glm::vec3, glm::quat)>& fn) const;

private:
    physics_types::JoltGlobals globals;
    JPH::TempAllocatorImpl temp_allocator{10 * 1024 * 1024};
    JPH::JobSystemThreadPool job_system{
        JPH::cMaxPhysicsJobs,
        JPH::cMaxPhysicsBarriers,
        std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1)
    };
    physics_types::BPLayerInterfaceImpl bp_layer_interface;
    physics_types::ObjectVsBroadPhaseLayerFilterImpl object_vs_bp_filter;
    physics_types::ObjectLayerPairFilterImpl object_pair_filter;
    JPH::PhysicsSystem physics;
    std::unordered_map<EntityId, JPH::BodyID> bodies;
    double accumulator = 0.0;
};
