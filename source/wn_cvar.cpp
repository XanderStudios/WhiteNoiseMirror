//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-21 01:39:44
//

#include "wn_filesystem.h"
#include "wn_cvar.h"

console_var_registry cvars;

void cvar_load(const std::string& registry_path)
{
    nlohmann::json root = fs_loadjson(registry_path);

    for (auto& item : root.items()) {
        cvars.vars[item.key()] = {};

        switch (item.value().type()) {
            case nlohmann::json::value_t::boolean: {
                cvars.vars[item.key()].as.b = item.value().template get<bool>();
                cvars.vars[item.key()].type = ConsoleVarType_Boolean;
                break;
            }
            case nlohmann::json::value_t::number_float: {
                cvars.vars[item.key()].as.f = item.value().template get<f32>();
                cvars.vars[item.key()].type = ConsoleVarType_Float;
                break;
            }
            case nlohmann::json::value_t::number_unsigned: {
                cvars.vars[item.key()].as.u = item.value().template get<u32>();
                cvars.vars[item.key()].type = ConsoleVarType_Unsigned;
                break;
            }
            case nlohmann::json::value_t::string: {
                cvars.vars[item.key()].as.s = item.value().template get<std::string>();
                cvars.vars[item.key()].type = ConsoleVarType_String;
                break;
            }
        }
    }
}

console_var* cvar_get(const std::string& cvar_name)
{
    return &cvars.vars[cvar_name];
}

void cvar_save(const std::string& registry_path)
{
    nlohmann::json root;

    for (auto& pair : cvars.vars) {
        switch (pair.second.type) {
            case ConsoleVarType_Boolean: {
                root[pair.first] = pair.second.as.b;
                break;
            }
            case ConsoleVarType_Float: {
                root[pair.first] = pair.second.as.f;
                break;
            }
            case ConsoleVarType_Unsigned: {
                root[pair.first] = pair.second.as.u;
                break;
            }
            case ConsoleVarType_String: {
                root[pair.first] = pair.second.as.s;
                break;
            }
        }
    }

    fs_writejson(registry_path, root);
}
