//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-20 23:42:44
//

#include "wn_dev_console.h"
#include "wn_output.h"
#include "wn_notification.h"
#include "wn_filesystem.h"
#include "wn_cvar.h"

#include <stdio.h>
#include <stdarg.h>
#include <sstream>
#include <string>

static int   Stricmp(const char* s1, const char* s2)         { int d; while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; } return d; }
static int   Strnicmp(const char* s1, const char* s2, int n) { int d = 0; while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; n--; } return d; }
static char* Strdup(const char* s)                           { IM_ASSERT(s); size_t len = strlen(s) + 1; void* buf = malloc(len); IM_ASSERT(buf); return (char*)memcpy(buf, (const void*)s, len); }
static void  Strtrim(char* s)                                { char* str_end = s + strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0; }

dev_console global_console;

int TextEditCallback(ImGuiInputTextCallbackData* data)
{
    switch (data->EventFlag)
    {
        case ImGuiInputTextFlags_CallbackHistory:
        {
            // Example of HISTORY
            const int prev_history_pos = global_console.history_pos;
            if (data->EventKey == ImGuiKey_UpArrow)
            {
                if (global_console.history_pos == -1)
                    global_console.history_pos = (i32)(global_console.history.size()) - 1;
                else if (global_console.history_pos > 0)
                    global_console.history_pos--;
            }
            else if (data->EventKey == ImGuiKey_DownArrow)
            {
                if (global_console.history_pos != -1)
                    if (++global_console.history_pos >= global_console.history.size())
                    global_console.history_pos = -1;
            }
            
            // A better implementation would preserve the data on the current input line along with cursor position.
            if (prev_history_pos != global_console.history_pos)
            {
                const char* history_str = (global_console.history_pos >= 0) ? global_console.history[global_console.history_pos].c_str() : "";
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, history_str);
            }
        }
    }
    return 0;
}

void dev_console_clear()
{
    global_console.ss.str("");
}

void dev_console_init()
{
    memset(global_console.input_buf, 0, sizeof(global_console.input_buf));
    global_console.history_pos = -1;
    global_console.auto_scroll = true;

    /// @note(ame): add default commands
    dev_console_add_command("clear", [](std::vector<std::string>){
        dev_console_clear();
    });
    dev_console_add_command("map", [](std::vector<std::string> args){
        if (args.size() != 2) {
            log("Invalid format!");
            return;
        }
        std::string level_path = args[1];
        if (!fs_exists(level_path)) {
            log("map: level file %s does not exist.", level_path.c_str());
            return;
        }

        notification_payload payload;
        payload.type = NotificationType_LevelChange;
        payload.level_change.level_path = level_path;
        game_send_notification(payload);
    });
}

void dev_console_shutdown()
{
    dev_console_clear();
}

void dev_console_draw(bool* open, bool* focused)
{
    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Developper Console", open))
    {
        ImGui::End();
        return;
    }
    
    ImGui::Separator();
    
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);
    if (ImGui::BeginPopupContextWindow())
    {
        if (ImGui::Selectable("Clear")) dev_console_clear();
        ImGui::EndPopup();
    }
    
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
    
    ImGui::Text(global_console.ss.str().c_str());
    
    if (global_console.scroll_to_bottom || (global_console.auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
        ImGui::SetScrollHereY(1.0f);
    global_console.scroll_to_bottom = false;
    
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::Separator();
    
    bool reclaim_focus = false;
    ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory;
    if (ImGui::InputText("Input", global_console.input_buf, 512, input_text_flags, &TextEditCallback))
    {
        char* s = global_console.input_buf;
        log("> %s", s);
        std::string cmd = std::string(s);
        std::istringstream stream(cmd);
        std::vector<std::string> args;
        
        std::string arg;
        while (stream >> arg)
            args.push_back(arg);
        
        global_console.history_pos = -1;
        for (i32 i = (i32)global_console.history.size() - 1; i >= 0; i--)
        {
            if (Stricmp(global_console.history[i].c_str(), args[0].c_str()) == 0)
            {
                global_console.history.erase(global_console.history.begin() + i);
                break;
            }
        }
        
        bool should_find_command = true;
        for (auto cvar = cvars.vars.begin(); cvar != cvars.vars.end(); ++cvar)
        {
            if (args[0] == cvar->first)
            {
                if (cvar->second.type == ConsoleVarType_Boolean)
                    cvars.vars[args[0]].as.b = args[1] == "true" ? true : false;
                if (cvar->second.type == ConsoleVarType_Unsigned)
                    cvars.vars[args[0]].as.u = std::stoul(args[1]);
                if (cvar->second.type == ConsoleVarType_Float)
                    cvars.vars[args[0]].as.f = std::stof(args[1]);
                if (cvar->second.type == ConsoleVarType_String)
                    cvars.vars[args[0]].as.s = args[1];
                should_find_command = false;
            }
        }
        
        if (should_find_command)
        {
            bool found_command = false;
            for (auto command = global_console.cmds.begin(); command != global_console.cmds.end(); ++command)
            {
                if (args[0] == command->first)
                {
                    global_console.history.push_back(Strdup(command->first));
                    found_command = true;
                    
                    command->second(args);
                }
            }
            if (!found_command)
                log("Unknown command: %s", args[0].c_str());
        }
        
        global_console.scroll_to_bottom = true;
        strcpy(s, "");
        reclaim_focus = true;
    }
    
    // Auto-focus on window apparition
    ImGui::SetItemDefaultFocus();
    if (reclaim_focus)
        ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
    
    ImGui::End();
}

void dev_console_add_command(const char* name, dev_console_fn function)
{
    global_console.cmds[name] = function;
}

void dev_console_add_log(const std::string& message)
{
    global_console.ss << message;
}