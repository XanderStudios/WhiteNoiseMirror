//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-26 16:56:27
//

#include <sys/stat.h>
#include <Windows.h>
#include <filesystem>
#include <sstream>
#include <fstream>

#include "wn_filesystem.h"
#include "wn_output.h"
#include "wn_util.h"

bool fs_exists(const std::string& path)
{
    struct stat s;
    if (stat(path.c_str(), &s) == -1)
        return false;
    return true;
}

bool fs_isdir(const std::string& path)
{
    struct stat s;
    if (stat(path.c_str(), &s) == -1)
        return false;
    return (s.st_mode & S_IFDIR) != 0;
}

void fs_create(const std::string& path)
{
    HANDLE handle = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (!handle) {
        throw_error("Error when creating file");
        return;
    }
    CloseHandle(handle);
}

void fs_createdir(const std::string& path)
{
    if (!CreateDirectoryA(path.c_str(), nullptr)) {
        throw_error("Error when creating directory");
    }
}

std::string fs_getextension(const std::string& path)
{
    std::filesystem::path fs_path(path);
    return fs_path.extension().string();
}

std::string fs_cachepath(const std::string& path)
{
    std::stringstream ss;
    ss << ".cache/" << wn_hash(path.c_str(), path.length(), 1000);
    return ss.str();
}

i32 fs_filesize(const std::string& path)
{
    struct stat s;
    if (stat(path.c_str(), &s) == -1)
        return 0;
    return s.st_size;
}

std::string fs_readtext(const std::string& path)
{
    std::ifstream stream(path);
    if (!stream.is_open())
        return "";
    
    std::stringstream ss;
    ss << stream.rdbuf();
    stream.close();
    return ss.str();
}

std::vector<uint8_t> fs_readbytes(const std::string& path)
{
    std::vector<uint8_t> result;

    HANDLE handle = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (!handle) {
        throw_error("File does not exist!");
        return {};
    }

    i32 size = fs_filesize(path);
    result.resize(size);

    DWORD bytes_read = 0;
    ::ReadFile(handle, reinterpret_cast<LPVOID>(result.data()), result.size(), &bytes_read, nullptr);
    CloseHandle(handle);
    return result;
}

void fs_getfiletime(const std::string& path, u32& low, u32& high)
{
    HANDLE handle = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    FILETIME temp;
    GetFileTime(handle, nullptr, nullptr, &temp);
    CloseHandle(handle);

    low = temp.dwLowDateTime;
    high = temp.dwHighDateTime;
}

nlohmann::json fs_loadjson(const std::string& path)
{
    std::ifstream stream(path);
    if (!stream.is_open()) {
        log("Failed to load json file {}", path.c_str());
        return nlohmann::json::parse("{}");
    }
    nlohmann::json document = nlohmann::json::parse(stream);
    stream.close();
    return document;
}

void fs_writejson(const std::string& path, const nlohmann::json& json)
{
    std::ofstream stream(path);
    if (!stream.is_open()) {
        log("Failed to write json file %s", path.c_str());
        return;
    }
    stream << json.dump(4) << std::endl;
    stream.close();
}

void file_watch_start(file_watch *watch, const std::string& path)
{
    watch->path = path;

    timer_init(&watch->check_timer);
    fs_getfiletime(path, watch->low, watch->high);
}

bool file_watch_check(file_watch *watch)
{
    if (TIMER_SECONDS(timer_elasped(&watch->check_timer)) > 0.5f) {
        timer_restart(&watch->check_timer);
        
        u32 low, high;
        fs_getfiletime(watch->path, low, high);

        bool changed = watch->low != low || watch->high != high;
        watch->low = low;
        watch->high = high;
        return changed;
    }
    return false;
}
