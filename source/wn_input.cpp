//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-08 22:32:41
//

#include "wn_input.h"
#include "wn_output.h"

const i32 JOYSTICK_DEAD_ZONE = 8000;

input_context input_ctx;

void input_init()
{
    input_ctx.keys[SDLK_TAB].state = KeyState_Up;
    input_ctx.keys[SDLK_LEFT].state = KeyState_Up;
    input_ctx.keys[SDLK_RIGHT].state = KeyState_Up;
    input_ctx.keys[SDLK_UP].state = KeyState_Up;
    input_ctx.keys[SDLK_DOWN].state = KeyState_Up;
    input_ctx.keys[SDLK_PAGEUP].state = KeyState_Up;
    input_ctx.keys[SDLK_PAGEDOWN].state = KeyState_Up;
    input_ctx.keys[SDLK_HOME].state = KeyState_Up;
    input_ctx.keys[SDLK_END].state = KeyState_Up;
    input_ctx.keys[SDLK_INSERT].state = KeyState_Up;
    input_ctx.keys[SDLK_DELETE].state = KeyState_Up;
    input_ctx.keys[SDLK_BACKSPACE].state = KeyState_Up;
    input_ctx.keys[SDLK_SPACE].state = KeyState_Up;
    input_ctx.keys[SDLK_RETURN].state = KeyState_Up;
    input_ctx.keys[SDLK_ESCAPE].state = KeyState_Up;
    input_ctx.keys[SDLK_APOSTROPHE].state = KeyState_Up;
    input_ctx.keys[SDLK_COMMA].state = KeyState_Up;
    input_ctx.keys[SDLK_MINUS].state = KeyState_Up;
    input_ctx.keys[SDLK_PERIOD].state = KeyState_Up;
    input_ctx.keys[SDLK_SLASH].state = KeyState_Up;
    input_ctx.keys[SDLK_SEMICOLON].state = KeyState_Up;
    input_ctx.keys[SDLK_EQUALS].state = KeyState_Up;
    input_ctx.keys[SDLK_LEFTBRACKET].state = KeyState_Up;
    input_ctx.keys[SDLK_BACKSLASH].state = KeyState_Up;
    input_ctx.keys[SDLK_RIGHTBRACKET].state = KeyState_Up;
    input_ctx.keys[SDLK_GRAVE].state = KeyState_Up;
    input_ctx.keys[SDLK_CAPSLOCK].state = KeyState_Up;
    input_ctx.keys[SDLK_SCROLLLOCK].state = KeyState_Up;
    input_ctx.keys[SDLK_NUMLOCKCLEAR].state = KeyState_Up;
    input_ctx.keys[SDLK_PRINTSCREEN].state = KeyState_Up;
    input_ctx.keys[SDLK_PAUSE].state = KeyState_Up;
    input_ctx.keys[SDLK_LCTRL].state = KeyState_Up;
    input_ctx.keys[SDLK_LSHIFT].state = KeyState_Up;
    input_ctx.keys[SDLK_LALT].state = KeyState_Up;
    input_ctx.keys[SDLK_LGUI].state = KeyState_Up;
    input_ctx.keys[SDLK_RCTRL].state = KeyState_Up;
    input_ctx.keys[SDLK_RSHIFT].state = KeyState_Up;
    input_ctx.keys[SDLK_RALT].state = KeyState_Up;
    input_ctx.keys[SDLK_RGUI].state = KeyState_Up;
    input_ctx.keys[SDLK_APPLICATION].state = KeyState_Up;
    input_ctx.keys[SDLK_0].state = KeyState_Up;
    input_ctx.keys[SDLK_1].state = KeyState_Up;
    input_ctx.keys[SDLK_2].state = KeyState_Up;
    input_ctx.keys[SDLK_3].state = KeyState_Up;
    input_ctx.keys[SDLK_4].state = KeyState_Up;
    input_ctx.keys[SDLK_5].state = KeyState_Up;
    input_ctx.keys[SDLK_6].state = KeyState_Up;
    input_ctx.keys[SDLK_7].state = KeyState_Up;
    input_ctx.keys[SDLK_8].state = KeyState_Up;
    input_ctx.keys[SDLK_9].state = KeyState_Up;
    input_ctx.keys[SDLK_A].state = KeyState_Up;
    input_ctx.keys[SDLK_B].state = KeyState_Up;
    input_ctx.keys[SDLK_C].state = KeyState_Up;
    input_ctx.keys[SDLK_D].state = KeyState_Up;
    input_ctx.keys[SDLK_E].state = KeyState_Up;
    input_ctx.keys[SDLK_F].state = KeyState_Up;
    input_ctx.keys[SDLK_G].state = KeyState_Up;
    input_ctx.keys[SDLK_H].state = KeyState_Up;
    input_ctx.keys[SDLK_I].state = KeyState_Up;
    input_ctx.keys[SDLK_J].state = KeyState_Up;
    input_ctx.keys[SDLK_K].state = KeyState_Up;
    input_ctx.keys[SDLK_L].state = KeyState_Up;
    input_ctx.keys[SDLK_M].state = KeyState_Up;
    input_ctx.keys[SDLK_N].state = KeyState_Up;
    input_ctx.keys[SDLK_O].state = KeyState_Up;
    input_ctx.keys[SDLK_P].state = KeyState_Up;
    input_ctx.keys[SDLK_Q].state = KeyState_Up;
    input_ctx.keys[SDLK_R].state = KeyState_Up;
    input_ctx.keys[SDLK_S].state = KeyState_Up;
    input_ctx.keys[SDLK_T].state = KeyState_Up;
    input_ctx.keys[SDLK_U].state = KeyState_Up;
    input_ctx.keys[SDLK_V].state = KeyState_Up;
    input_ctx.keys[SDLK_W].state = KeyState_Up;
    input_ctx.keys[SDLK_X].state = KeyState_Up;
    input_ctx.keys[SDLK_Y].state = KeyState_Up;
    input_ctx.keys[SDLK_Z].state = KeyState_Up;
    input_ctx.keys[SDLK_F1].state = KeyState_Up;
    input_ctx.keys[SDLK_F2].state = KeyState_Up;
    input_ctx.keys[SDLK_F3].state = KeyState_Up;
    input_ctx.keys[SDLK_F4].state = KeyState_Up;
    input_ctx.keys[SDLK_F5].state = KeyState_Up;
    input_ctx.keys[SDLK_F6].state = KeyState_Up;
    input_ctx.keys[SDLK_F7].state = KeyState_Up;
    input_ctx.keys[SDLK_F8].state = KeyState_Up;
    input_ctx.keys[SDLK_F9].state = KeyState_Up;
    input_ctx.keys[SDLK_F10].state = KeyState_Up;
    input_ctx.keys[SDLK_F11].state = KeyState_Up;
    input_ctx.keys[SDLK_F12].state = KeyState_Up;
    input_ctx.keys[SDLK_F13].state = KeyState_Up;
    input_ctx.keys[SDLK_F14].state = KeyState_Up;
    input_ctx.keys[SDLK_F15].state = KeyState_Up;
    input_ctx.keys[SDLK_F16].state = KeyState_Up;
    input_ctx.keys[SDLK_F17].state = KeyState_Up;
    input_ctx.keys[SDLK_F18].state = KeyState_Up;
    input_ctx.keys[SDLK_F19].state = KeyState_Up;
    input_ctx.keys[SDLK_F20].state = KeyState_Up;
    input_ctx.keys[SDLK_F21].state = KeyState_Up;
    input_ctx.keys[SDLK_F22].state = KeyState_Up;
    input_ctx.keys[SDLK_F23].state = KeyState_Up;
    input_ctx.keys[SDLK_F24].state = KeyState_Up;
    input_ctx.keys[SDLK_AC_BACK].state = KeyState_Up;
    input_ctx.keys[SDLK_AC_FORWARD].state = KeyState_Up;

    input_ctx.gamepad.buttons[SDL_GAMEPAD_BUTTON_START].state = KeyState_Up;
    input_ctx.gamepad.buttons[SDL_GAMEPAD_BUTTON_BACK].state = KeyState_Up;
    input_ctx.gamepad.buttons[SDL_GAMEPAD_BUTTON_WEST].state = KeyState_Up; 
    input_ctx.gamepad.buttons[SDL_GAMEPAD_BUTTON_EAST].state = KeyState_Up; 
    input_ctx.gamepad.buttons[SDL_GAMEPAD_BUTTON_NORTH].state = KeyState_Up;
    input_ctx.gamepad.buttons[SDL_GAMEPAD_BUTTON_SOUTH].state = KeyState_Up;
    input_ctx.gamepad.buttons[SDL_GAMEPAD_BUTTON_DPAD_LEFT].state = KeyState_Up;
    input_ctx.gamepad.buttons[SDL_GAMEPAD_BUTTON_DPAD_RIGHT].state = KeyState_Up;
    input_ctx.gamepad.buttons[SDL_GAMEPAD_BUTTON_DPAD_UP].state = KeyState_Up;
    input_ctx.gamepad.buttons[SDL_GAMEPAD_BUTTON_DPAD_DOWN].state = KeyState_Up;
    input_ctx.gamepad.buttons[SDL_GAMEPAD_BUTTON_LEFT_SHOULDER].state = KeyState_Up;
    input_ctx.gamepad.buttons[SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER].state = KeyState_Up;
    input_ctx.gamepad.buttons[SDL_GAMEPAD_AXIS_LEFT_TRIGGER].state = KeyState_Up;
    input_ctx.gamepad.buttons[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER].state = KeyState_Up;
    input_ctx.gamepad.buttons[SDL_GAMEPAD_BUTTON_LEFT_STICK].state = KeyState_Up;
    input_ctx.gamepad.buttons[SDL_GAMEPAD_BUTTON_RIGHT_STICK].state = KeyState_Up;

    input_ctx.buttons[SDL_BUTTON_LEFT] = false;
    input_ctx.buttons[SDL_BUTTON_RIGHT] = false;
    input_ctx.buttons[SDL_BUTTON_MIDDLE] = false;

    log("[input] initialized input");
}

void input_update(SDL_Event *event)
{
    input_ctx.mx = input_get_mouse_x();
    input_ctx.my = input_get_mouse_y();

    u64 timestamp = SDL_GetTicks();
    switch (event->type) {
        case SDL_EVENT_KEY_DOWN: {
            SDL_Keycode key = event->key.key;
            if (event->key.repeat) {
                input_ctx.keys[key].state = KeyState_Held;
            } else {
                input_ctx.keys[key].state = KeyState_Pressed;
            }
            input_ctx.keys[key].timestamp = timestamp;
            break;
        };
        case SDL_EVENT_KEY_UP: {
            SDL_Keycode key = event->key.key;
            input_ctx.keys[key].state = KeyState_Up;
            input_ctx.keys[key].timestamp = timestamp;
            break;
        };
        case SDL_EVENT_GAMEPAD_ADDED: {
            const SDL_JoystickID id = event->cdevice.which;
            input_ctx.gamepad.connected = true;
            input_ctx.gamepad.controller = SDL_OpenGamepad(id);
            log("[sdl3] gamepad connected %s", SDL_GetGamepadName(input_ctx.gamepad.controller));
            break;
        };
        case SDL_EVENT_GAMEPAD_REMOVED: {
            input_ctx.gamepad.connected = false;
            SDL_CloseGamepad(input_ctx.gamepad.controller);
            log("[sdl3] gamepad disconnected");
            break;
        };
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN: {
            u8 button = event->gbutton.button;
            input_ctx.gamepad.buttons[button].state = KeyState_Pressed;
            input_ctx.gamepad.buttons[button].timestamp = timestamp;
            break;
        };
        case SDL_EVENT_GAMEPAD_BUTTON_UP: {
            u8 button = event->gbutton.button;
            input_ctx.gamepad.buttons[button].state = KeyState_Up;
            input_ctx.gamepad.buttons[button].timestamp = timestamp;
            break;
        };
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            input_ctx.buttons[event->button.button] = true;
            break;
        };
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            input_ctx.buttons[event->button.button] = false;
            break;
        };
    }

    if (input_ctx.gamepad.connected) {
        i16 lx = SDL_GetGamepadAxis(input_ctx.gamepad.controller, SDL_GAMEPAD_AXIS_LEFTX);
        i16 ly = SDL_GetGamepadAxis(input_ctx.gamepad.controller, SDL_GAMEPAD_AXIS_LEFTY);
        i16 rx = SDL_GetGamepadAxis(input_ctx.gamepad.controller, SDL_GAMEPAD_AXIS_RIGHTX);
        i16 ry = SDL_GetGamepadAxis(input_ctx.gamepad.controller, SDL_GAMEPAD_AXIS_RIGHTY);
        i16 rt = SDL_GetGamepadAxis(input_ctx.gamepad.controller, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
        i16 lt = SDL_GetGamepadAxis(input_ctx.gamepad.controller, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);

        input_ctx.gamepad.lx = 0.0f;
        input_ctx.gamepad.rx = 0.0f;
        input_ctx.gamepad.ly = 0.0f;
        input_ctx.gamepad.ry = 0.0f;
        input_ctx.gamepad.rt = 0.0f;
        input_ctx.gamepad.lt = 0.0f;
        
        if (lx < -JOYSTICK_DEAD_ZONE || lx > JOYSTICK_DEAD_ZONE)
            input_ctx.gamepad.lx = f32(lx) / INT16_MAX;
        if (rx < -JOYSTICK_DEAD_ZONE || rx > JOYSTICK_DEAD_ZONE)
            input_ctx.gamepad.rx = f32(rx) / INT16_MAX;
        if (ly < -JOYSTICK_DEAD_ZONE || ly > JOYSTICK_DEAD_ZONE)
            input_ctx.gamepad.ly = f32(ly) / INT16_MAX;
        if (ry < -JOYSTICK_DEAD_ZONE || ry > JOYSTICK_DEAD_ZONE)
            input_ctx.gamepad.ry = f32(ry) / INT16_MAX;
        if (rt > 1)
            input_ctx.gamepad.rt = f32(rt) / INT16_MAX;
        if (lt > 1)
            input_ctx.gamepad.lt = f32(lt) / INT16_MAX;
    }
}

f32 input_get_gamepad_lstick_x()
{
    return input_ctx.gamepad.lx;
}

f32 input_get_gamepad_lstick_y()
{
    return input_ctx.gamepad.ly;
}

f32 input_get_gamepad_rstick_x()
{
    return input_ctx.gamepad.rx;
}

f32 input_get_gamepad_rstick_y()
{
    return input_ctx.gamepad.ry;
}

f32 input_get_gamepad_left_trigger()
{
    return input_ctx.gamepad.lt;
}

f32 input_get_gamepad_right_trigger()
{
    return input_ctx.gamepad.rt;
}

bool input_gamepad_is_button_pressed(u8 button)
{
    return input_ctx.gamepad.buttons[button].state == KeyState_Pressed;
}

bool input_gamepad_is_button_down(u8 button)
{
    return input_ctx.gamepad.buttons[button].state == KeyState_Held;
}

bool input_gamepad_is_button_pressed_or_down(u8 button)
{
    return input_gamepad_is_button_down(button) || input_gamepad_is_button_pressed(button);
}

bool input_gamepad_is_button_up(u8 button)
{
    return input_ctx.gamepad.buttons[button].state == KeyState_Up;
}

void input_post_frame()
{
    input_ctx.lmx = input_get_mouse_x() - input_ctx.mx;
    input_ctx.lmy = input_get_mouse_y() - input_ctx.my;
    
    for (auto& key : input_ctx.keys) {
        u64 timestamp = SDL_GetTicks();
        if (input_ctx.keys[key.first].state == KeyState_Pressed && input_ctx.keys[key.first].timestamp != timestamp)
            input_ctx.keys[key.first].state = KeyState_Held;
    }

    if (input_ctx.gamepad.connected) {
        for (auto& button : input_ctx.gamepad.buttons) {
            u64 timestamp = SDL_GetTicks();
            if (input_ctx.gamepad.buttons[button.first].state == KeyState_Pressed && input_ctx.gamepad.buttons[button.first].timestamp != timestamp)
                input_ctx.gamepad.buttons[button.first].state = KeyState_Held;
        }
    }
}

void input_exit()
{
    if (input_ctx.gamepad.connected) {
        SDL_CloseGamepad(input_ctx.gamepad.controller);
    }
}

bool input_is_key_pressed(SDL_Keycode key)
{
    return input_ctx.keys[key].state == KeyState_Pressed;
}

bool input_is_key_down(SDL_Keycode key)
{
    return input_ctx.keys[key].state == KeyState_Held;
}

bool input_is_key_up(SDL_Keycode key)
{
    return input_ctx.keys[key].state == KeyState_Up;
}

bool input_is_key_pressed_or_down(SDL_Keycode key)
{
    return input_is_key_down(key) || input_is_key_pressed(key);
}

f32 input_get_mouse_x()
{
    f32 x, y;
    SDL_GetGlobalMouseState(&x, &y);
    return x;
}

f32 input_get_mouse_y()
{
    f32 x, y;
    SDL_GetGlobalMouseState(&x, &y);
    return y;
}

f32 input_get_mouse_dx()
{
    return input_ctx.lmx;
}

f32 input_get_mouse_dy()
{
    return input_ctx.lmy;
}

bool input_is_mouse_down(u8 button)
{
    return input_ctx.buttons[button] == true;
}

bool input_is_mouse_up(u8 button)
{
    return input_ctx.buttons[button] == false;
}

void input_add_mapping_binding_key(const std::string& name, SDL_Keycode key)
{
    if (input_ctx.mappings.count(name) == 0) {
        input_ctx.mappings[name] = {};
    }

    input_ctx.mappings[name].descriptors.push_back({
        InputMappingType_Key,
        key,
        0,
        0,
        0
    });
}

void input_add_mapping_binding_mouse(const std::string& name, u8 button)
{
    if (input_ctx.mappings.count(name) == 0) {
        input_ctx.mappings[name] = {};
    }

    input_ctx.mappings[name].descriptors.push_back({
        InputMappingType_MouseButton,
        0,
        button,
        0,
        0
    });
}

void input_add_mapping_binding_axis(const std::string& name, u8 axis)
{
    if (input_ctx.mappings.count(name) == 0) {
        input_ctx.mappings[name] = {};
    }

    input_ctx.mappings[name].descriptors.push_back({
        InputMappingType_GamepadAxis,
        0,
        0,
        0,
        axis
    });
}

void input_add_mapping_binding_gamepad(const std::string& name, u8 button)
{
    if (input_ctx.mappings.count(name) == 0) {
        input_ctx.mappings[name] = {};
    }

    input_ctx.mappings[name].descriptors.push_back({
        InputMappingType_GamepadButton,
        0,
        0,
        button,
        0
    });
}

f32 input_get_mapping_value_analog(const std::string& name)
{
    f32 sum = 0.0f;
    i32 counter = 0;

    for (auto& descriptor : input_ctx.mappings[name].descriptors) {
        if (descriptor.type == InputMappingType_GamepadAxis) {
            switch (descriptor.axis) {
                case AXIS_LEFT_STICK_X: {
                    sum += input_get_gamepad_lstick_x();
                    counter++;
                    break;
                };
                case AXIS_LEFT_STICK_Y: {
                    sum += input_get_gamepad_lstick_y();
                    counter++;
                    break;
                };
                case AXIS_RIGHT_STICK_X: {
                    sum += input_get_gamepad_rstick_x();
                    counter++;
                    break;
                };
                case AXIS_RIGHT_STICK_Y: {
                    sum += input_get_gamepad_rstick_y();
                    counter++;
                    break;
                };
                case AXIS_LEFT_TRIGGER: {
                    sum += input_get_gamepad_left_trigger();
                    counter++;
                    break;
                };
                case AXIS_RIGHT_TRIGGER: {
                    sum += input_get_gamepad_right_trigger();
                    counter++;
                    break;
                };
            }
        }
    }

    if (counter == 0)
        return 0.0f;
    return (sum / counter);
}

bool input_get_mapping_value(const std::string& name, bool repeat)
{
    for (auto& descriptor : input_ctx.mappings[name].descriptors) {
        switch (descriptor.type) {
            case InputMappingType_GamepadButton: {
                if (input_gamepad_is_button_pressed(descriptor.gamepad_button))
                    return true;
                if (input_gamepad_is_button_down(descriptor.gamepad_button) && repeat)
                    return true;
                break;
            };
            case InputMappingType_Key: {
                if (input_is_key_pressed(descriptor.key))
                    return true;
                if (input_is_key_down(descriptor.key) && repeat)
                    return true;
                break;
            };
            case InputMappingType_MouseButton: {
                if (input_is_mouse_down(descriptor.mouse_button))
                    return true;
                break;
            };
        }
    }
    return false;
}
