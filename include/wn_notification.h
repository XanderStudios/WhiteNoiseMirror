//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-06 19:15:18
//

#pragma once

#include <string>
#include <vector>

enum notification_type
{
    NotificationType_LevelChange,
    NotificationType_QuitGame,
};

struct notification_payload
{
    notification_type type;
    struct {
        std::string level_path;
        /// @todo(ame): spawn index
    } level_change;
};

struct notification_handler
{
    std::vector<notification_payload> notifications;
};

extern notification_handler noti_handler;

void game_send_notification(notification_payload payload);
