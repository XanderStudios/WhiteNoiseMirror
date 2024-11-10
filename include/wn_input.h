//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-08 22:31:40
//

#pragma once

#include <SDL3/SDL.h>
#include <unordered_map>
#include <string>

#include "wn_common.h"

#define AXIS_LEFT_STICK_X 0
#define AXIS_RIGHT_STICK_X 1
#define AXIS_LEFT_STICK_Y 2
#define AXIS_RIGHT_STICK_Y 3
#define AXIS_LEFT_TRIGGER 4
#define AXIS_RIGHT_TRIGGER 5

enum key_state
{
    KeyState_Pressed,
    KeyState_Held,
    KeyState_Up
};

struct gamepad_state
{
    bool connected = false;

    SDL_Gamepad* controller;

    f32 lx, ly;
    f32 rx, ry;
    f32 lt, rt;

    struct button_info {
        key_state state;
        u64 timestamp;
    };
    std::unordered_map<u8, button_info> buttons;
};

enum input_mapping_type
{
    InputMappingType_Key,
    InputMappingType_MouseButton,
    InputMappingType_GamepadButton,
    InputMappingType_GamepadAxis
};

struct input_mapping_descriptor
{
    input_mapping_type type;
    SDL_Keycode key;
    u8 mouse_button;
    u8 gamepad_button;
    u8 axis;
};

struct input_mapping
{
    std::vector<input_mapping_descriptor> descriptors;
};

struct input_context
{
    f32 mx;
    f32 my;
    f32 lmx;
    f32 lmy;

    struct key_info {
        key_state state;
        u64 timestamp;
    };

    std::unordered_map<u8, bool> buttons;
    std::unordered_map<SDL_Keycode, key_info> keys;
    std::unordered_map<std::string, input_mapping> mappings;

    gamepad_state gamepad;
};

void input_init();
void input_update(SDL_Event *event);
void input_post_frame();
void input_exit();

void input_add_mapping_binding_key(const std::string& name, SDL_Keycode key);
void input_add_mapping_binding_mouse(const std::string& name, u8 button);
void input_add_mapping_binding_axis(const std::string& name, u8 axis);
void input_add_mapping_binding_gamepad(const std::string& name, u8 button);

f32 input_get_mapping_value_analog(const std::string& name);
bool input_get_mapping_value(const std::string& name, bool repeat);

/// @note(ame): key input
bool input_is_key_pressed(SDL_Keycode key);
bool input_is_key_down(SDL_Keycode key);
bool input_is_key_pressed_or_down(SDL_Keycode key);
bool input_is_key_up(SDL_Keycode key);

/// @note(ame): mouse input
f32 input_get_mouse_x();
f32 input_get_mouse_y();
f32 input_get_mouse_dx();
f32 input_get_mouse_dy();
bool input_is_mouse_down(u8 button);
bool input_is_mouse_up(u8 button);

/// @note(ame): gamepad input
f32 input_get_gamepad_lstick_x();
f32 input_get_gamepad_lstick_y();
f32 input_get_gamepad_rstick_x();
f32 input_get_gamepad_rstick_y();
f32 input_get_gamepad_lstick_x();
f32 input_get_gamepad_lstick_y();
f32 input_get_gamepad_left_trigger();
f32 input_get_gamepad_right_trigger();

bool input_gamepad_is_button_pressed(u8 button);
bool input_gamepad_is_button_down(u8 button);
bool input_gamepad_is_button_pressed_or_down(u8 button);
bool input_gamepad_is_button_up(u8 button);
