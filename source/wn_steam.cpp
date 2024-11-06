//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-04 20:58:14
//

#include "wn_steam.h"
#include "wn_output.h"

void steam_init()
{
    if (!SteamAPI_Init()) {
        log("[steam] failed to initialize steam!");
        throw_error("steam error");
    }
}

void steam_exit()
{
    SteamAPI_Shutdown();
}
