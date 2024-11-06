//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-26 13:15:01
//

#include "wn_output.h"

#include <sstream>
#include <iomanip>
#include <ctime>
#include <iostream>
#include <cstdarg>

#include <Windows.h>

void throw_error(const std::string& message)
{
    MessageBoxA(nullptr, message.c_str(), "WHITE NOISE ERROR", MB_OK | MB_ICONERROR);
    __debugbreak();
}

void log(const char* msg, ...)
{
    std::stringstream ss;
    char buf[4096];
    va_list vl;
    
    va_start(vl, msg);
    vsnprintf(buf, sizeof(buf), msg, vl);
    va_end(vl);
    ss << buf << std::endl;
    std::cout << ss.str();
}
