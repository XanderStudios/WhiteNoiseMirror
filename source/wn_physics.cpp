//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-30 21:02:24
//

#include "wn_physics.h"
#include "wn_output.h"
#include "wn_world.h"

namespace Layers
{
    static constexpr uint8_t NON_MOVING = 0;
    static constexpr uint8_t MOVING = 1;
    static constexpr uint8_t CHARACTER = 2;
    static constexpr uint8_t CHARACTER_GHOST = 3;
    static constexpr uint8_t TRIGGER = 4;
    static constexpr uint8_t NUM_LAYERS = 5;
};

namespace BroadPhaseLayers
{
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr uint32_t NUM_LAYERS(2);
};

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
    {
        // Create a mapping table from object to broad phase layer
        mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
        mObjectToBroadPhase[Layers::CHARACTER] = BroadPhaseLayers::MOVING;
        mObjectToBroadPhase[Layers::CHARACTER_GHOST] = BroadPhaseLayers::MOVING;
        mObjectToBroadPhase[Layers::TRIGGER] = BroadPhaseLayers::MOVING;
    }

    virtual JPH::uint GetNumBroadPhaseLayers() const override
    {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        using namespace JPH;
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }
private:
    JPH::BroadPhaseLayer					mObjectToBroadPhase[Layers::NUM_LAYERS];
};

class MyBodyActivationListener : public JPH::BodyActivationListener
{
public:
    virtual void OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override
    {
        //std::cout << "A body got activated" << std::endl;
    }
    
    virtual void OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override
    {
        //std::cout << "A body went to sleep" << std::endl;
    }
};

class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
    {
        switch (inLayer1)
        {
        case Layers::NON_MOVING:
            return inLayer2 == BroadPhaseLayers::MOVING;
        case Layers::MOVING:
            return true;
        case Layers::TRIGGER:
            return inLayer2 == BroadPhaseLayers::MOVING;
        default:
            return false;
        }
    }
};

class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool					ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
    {
        switch (inObject1)
        {
        case Layers::NON_MOVING:
            return inObject2 == Layers::MOVING || inObject2 == Layers::CHARACTER_GHOST || inObject2 == Layers::CHARACTER; // Non moving only collides with moving
        case Layers::MOVING:
            return true; // Moving collides with everything
        case Layers::CHARACTER_GHOST:
            return inObject2 != Layers::CHARACTER;
        case Layers::CHARACTER:
            return inObject2 != Layers::CHARACTER_GHOST;
        case Layers::TRIGGER:
			return inObject2 == Layers::MOVING || inObject2 == Layers::CHARACTER_GHOST;
        default:
            return false;
        }
    }
};

class MyContactListener : public JPH::ContactListener
{
public:
    virtual JPH::ValidateResult	OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override
	{
		//std::cout << "Contact validate callback" << std::endl;

		// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
  		return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
	}

    virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override
    {
        // if (inBody1.GetObjectLayer() == Layers::TRIGGER && inBody2.GetObjectLayer() == Layers::CHARACTER_GHOST) {
        //     physics_trigger* ptr1 = reinterpret_cast<physics_trigger*>(inBody1.GetUserData());
        //     entity* ptr2 = reinterpret_cast<entity*>(inBody2.GetUserData());
// 
        //     if (!ptr1 || !ptr2)
        //         return;
// 
        //     if (ptr1->on_trigger_enter) {
        //         ptr1->on_trigger_enter(ptr2);
        //     }
        // }
    }

    virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override
	{
		if (inBody1.GetObjectLayer() == Layers::TRIGGER && inBody2.GetObjectLayer() == Layers::CHARACTER_GHOST) {
            entity* ptr1 = reinterpret_cast<entity*>(inBody1.GetUserData());
            entity* ptr2 = reinterpret_cast<entity*>(inBody2.GetUserData());

            if (!ptr1 || !ptr2)
                return;

            if (ptr1->trigger.on_trigger_stay) {
                ptr1->trigger.on_trigger_stay(ptr1, ptr2);
            }
        }

        if (inBody1.GetObjectLayer() == Layers::CHARACTER_GHOST && inBody2.GetObjectLayer() == Layers::TRIGGER) {
            entity* ptr1 = reinterpret_cast<entity*>(inBody1.GetUserData());
            entity* ptr2 = reinterpret_cast<entity*>(inBody2.GetUserData());

            if (!ptr1 || !ptr2)
                return;

            if (ptr2->trigger.on_trigger_stay) {
                ptr2->trigger.on_trigger_stay(ptr2, ptr1);
            }
        }
	}

    /// @todo(ame): on contact removed
};

BPLayerInterfaceImpl JoltBroadphaseLayerInterface = BPLayerInterfaceImpl();
ObjectVsBroadPhaseLayerFilterImpl JoltObjectVSBroadphaseLayerFilter = ObjectVsBroadPhaseLayerFilterImpl();
ObjectLayerPairFilterImpl JoltObjectVSObjectLayerFilter;

JPH::Ref<JPH::PhysicsMaterial> physics_materials::LevelMaterial;
JPH::Ref<JPH::PhysicsMaterial> physics_materials::CharacterMaterial;
JPH::Ref<JPH::PhysicsMaterial> physics_materials::TriggerMaterial;

physics_system physics;

void physics_init()
{
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();

    JPH::RegisterTypes();

    const u32 max_bodies = 4096;
    const u32 num_body_mutexes = 0;
    const u32 max_body_pairs = 2048;
    const u32 max_contact_contraints = 2048;

    physics.system = new JPH::PhysicsSystem;
    physics.system->Init(max_bodies, num_body_mutexes, max_body_pairs, max_contact_contraints, JoltBroadphaseLayerInterface, JoltObjectVSBroadphaseLayerFilter, JoltObjectVSObjectLayerFilter);

    physics.activation_listener = new MyBodyActivationListener;
    physics.system->SetBodyActivationListener(physics.activation_listener);

    physics.contact_listener = new MyContactListener;
    physics.system->SetContactListener(physics.contact_listener);

    physics.system->SetGravity(JPH::Vec3(0.0f, -3.0f, 0.0f));

    physics.body_interface = &physics.system->GetBodyInterface();

    physics.system->OptimizeBroadPhase();
    const u32 available_threads = std::thread::hardware_concurrency() - 1;
    physics.job_system = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, available_threads);

    timer_init(&physics.physics_timer);

    /// @note(ame): Initialize all the materials
    physics_materials::LevelMaterial = JPH::Ref<JPH::PhysicsMaterial>(new JPH::PhysicsMaterialSimple("Level", JPH::Color::sWhite));
    physics_materials::CharacterMaterial = JPH::Ref<JPH::PhysicsMaterial>(new JPH::PhysicsMaterialSimple("Character", JPH::Color::sRed));
    physics_materials::TriggerMaterial = JPH::Ref<JPH::PhysicsMaterial>(new JPH::PhysicsMaterialSimple("Trigger", JPH::Color::sYellow));

    JPH::PhysicsMaterial::sDefault = physics_materials::LevelMaterial;

    log("[physics] initialized physics");
}

void physics_attach_debug_renderer(debug_renderer *dbg)
{
    JPH::DebugRenderer::sInstance = dbg;
}

void physics_draw()
{
    JPH::BodyManager::DrawSettings settings = {};
    settings.mDrawShapeColor = JPH::BodyManager::EShapeColor::MaterialColor;

    physics.system->DrawBodies(settings, JPH::DebugRenderer::sInstance);
}

void physics_update()
{
    i32 collision_steps = 1;
    // Run simulation at 90 FPS
    constexpr f32 min_step_duration = 1.0f / 90.0f;
    constexpr i32 max_step_count = 32;

    if (TIMER_SECONDS(timer_elasped(&physics.physics_timer)) > min_step_duration) {
        try {
            /// @note(ame): update physics
            auto allocator = std::make_shared<JPH::TempAllocatorMalloc>();
            auto error = physics.system->Update(min_step_duration, collision_steps, allocator.get(), physics.job_system);
            if (error != JPH::EPhysicsUpdateError::None) {
                const char* err_msg = "";
                switch (error) {
                    case JPH::EPhysicsUpdateError::ManifoldCacheFull:
                        err_msg = "Manifold cache full";
                        break;
                    case JPH::EPhysicsUpdateError::BodyPairCacheFull:
                        err_msg = "Body pair cache full";
                        break;
                    case JPH::EPhysicsUpdateError::ContactConstraintsFull:
                        err_msg = "contact constraints full";
                        break;
                }
                log("[jolt] error: %s", err_msg);
            }

            /// @note(ame): update characters
            {
                const auto& bplf = physics.system->GetDefaultBroadPhaseLayerFilter(Layers::NON_MOVING);
                const auto& layer_filter = physics.system->GetDefaultLayerFilter(Layers::CHARACTER);
                const auto& gravity = physics.system->GetGravity();
                auto& temp_allocator_ptr = *(allocator);

                for (physics_character* character : physics.characters) {
                    JPH::CharacterVirtual::ExtendedUpdateSettings update_settings = {};
                    update_settings.mWalkStairsStepUp = JPH::Vec3(0.0f, 0.2f, 0.0f);
                    update_settings.mWalkStairsStepDownExtra = JPH::Vec3(0.0f, -0.2f, 0.0f);

                    character->character->ExtendedUpdate(min_step_duration,
                                                         gravity,
                                                         update_settings,
                                                         bplf,
                                                         layer_filter,
                                                         {},
                                                         {},
                                                         temp_allocator_ptr);
                    
                    JPH::Mat44 jolt_transform = character->character->GetWorldTransform();
                    const auto body_rotation = character->character->GetRotation();
                    glm::mat4 transform = glm::mat4(
                        jolt_transform(0, 0), jolt_transform(1, 0), jolt_transform(2, 0), jolt_transform(3, 0),
                        jolt_transform(0, 1), jolt_transform(1, 1), jolt_transform(2, 1), jolt_transform(3, 1),
                        jolt_transform(0, 2), jolt_transform(1, 2), jolt_transform(2, 2), jolt_transform(3, 2),
                        jolt_transform(0, 3), jolt_transform(1, 3), jolt_transform(2, 3), jolt_transform(3, 3)
                    );

                    glm::vec3 scale = glm::vec3();
                    glm::quat rotation = glm::quat();
                    glm::vec3 pos = glm::vec3();
                    glm::vec3 skew = glm::vec3();
                    glm::vec4 pesp = glm::vec4();
                    glm::decompose(transform, scale, rotation, pos, skew, pesp);
                
                    physics.body_interface->MoveKinematic(character->body_index, JPH::Vec3{ pos.x, pos.y, pos.z }, { rotation.x, rotation.y, rotation.z, rotation.w }, min_step_duration);
                }
            }
        } catch (...) {
            log("[jolt] somehow failed to update physics");
        }

        timer_restart(&physics.physics_timer);
    }
}

void physics_exit()
{
    delete physics.job_system;
    delete physics.contact_listener;
    delete physics.activation_listener;
    delete physics.system;

    JPH::UnregisterTypes();

    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

void physics_clear_characters()
{
    physics.characters.clear();
}

void physics_body_init(physics_body *body, physics_shape *shape, glm::vec3 position, bool is_static, void *user_data)
{
    body->shape = shape;
    body->is_static = is_static;

    JPH::BodyCreationSettings settings(body->shape->get_shape(),
                                       JPH::Vec3(position.x, position.y, position.z),
                                       JPH::Quat::sIdentity(),
                                       is_static ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic,
                                       Layers::MOVING);
    
    if (body->shape->rb_type == RigidbodyType_Mesh) {
        settings.mMassPropertiesOverride.mMass = 1.0f;
        settings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
    }

    body->body = physics.body_interface->CreateBody(settings);
    physics.body_interface->AddBody(body->body->GetID(), JPH::EActivation::Activate);

    body->body->SetUserData(reinterpret_cast<u64>(user_data));
}

glm::mat4 physics_body_get_transform(physics_body *body)
{
    JPH::Vec3 position = physics.body_interface->GetCenterOfMassPosition(body->body->GetID());
    JPH::Quat rotation = physics.body_interface->GetRotation(body->body->GetID());

    return glm::translate(glm::mat4(1.0f), glm::vec3(position.GetX(), position.GetY(), position.GetZ()))
         * glm::mat4_cast(glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ()));
}

void physics_body_free(physics_body *body)
{
    physics.body_interface->RemoveBody(body->body->GetID());
    delete body->shape;
}

void physics_trigger_init(physics_trigger *trigger, glm::vec3 position, glm::vec3 size, glm::quat q, void *user_data)
{
    trigger->position = position;
    trigger->size = size;
    trigger->rotation = q;

    JPH::BoxShapeSettings box_settings(JPH::Vec3(size.x, size.y, size.z), 0.05, physics_materials::TriggerMaterial);

    JPH::ShapeSettings::ShapeResult result = box_settings.Create();
    if (result.HasError()) {
        log("%s", result.GetError().c_str());
        throw_error("Somehow failed to create a Jolt shape, lol");
    }
    trigger->shape = result.Get();

    JPH::BodyCreationSettings body_settings(trigger->shape,
                                            JPH::Vec3(position.x, position.y, position.z),
                                            JPH::Quat(q.x, q.y, q.z, q.w),
                                            JPH::EMotionType::Kinematic,
                                            Layers::TRIGGER);
    body_settings.mIsSensor = true;
    body_settings.mCollideKinematicVsNonDynamic = true;

    trigger->body = physics.body_interface->CreateBody(body_settings);
    physics.body_interface->AddBody(trigger->body->GetID(), JPH::EActivation::Activate);

    trigger->body->SetUserData(reinterpret_cast<u64>(user_data));
}

glm::vec3 physics_trigger_get_position(physics_trigger *trigger)
{
    JPH::Vec3 pos = physics.body_interface->GetPosition(trigger->body->GetID());
    return glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ());
}

void physics_trigger_set_position(physics_trigger *trigger, glm::vec3 position)
{
    trigger->position = position;
    physics.body_interface->SetPosition(trigger->body->GetID(), JPH::Vec3(position.x, position.y, position.z), JPH::EActivation::Activate);
}

glm::vec3 physics_trigger_get_rotation(physics_trigger *trigger)
{
    JPH::Vec3 euler = physics.body_interface->GetRotation(trigger->body->GetID()).GetEulerAngles();
    return glm::vec3(euler.GetX(), euler.GetY(), euler.GetZ());
}

void physics_trigger_set_rotation(physics_trigger *trigger, glm::vec3 euler)
{
    JPH::Quat quat = JPH::Quat::sEulerAngles(JPH::Vec3(euler.x, euler.y, euler.z));
    trigger->rotation = glm::quat(quat.GetW(), quat.GetX(), quat.GetY(), quat.GetZ());
    physics.body_interface->SetRotation(trigger->body->GetID(), quat, JPH::EActivation::Activate);
}

void physics_trigger_free(physics_trigger *trigger)
{
    physics.body_interface->RemoveBody(trigger->body->GetID());
}

void physics_character_init(physics_character *c, physics_shape *shape, glm::vec3 position, void *user_data)
{
    c->shape = shape;

    JPH::CharacterVirtualSettings settings = {};
    settings.mShape = shape->get_shape();

    c->character = new JPH::CharacterVirtual(&settings, JPH::RVec3(position.x, position.y, position.z), JPH::QuatArg::sIdentity(), physics.system);

    JPH::BodyCreationSettings body_settings(c->character->GetShape(),
                                            JPH::RVec3(position.x, position.y, position.z),
                                            JPH::Quat::sIdentity(),
                                            JPH::EMotionType::Dynamic,
                                            Layers::CHARACTER_GHOST);
    c->body_index = physics.body_interface->CreateAndAddBody(body_settings, JPH::EActivation::Activate);
    physics.body_interface->SetUserData(c->body_index, reinterpret_cast<u64>(user_data));

    physics.characters.push_back(c);
}

void physics_character_move(physics_character *c, glm::vec3 velocity)
{
    c->character->SetLinearVelocity(JPH::Vec3(velocity.x, velocity.y, velocity.z));
}

glm::mat4 physics_character_get_transform(physics_character *c)
{
    JPH::Mat44 jolt_transform = physics.body_interface->GetWorldTransform(c->body_index);
    glm::mat4 transform = glm::mat4(
        jolt_transform(0, 0), jolt_transform(1, 0), jolt_transform(2, 0), jolt_transform(3, 0),
        jolt_transform(0, 1), jolt_transform(1, 1), jolt_transform(2, 1), jolt_transform(3, 1),
        jolt_transform(0, 2), jolt_transform(1, 2), jolt_transform(2, 2), jolt_transform(3, 2),
        jolt_transform(0, 3), jolt_transform(1, 3), jolt_transform(2, 3), jolt_transform(3, 3)
    );

    return transform;
}

glm::vec3 physics_character_get_position(physics_character *c)
{
    JPH::Vec3 pos = c->character->GetPosition();
    return glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ());
}

void physics_character_set_position(physics_character *c, glm::vec3 p)
{
    physics.body_interface->SetPosition(c->body_index, JPH::Vec3(p.x, p.y, p.z), JPH::EActivation::Activate);
    c->character->SetPosition(JPH::Vec3(p.x, p.y, p.z));
}

void physics_character_free(physics_character *c)
{
    physics.body_interface->RemoveBody(c->body_index);
    delete c->shape;
}

ray_result physics_body_trace_ray(physics_body *body, glm::vec3 start, glm::vec3 end)
{
    ray_result result;

    glm::vec3 direction = end - start;

    JPH::RayCast ray;
    ray.mOrigin = JPH::Vec3(start.x, start.y, start.z);
    ray.mDirection = JPH::Vec3(direction.x, start.y, start.z);

    JPH::RayCastResult hit;
    result.hit = body->shape->get_shape()->CastRay(ray, JPH::SubShapeIDCreator(), hit);
    result.point = start + hit.mFraction * (end - start);
    result.t = hit.mFraction;
    return result;
}

struct skip_character_filter : JPH::ObjectLayerFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer inLayer) const
    {
        if (inLayer == Layers::CHARACTER_GHOST || inLayer == Layers::TRIGGER)
            return false;
        return true;
    }
};

ray_result physics_character_trace_ray(physics_character *c, glm::vec3 start, glm::vec3 end)
{
    JPH::Vec3 from = JPH::Vec3(start.x, start.y, start.z);
	JPH::Vec3 dir = JPH::Vec3(end.x - start.x, end.y - start.y, end.z - start.z);

    JPH::RRayCast ray = JPH::RRayCast(from, dir);
	JPH::ClosestHitCollisionCollector<JPH::CastRayCollector> collector;

    JPH::RayCastResult result;
    skip_character_filter filter;

    physics.system->GetNarrowPhaseQuery().CastRay(ray, JPH::RayCastSettings(), collector, {}, filter);

    ray_result rresult = {};
    if (collector.HadHit()) {
        rresult.hit = true;
        JPH::BroadPhaseCastResult result = collector.mHit;
    
        const float hit_frac = result.mFraction;
		const JPH::Vec3& hit_pos = ray.GetPointOnRay(result.mFraction);

        rresult.point = glm::vec3(hit_pos.GetX(), hit_pos.GetY(), hit_pos.GetZ());
        rresult.t = hit_frac;
    } else {
        rresult.hit = false;
    }
    return rresult;
}

ray_result physics_character_trace_ray_dir(physics_character *c, glm::vec3 start, glm::vec3 dir)
{
    JPH::Vec3 from = JPH::Vec3(start.x, start.y, start.z);
	JPH::Vec3 jdir = JPH::Vec3(dir.x, dir.y, dir.z);

    JPH::RayCast ray = JPH::RayCast(from, jdir);
	JPH::ClosestHitCollisionCollector<JPH::RayCastBodyCollector> collector;

    JPH::RayCastResult result;
    skip_character_filter filter;

    physics.system->GetBroadPhaseQuery().CastRay(ray, collector, {}, filter);

    ray_result rresult = {};
    if (collector.HadHit()) {
        rresult.hit = true;
        JPH::BroadPhaseCastResult result = collector.mHit;
    
        const float hit_frac = result.mFraction;
		const JPH::Vec3& hit_pos = ray.GetPointOnRay(result.mFraction);

        rresult.point = glm::vec3(hit_pos.GetX(), hit_pos.GetY(), hit_pos.GetZ());
        rresult.t = hit_frac;
    } else {
        rresult.hit = false;
    }
    return rresult;
}
