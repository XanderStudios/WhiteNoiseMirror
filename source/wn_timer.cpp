//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-27 13:33:50
//

#include "wn_timer.h"

void timer_init(timer *t)
{
    QueryPerformanceFrequency(&t->frequency);
    QueryPerformanceCounter(&t->start);
}

f32 timer_elasped(timer *t)
{
    LARGE_INTEGER end;
    QueryPerformanceCounter(&end);
    return (end.QuadPart - t->start.QuadPart) * 1000.0 / t->frequency.QuadPart;
}

void timer_restart(timer *t)
{
    QueryPerformanceCounter(&t->start);
}
