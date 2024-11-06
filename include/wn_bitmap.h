//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-27 12:01:27
//

#pragma once

#include <vector>
#include <string>

#include "wn_common.h"

/// @todo(ame): compressed bitmaps using BC7 and XTC (my own texture format hdzahdzadzaiudhzadza :333)

struct uncompressed_bitmap
{
    i32 width;
    i32 height;
    std::vector<u8> pixels; 
};

void uncompressed_bitmap_load(uncompressed_bitmap *bitmap, const std::string& path);
