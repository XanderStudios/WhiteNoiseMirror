//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-26 16:52:12
//

#pragma once

#include <string>
#include <vector>
#include <json/json.hpp>

#include "wn_common.h"
#include "wn_timer.h"

struct file_watch
{
    std::string path;
    u32 low;
    u32 high;

    timer check_timer;
};

void file_watch_start(file_watch *watch, const std::string& path);
bool file_watch_check(file_watch *watch);

bool fs_exists(const std::string& path);
bool fs_isdir(const std::string& path);
void fs_create(const std::string& path);
void fs_createdir(const std::string& path);
std::string fs_getextension(const std::string& path);
std::string fs_cachepath(const std::string& path);
i32 fs_filesize(const std::string& path);
std::string fs_readtext(const std::string& path);
std::vector<uint8_t> fs_readbytes(const std::string& path);
void fs_getfiletime(const std::string& path, u32& low, u32& high);
nlohmann::json fs_loadjson(const std::string& path);
void fs_writejson(const std::string& path, const nlohmann::json& json);
