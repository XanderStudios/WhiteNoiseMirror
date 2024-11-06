//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-05 16:45:47
//

#include "wn_resource_cache.h"
#include "wn_uploader.h"

resource_cache global_cache;

void resource_cache_init()
{
    global_cache.resources = {};
}

resource *resource_cache_get(const std::string& path, resource_type type, bool gen_collisions)
{
    if (global_cache.resources.count(path) > 0) {
        return global_cache.resources[path];
    } else {
        resource* res = new resource;;
        res->type = type;
        res->path = path;
        res->ref_count++;

        switch (type) {
            case ResourceType_GLTF: {
                gltf_model_load(&res->model, path, gen_collisions);
                log("[resource_cache::gltf] Loaded GLTF %s", path.c_str());
                break;
            }
            case ResourceType_Texture: {
                uploader_ctx_enqueue(path, &res->tex);
                break;
            }
        }

        global_cache.resources[path] = res;
        return global_cache.resources[path];
    }
}

void resource_cache_give_back(resource *res)
{
    res->ref_count--;
    if (res->ref_count == 0) {
        switch (res->type) {
            case ResourceType_Texture:
                texture_free(&res->tex);
                break;
            case ResourceType_GLTF:
                gltf_model_free(&res->model);
                break;
        }
        delete res;
    }
}

void resource_cache_free()
{
    for (auto& res : global_cache.resources) {
        if (res.second->ref_count > 0) {
            log("[resource_cache] forgot to free resource %s", res.first.c_str());
            resource_cache_give_back(res.second);
        }
    }
}
