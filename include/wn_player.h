//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-02 16:24:54
//

#pragma once

#include "wn_world.h"

void player_init(entity *p, glm::vec3 start_pos);
void player_update(entity *p);
void player_free(entity *p);
