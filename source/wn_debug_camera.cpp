//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-27 13:11:09
//

#include <cmath>
#include <Windows.h>

#include "wn_debug_camera.h"
#include "wn_input.h"

#define DEBUG_CAMERA_SPEED 3.0f

/// @note(ame): put this in a wn_input file
void get_mouse_position(i32& x, i32& y)
{
    POINT point;
    GetCursorPos(&point);
    x = point.x;
    y = point.y;
}

void debug_camera_compute_vectors(debug_camera *camera)
{
    glm::vec3 front;
    front.x = std::cos(glm::radians(camera->yaw)) * std::cos(glm::radians(camera->pitch));
    front.y = std::sin(glm::radians(camera->pitch));
    front.z = std::sin(glm::radians(camera->yaw)) * std::cos(glm::radians(camera->pitch));
    camera->front = glm::normalize(front);

    camera->right = glm::normalize(glm::cross(camera->front, glm::vec3(0.0f, 1.0f, 0.0f)));
    camera->up = glm::normalize(glm::cross(camera->right, camera->front));
}

/// @todo(ame): start position?
void debug_camera_init(debug_camera *camera)
{
    memset(camera, 0, sizeof(debug_camera));

    camera->position = glm::vec3(0.0f, 0.0f, -2.0f);
    camera->front = glm::vec3(0.0f, 0.0f, 1.0f);
    camera->right = glm::vec3(1.0f, 0.0f, 0.0f);
    camera->up = glm::vec3(0.0f, 1.0f, 0.0f);
    camera->yaw = 90.0f;

    debug_camera_compute_vectors(camera);
}

void debug_camera_update(debug_camera *camera, f32 dt)
{
    camera->view = glm::lookAt(camera->position, camera->position + camera->front, glm::vec3(0.0f, 1.0f, 0.0f));
    camera->projection = glm::perspective(glm::radians(90.0f), camera->width / camera->height, 0.001f, 10000.0f);

    if (input_is_key_pressed_or_down(SDLK_Z))
        camera->position += camera->front * dt * DEBUG_CAMERA_SPEED;
    if (input_is_key_pressed_or_down(SDLK_S))
        camera->position -= camera->front * dt * DEBUG_CAMERA_SPEED;
    if (input_is_key_pressed_or_down(SDLK_Q))
        camera->position -= camera->right * dt * DEBUG_CAMERA_SPEED;
    if (input_is_key_pressed_or_down(SDLK_D))
        camera->position += camera->right * dt * DEBUG_CAMERA_SPEED;

    f32 dx = input_get_mouse_dx() * 0.1f;
    f32 dy = input_get_mouse_dy() * 0.1f;

    if (input_is_mouse_down(SDL_BUTTON_LEFT)) {
        camera->yaw += dx;
        camera->pitch -= dy;
    }

    debug_camera_compute_vectors(camera);
}
