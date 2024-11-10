//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-09 00:24:46
//

#include <ctime>

#include "wn_discord.h"
#include "wn_output.h"
#include "wn_filesystem.h"

discord_context discord;

void DISCORD_CALLBACK UpdateActivityCallback(void* data, enum EDiscordResult result)
{
}

void discord_init()
{
    nlohmann::json keys = fs_loadjson("assets/keys.json");

    IDiscordCoreEvents events = {};

    DiscordCreateParams params;
    params.client_id = keys["discord"].template get<uint64_t>();
    params.flags = DiscordCreateFlags_Default;
    params.events = &events;
    params.event_data = &discord;

    EDiscordResult result = DiscordCreate(DISCORD_VERSION, &params, &discord.core);
    if (result != DiscordResult_Ok) {
        log("[discord] failed to initialize discord rpc!");
        throw_error("discord error lol");
    }
    log("[discord] initialized discord with RPC key %llu", params.client_id);

    discord.activities = discord.core->get_activity_manager(discord.core);
    discord.start = std::time(nullptr);
}

void discord_run_callbacks(const std::string& title, const std::string& status)
{
    DiscordActivity activity = {};
    strcpy(activity.details, title.c_str());
    strcpy(activity.state, status.c_str());
    activity.timestamps.start = discord.start;
    activity.timestamps.end = std::time(nullptr) + 5 * 60;
    sprintf(activity.assets.large_image, "cover");

    discord.activities->update_activity(discord.activities, &activity, &discord, UpdateActivityCallback);

    EDiscordResult result = discord.core->run_callbacks(discord.core);
    if (result != DiscordResult_Ok) {
        log("[discord] failed to run discord callbacks!");
    }
}

void discord_exit()
{
    discord.core->destroy(discord.core);
}
