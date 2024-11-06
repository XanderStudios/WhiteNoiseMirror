//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-31 20:51:12
//

#pragma once

#include <angel/angelscript.h>
#include <unordered_map>

#include "wn_common.h"

/// @note(ame): scripts

struct game_script
{
    asIScriptModule* module = nullptr;
    asIScriptContext* ctx = nullptr;
    asITypeInfo* type = nullptr;

    /// @note(ame): behaviour functions
    asIScriptObject* instance = nullptr;
    asIScriptFunction* start = nullptr;
    asIScriptFunction* update = nullptr;
};

void game_script_load(game_script *s, const char* path, const char* script_name);
void game_script_execute(game_script *s);
void game_script_free(game_script *s);

/// @note(ame): script system

struct script_system
{
    asIScriptEngine* engine;
    std::unordered_map<std::string, game_script> script_cache;
};

extern script_system script;

void script_system_init();
game_script* script_system_load_or_get_script(const std::string& path, const std::string& class_name);
void script_system_exit();
