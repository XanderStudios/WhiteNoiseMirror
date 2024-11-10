//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-09 00:23:09
//

#pragma once

#include <discord/discord_game_sdk.h>
#include <string>

#include "wn_common.h"

struct discord_context
{
    IDiscordCore* core;
    IDiscordActivityManager* activities;
    
    u32 start;
};

void discord_init();
void discord_run_callbacks(const std::string& title, const std::string& status);
void discord_exit();
