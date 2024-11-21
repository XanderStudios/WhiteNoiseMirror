//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-20 23:41:58
//

#pragma once

#include <vector>
#include <functional>
#include <unordered_map>
#include <string>
#include <sstream>

#include <imgui/imgui.h>

#include "wn_common.h"

typedef void(*dev_console_fn)(std::vector<std::string>);

struct dev_console
{
    std::unordered_map<const char*, dev_console_fn> cmds;
    char input_buf[512];

    std::stringstream ss;
    std::vector<std::string> history;
    
    i32 history_pos;
    bool auto_scroll;
    bool scroll_to_bottom;
};

void dev_console_init();
void dev_console_shutdown();
void dev_console_draw(bool* open, bool* focused);
void dev_console_add_command(const char* name, dev_console_fn function);
void dev_console_add_log(const std::string& message);
