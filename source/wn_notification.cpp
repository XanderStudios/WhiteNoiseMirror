//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-06 19:17:50
//

#include "wn_notification.h"

void game_send_notification(notification_payload payload)
{
    noti_handler.notifications.push_back(payload);
}
