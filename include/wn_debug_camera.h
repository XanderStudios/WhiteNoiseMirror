//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-27 13:07:12
//

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include "wn_common.h"

struct debug_camera
{
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 right;
    glm::vec3 up;

    f32 width;
    f32 height;

    f32 yaw;
    f32 pitch;

    glm::mat4 view;
    glm::mat4 projection;
};

void debug_camera_init(debug_camera *camera);
void debug_camera_update(debug_camera *camera, f32 dt);
