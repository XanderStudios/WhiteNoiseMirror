//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-27 13:32:54
//

#pragma once

#include <Windows.h>

#include "wn_common.h"

#define TIMER_SECONDS(s) (s / 1000.0f)

struct timer
{
    LARGE_INTEGER frequency;
    LARGE_INTEGER start;
};

void timer_init(timer *t);
f32 timer_elasped(timer *t);
void timer_restart(timer *t);
