//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-30 21:01:59
//

#pragma once

#include <memory>
#include <vector>
#include <fstream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#define JPH_DEBUG_RENDERER
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/StreamWrapper.h>
#include <Jolt/Core/StreamIn.h>
#include <Jolt/Core/StreamOut.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/PhysicsMaterialSimple.h>

#include "wn_common.h"
#include "wn_output.h"
#include "wn_timer.h"
#include "wn_debug_renderer.h"
#include "wn_filesystem.h"

struct entity;

/// @note(ame): SHAPES
namespace physics_materials
{
    extern JPH::Ref<JPH::PhysicsMaterial> LevelMaterial;
    extern JPH::Ref<JPH::PhysicsMaterial> CharacterMaterial;
    extern JPH::Ref<JPH::PhysicsMaterial> TriggerMaterial;
};

enum RigidbodyType
{
    RigidbodyType_Box,
    RigidbodyType_Capsule,
    RigidbodyType_Mesh,
    RigidbodyType_ConvexHull
};

struct physics_shape
{
    RigidbodyType rb_type;
    JPH::Ref<JPH::PhysicsMaterial> material = nullptr;
    JPH::Ref<JPH::Shape> shape;

    ~physics_shape()
    {
    }

    virtual JPH::Ref<JPH::Shape> get_shape()
    {
        return shape;
    }

    void save_shape(const std::string& path) {
        std::ofstream stream(path, std::ios::binary);
        if (!stream.is_open()) {
            log("sigma");
        }

        JPH::StreamOutWrapper stream_out(stream);
        get_shape()->SaveBinaryState(stream_out);
        stream.close(); 
    }
};

struct box_shape : public physics_shape
{
    glm::vec3 size;

    box_shape(glm::vec3 s, JPH::Ref<JPH::PhysicsMaterial> m = nullptr)
    : size(s) {
        material = m;
        rb_type = RigidbodyType_Box;

        JPH::ShapeSettings::ShapeResult result;

        JPH::BoxShapeSettings settings(JPH::Vec3(size.x, size.y, size.z), 0.05f, material);
        result = settings.Create();
        if (result.HasError()) {
            log("%s", result.GetError().c_str());
            throw_error("Somehow failed to create a Jolt shape, lol");
        }
        shape = result.Get();
    } 
};

struct capsule_shape : public physics_shape
{
    f32 radius;
    f32 height;

    capsule_shape(f32 r, f32 h, JPH::Ref<JPH::PhysicsMaterial> m = nullptr)
    : radius(r), height(h) {
        material = m;
        rb_type = RigidbodyType_Capsule;

        JPH::ShapeSettings::ShapeResult result;

        JPH::CapsuleShapeSettings settings(height / 2.0f, radius, material);
        result = settings.Create();
        if (result.HasError()) {
            log("%s", result.GetError().c_str());
            throw_error("Somehow failed to create a Jolt shape, lol");
        }
        shape = result.Get();
    }
};

struct mesh_shape : public physics_shape
{
    JPH::Array<JPH::Triangle> triangles;

    mesh_shape(const JPH::Array<JPH::Triangle>& v, JPH::Ref<JPH::PhysicsMaterial> m = nullptr)
        : triangles(v) {
        material = m;
        rb_type = RigidbodyType_Mesh;
    
        JPH::ShapeSettings::ShapeResult result;

        JPH::PhysicsMaterialList list;
        if (material) {
            list.push_back(material);
        }

        JPH::MeshShapeSettings settings(triangles, list);
        result = settings.Create();
        if (result.HasError()) {
            log("%s", result.GetError().c_str());
            throw_error("Somehow failed to create a Jolt shape, lol");
        }
        shape = result.Get();
    }
};

struct convex_hull_shape : public physics_shape
{
    JPH::Array<JPH::Vec3> points;

    convex_hull_shape(const JPH::Array<JPH::Vec3>& v, JPH::Ref<JPH::PhysicsMaterial> m = nullptr)
        : points(v) {
        material = m;
        rb_type = RigidbodyType_ConvexHull;
    
        JPH::ShapeSettings::ShapeResult result;
    
        JPH::ConvexHullShapeSettings settings(points, 0.05f, material);
        result = settings.Create();
        if (result.HasError()) {
            log("%s", result.GetError().c_str());
            throw_error("Somehow failed to create a Jolt shape, lol");
        }
        shape = result.Get();
    }
};

struct cached_shape : public physics_shape
{
    std::string shape_path;

    cached_shape(const std::string& path, JPH::Ref<JPH::PhysicsMaterial> m = nullptr)
        : shape_path(path)
    {
        rb_type = RigidbodyType_ConvexHull;
    
        std::ifstream stream(shape_path, std::ios_base::binary);

        JPH::StreamInWrapper stream_in(stream);
        JPH::Shape::ShapeResult result = JPH::Shape::sRestoreFromBinaryState(stream_in);
        if (result.HasError()) {
            log("%s", result.GetError().c_str());
            throw_error("Somehow failed to create a Jolt shape, lol");
        }
        shape = result.Get();
    }
};

/// @note(ame): rigidbody
struct physics_body
{
    JPH::Body* body;
    physics_shape* shape;
    bool is_static = false;
};

void physics_body_init(physics_body *body, physics_shape *shape, glm::vec3 position = glm::vec3(0.0f), bool is_static = false, void *user_data = nullptr);
void physics_body_free(physics_body *body);

/// @note(ame): controllers

struct physics_character
{
    JPH::CharacterVirtual* character;
    physics_shape *shape;
    JPH::BodyID body_index;
};

void physics_character_init(physics_character *c, physics_shape *shape, glm::vec3 position, void *user_data = nullptr);
void physics_character_move(physics_character *c, glm::vec3 velocity);
glm::mat4 physics_character_get_transform(physics_character *c);
glm::vec3 physics_character_get_position(physics_character *c);
void physics_character_set_position(physics_character *c, glm::vec3 p);
void physics_character_free(physics_character *c);

/// @note(ame): trigger
struct physics_trigger
{
    /// @note(ame): Serialization data
    glm::vec3 position;
    glm::vec3 size;

    JPH::Body* body;
    JPH::Ref<JPH::Shape> shape;
    
    /// @todo(ame): OnTriggerExit
    std::function<void(entity* collider, entity *collided)> on_trigger_enter = {};
    std::function<void(entity* collider, entity *collided)> on_trigger_stay = {};
};

void physics_trigger_init(physics_trigger *trigger, glm::vec3 position = glm::vec3(0.0f), glm::vec3 size = glm::vec3(0.0f), void *user_data = nullptr);
glm::vec3 physics_trigger_get_position(physics_trigger *trigger);
void physics_trigger_set_position(physics_trigger *trigger, glm::vec3 position);
glm::vec3 physics_trigger_get_rotation(physics_trigger *trigger);
void physics_trigger_set_rotation(physics_trigger *trigger, glm::vec3 q);
void physics_trigger_free(physics_trigger *trigger);

/// @note(ame): SYSTEM
class BPLayerInterfaceImpl;
class MyContactListener;
class MyBodyActivationListener;

struct physics_system
{
    JPH::PhysicsSystem* system;
    JPH::JobSystemThreadPool* job_system;
    JPH::BodyInterface* body_interface;

    MyContactListener* contact_listener;
    MyBodyActivationListener* activation_listener;
    BPLayerInterfaceImpl* broadphase_interface;

    std::vector<physics_character*> characters;
    timer physics_timer;
};

extern physics_system physics;

void physics_init();
void physics_attach_debug_renderer(debug_renderer *dbg);
void physics_draw();
void physics_update();
void physics_clear_characters();
void physics_exit();
