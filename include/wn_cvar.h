//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-21 01:35:18
//

#pragma once

#include <string>
#include <unordered_map>

#include "wn_common.h"

enum console_var_type
{
    ConsoleVarType_Unsigned,
    ConsoleVarType_Float,
    ConsoleVarType_String,
    ConsoleVarType_Boolean
};

struct console_var
{
    console_var_type type;
    struct {
        u32 u;
        f32 f;
        std::string s;
        bool b;
    } as;
};

struct console_var_registry
{
    std::unordered_map<std::string, console_var> vars;
};

extern console_var_registry cvars;

void cvar_load(const std::string& registry_path);
console_var* cvar_get(const std::string& cvar_name);
void cvar_save(const std::string& registry_path);
