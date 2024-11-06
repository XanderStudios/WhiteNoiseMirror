//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-05 01:44:28
//

#include "wn_gamepad.h"
#include "wn_output.h"

gamepad_system gamepad_sys;

void gamepad_init()
{
    
}

void gamepad_update(SDL_Event *event)
{
    if (event->type == SDL_EVENT_JOYSTICK_ADDED) {
        const SDL_JoystickID id = event->cdevice.which;
        gamepad_sys.gamepad.id = id;
        gamepad_sys.gamepad.stick = SDL_OpenJoystick(gamepad_sys.gamepad.id);
        gamepad_sys.gamepad.connected = true;
        log("[sdl3] gamepad connected");
    }
    if (event->type == SDL_EVENT_JOYSTICK_REMOVED) {
        gamepad_sys.gamepad.connected = false;
        SDL_CloseJoystick(gamepad_sys.gamepad.stick);
        log("[sdl3] gamepad disconnected");
    }

}

void gamepad_exit()
{
    if (gamepad_sys.gamepad.connected) {
        SDL_CloseJoystick(gamepad_sys.gamepad.stick);
    }
}
