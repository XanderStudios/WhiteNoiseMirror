//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-05 01:42:59
//

#pragma once

#include <SDL3/SDL.h>

#include "wn_common.h"

#define MAX_GAMEPADS 4

struct gamepad_state
{
    bool connected = false;

    SDL_Joystick* stick;
    SDL_JoystickID id;
};

struct gamepad_system
{
    gamepad_state gamepad;
};

extern gamepad_system gamepad_sys;

void gamepad_init();
void gamepad_update(SDL_Event *event);
void gamepad_exit();
