//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-27 14:03:21
//

#include <sstream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "wn_gltf.h"
#include "wn_output.h"
#include "wn_video.h"
#include "wn_resource_cache.h"
#include "wn_util.h"
#include "wn_ai.h"

#define CACHE_PHYSICS 1

void gltf_process_primitive(gltf_model *model, cgltf_primitive *primitive, gltf_node *node)
{
    if (primitive->type != cgltf_primitive_type_triangles) {
        return;
    }

    gltf_primitive out;

    cgltf_attribute* pos_attribute = nullptr;
    cgltf_attribute* uv_attribute = nullptr;
    cgltf_attribute* norm_attribute = nullptr;
    cgltf_attribute* joint_attribute = nullptr;
    cgltf_attribute* weight_attribute = nullptr;

    for (i32 i = 0; i < primitive->attributes_count; i++) {
        if (!strcmp(primitive->attributes[i].name, "POSITION")) {
            pos_attribute = &primitive->attributes[i];
        }
        if (!strcmp(primitive->attributes[i].name, "TEXCOORD_0")) {
            uv_attribute = &primitive->attributes[i];
        }
        if (!strcmp(primitive->attributes[i].name, "NORMAL")) {
            norm_attribute = &primitive->attributes[i];
        }
        if (!strcmp(primitive->attributes[i].name, "JOINTS_0")) {
            joint_attribute = &primitive->attributes[i];
        }
        if (!strcmp(primitive->attributes[i].name, "WEIGHTS_0")) {
            weight_attribute = &primitive->attributes[i];
        }
    }

    i32 vertex_count = pos_attribute->data->count;
    i32 index_count = primitive->indices->count;

    std::vector<gltf_vertex> vertices = {};
    JPH::Array<JPH::Vec3> points = {};
    std::vector<glm::vec3> glm_points = {};
    std::vector<u32> indices = {};

    for (i32 i = 0; i < vertex_count; i++) {
        gltf_vertex vertex = {};

        //u32 ids[4];

        if (!cgltf_accessor_read_float(pos_attribute->data, i, glm::value_ptr(vertex.Position), 4)) {
        }
        if (!cgltf_accessor_read_float(uv_attribute->data, i, glm::value_ptr(vertex.UV), 4)) {
        }
        if (!cgltf_accessor_read_float(norm_attribute->data, i, glm::value_ptr(vertex.Normals), 4)) {
        }
        //if (!cgltf_accessor_read_uint(joint_attribute->data, i, ids, 4)) {
        //}
        //if (!cgltf_accessor_read_float(weight_attribute->data, i, vertex.Weights, 4)) {
        //}

        //for (u32 i = 0; i < MAX_BONE_WEIGHTS; i++)
        //    vertex.MaxBoneInfluence[i] = static_cast<i32>(ids[i]);

        glm::vec4 untransformed_point = node->transform * glm::vec4(vertex.Position, 1.0f);

        points.push_back(JPH::Vec3(untransformed_point.x, untransformed_point.y, untransformed_point.z));
        glm_points.push_back(glm::vec3(untransformed_point));
        vertices.push_back(vertex);
    }

    for (int i = 0; i < index_count; i++) {
        indices.push_back(cgltf_accessor_read_index(primitive->indices, i));
    }

    out.vtx_count = vertex_count;
    out.idx_count = index_count;

    if (model->gen_collisions) {
#if CACHE_PHYSICS
        /// @todo(ame): le physics
        std::stringstream ss;
        ss << ".cache/" << wn_hash(model->path.c_str(), model->path.size(), 1000) << "_" << std::to_string(model->physics_counter) << ".wnp";
        if (fs_exists(ss.str())) {
            log("[physics] loading cached collider %s", ss.str().c_str());
            physics_body_init(&out.body, new cached_shape(ss.str(), physics_materials::LevelMaterial), glm::vec3(0.0f), true);
        } else {
            fs_create(ss.str());
            log("[physics] caching %s", ss.str().c_str());
            physics_shape* shape = new convex_hull_shape(points, physics_materials::LevelMaterial);
            shape->save_shape(ss.str());
            physics_body_init(&out.body, shape, glm::vec3(0.0f), true);
        }
#else
        physics_shape* shape = new convex_hull_shape(points, physics_materials::LevelMaterial);
        physics_body_init(&out.body, shape, glm::vec3(0.0f), true);
#endif
    }
    model->physics_counter++;

    i32 vertex_offset = model->flattened_vertices.size(); // Assuming 3 components per vertex
    model->flattened_vertices.insert(model->flattened_vertices.end(), vertices.begin(), vertices.end());
    for (i32 index : indices) {
        model->flattened_indices.push_back(index + vertex_offset);
    }

    /// @note(ame): create buffers
    buffer_init(&out.vertex_buffer, vertices.size() * sizeof(gltf_vertex), sizeof(gltf_vertex), BufferType_Vertex, false, node->name + " Vertex Buffer");
    buffer_init(&out.index_buffer, indices.size() * sizeof(u32), sizeof(u32), BufferType_Index, false, node->name + " Index Buffer");

    buffer* vertex_staging = new buffer;
    buffer_init(vertex_staging, out.vertex_buffer.size, 0, BufferType_Copy, false, node->name + " Staging Vertex");

    buffer* index_staging = new buffer;
    buffer_init(index_staging, out.index_buffer.size, 0, BufferType_Copy, false, node->name + " Staging Index");
    /// @note(ame): create textures
    cgltf_material *material = primitive->material;
    gltf_material out_material = {};
    out.material_index = model->materials.size();

    /// @note(ame): loading + uploading
    void *data;
    buffer_map(vertex_staging, 0, 0, &data);
    memcpy(data, vertices.data(), out.vertex_buffer.size);
    command_buffer_copy_buffer_to_buffer(&model->model_cmd, &out.vertex_buffer, vertex_staging);
    buffer_unmap(vertex_staging);

    buffer_map(index_staging, 0, 0, &data);
    memcpy(data, indices.data(), out.index_buffer.size);
    command_buffer_copy_buffer_to_buffer(&model->model_cmd, &out.index_buffer, index_staging);
    buffer_unmap(index_staging);

    if (material && material->pbr_metallic_roughness.base_color_texture.texture) {
        std::string path = model->directory + '/' + std::string(material->pbr_metallic_roughness.base_color_texture.texture->image->uri);
        if (model->textures.count(path) > 0) {
            out_material.albedo = &model->textures[path];
            out_material.has_albedo = true;
        } else {
            gltf_texture tex;
            model->textures[path] = tex;
            model->textures[path].handle = resource_cache_get(path, ResourceType_Texture, false);
        
            out_material.albedo = &model->textures[path];
            out_material.has_albedo = true;
        }
    }

    model->staging.push_back(vertex_staging);
    model->staging.push_back(index_staging);

    model->vtx_count += out.vtx_count;
    model->idx_count += out.idx_count;

    model->materials.push_back(out_material);
    node->primitives.push_back(out);
}

void gltf_process_node(gltf_model *model, cgltf_node *node, gltf_node *mnode)
{
    glm::mat4 local_transform(1.0f);
    glm::mat4 translation_matrix(1.0f);
    glm::mat4 rotation_matrix(1.0f);
    glm::mat4 scale_matrix(1.0f);

    if (node->has_translation) {
        glm::vec3 translation = glm::vec3(node->translation[0], node->translation[1], node->translation[2]);
        translation_matrix = glm::translate(glm::mat4(1.0f), translation);
    }
    if (node->has_rotation) {
        rotation_matrix = glm::mat4_cast(glm::quat(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]));
    }
    if (node->has_scale) {
        glm::vec3 scale = glm::vec3(node->scale[0], node->scale[1], node->scale[2]);
        scale_matrix = glm::scale(glm::mat4(1.0f), scale);
    }

    if (node->has_matrix) {
        local_transform *= glm::make_mat4(node->matrix);
    } else {
        local_transform *= translation_matrix * rotation_matrix * scale_matrix;
    }

    glm::mat4 parent_transform = mnode->parent ? mnode->parent->transform : glm::mat4(1.0f);
    mnode->name = node->name ? node->name : "Unnamed Node " + std::to_string(rand());
    mnode->transform = local_transform;

    if (node->mesh) {
        for (i32 i = 0; i < node->mesh->primitives_count; i++) {
            gltf_process_primitive(model, &node->mesh->primitives[i], mnode);
        }
    }

    for (i32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        buffer_init(&mnode->model_buffer[i], 512, 0, BufferType_Constant, false, node->name + std::string(" CBV") + std::to_string(i));
        buffer_build_constant(&mnode->model_buffer[i]);
    }

    mnode->children.resize(node->children_count);
    for (i32 i = 0; i < node->children_count; i++) {
        mnode->children[i] = new gltf_node;
        mnode->children[i]->parent = mnode;

        gltf_process_node(model, node->children[i], mnode->children[i]);
    }
}

void gltf_model_load(gltf_model *model, const std::string& path, bool generate_collisions)
{
    model->path = path;
    model->gen_collisions = generate_collisions;
    model->directory = path.substr(0, path.find_last_of('/'));

    cgltf_options options = {};
    cgltf_data* data = nullptr;

    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success) {
        throw_error("Failed to load GLTF!");
    }
    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success) {
        throw_error("Failed to load GLTF buffers!");
    }
    cgltf_scene *scene = data->scene;

    command_buffer_init(&model->model_cmd, D3D12_COMMAND_LIST_TYPE_DIRECT, false);
    command_buffer_begin(&model->model_cmd, false);

    /// @note(ame): Process animations
    /// @todo(ame): Process animations

    /// @note(ame): Process geometry
    model->root = new gltf_node;
    model->root->name = "RootNode";
    model->root->parent = nullptr;
    model->root->transform = glm::mat4(1.0f);
    model->root->children.resize(scene->nodes_count);

    for (i32 i = 0; i < scene->nodes_count; i++) {
        model->root->children[i] = new gltf_node;
        model->root->children[i]->parent = model->root;

        gltf_process_node(model, scene->nodes[i], model->root->children[i]);
    }
    
    command_buffer_end(&model->model_cmd);
    command_queue_submit(&video.graphics_queue, { &model->model_cmd });
    video_wait();

    for (auto& buffer : model->staging) {
        buffer_free(buffer);
        delete buffer;
    }
    command_buffer_free(&model->model_cmd);
    
    cgltf_free(data);
}

void gltf_free_nodes(gltf_model *model, gltf_node *node)
{
    if (!node) {
        return;
    }

    for (auto& primitive : node->primitives) {
        if (model->gen_collisions) {
            physics_body_free(&primitive.body);
        }
        buffer_free(&primitive.index_buffer);
        buffer_free(&primitive.vertex_buffer);
    }
    for (i32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        if (node->model_buffer[i].size > 0) {
            buffer_free(&node->model_buffer[i]);
        }
    }

    for (gltf_node *child : node->children) {
        gltf_free_nodes(model, child);
    }
    node->children.clear();

    delete node;
}

void gltf_model_free(gltf_model *model)
{
    gltf_free_nodes(model, model->root);

    model->materials.clear();
    for (auto& texture : model->textures) {
        resource_cache_give_back(texture.second.handle);
    }
}
