//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-27 12:04:48
//

#include <stb_image.h>

#include "wn_bitmap.h"
#include "wn_output.h"

void uncompressed_bitmap_load(uncompressed_bitmap *bitmap, const std::string& path)
{
    stbi_set_flip_vertically_on_load(true);

    i32 channels = 0;
    stbi_uc *buffer = stbi_load(path.c_str(), &bitmap->width, &bitmap->height, &channels, STBI_rgb_alpha);
    if (!buffer) {
        log("Failed to load bitmap %s", path.c_str());
        throw_error("Failed to load bitmap");
    }

    bitmap->pixels.resize(bitmap->width * bitmap->height * 4);
    memcpy(bitmap->pixels.data(), buffer, bitmap->pixels.size());
    delete buffer;
}
