//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-27 12:04:48
//

#include <stb_image.h>
#include <nvtt/nvtt.h>
#include <filesystem>
#include <sstream>

#include "wn_bitmap.h"
#include "wn_output.h"
#include "wn_filesystem.h"
#include "wn_util.h"

class nvtt_error_handler : nvtt::ErrorHandler
{
public:
    virtual void error(nvtt::Error e) override {
       switch (e) {
           case nvtt::Error::Error_UnsupportedOutputFormat: {
               log("[nvtt] Error_UnsupportedOutputFormat");
               break;
           }
           case nvtt::Error::Error_UnsupportedFeature: {
               log("[nvtt] Error_UnsupportedFeature");
               break;
           }
           case nvtt::Error::Error_Unknown: {
               log("[nvtt] Error_Unknown");
               break;
           }
           case nvtt::Error::Error_InvalidInput: {
               log("[nvtt] Error_InvalidInput");
               break;
           }
           case nvtt::Error::Error_FileWrite: {
               log("[nvtt] Error_FileWrite");
               break;
           }
           case nvtt::Error::Error_FileOpen: {
               log("[nvtt] Error_FileOpen");
               break;
           }
           case nvtt::Error::Error_CudaError: {
               log("[nvtt] Error_CudaError");
               break;
           }
           default: {
               log("[nvtt] unknown error!");
               break;
           }
       }
    }
};

class texture_writer : nvtt::OutputHandler
{
public:
    texture_writer(const std::string& path, int width, int height, int mipCount, int mode) {
        f = fopen(path.c_str(), "wb+");
        if (!f) {
            log("[nvtt] failed to fopen file %s", path.c_str());
        }

        bitmap_header header;
        header.width = width;
        header.height = height;
        header.levels = mipCount;
        
        fwrite(&header, sizeof(header), 1, f);
    }

    ~texture_writer() {
        fclose(f);
    }

    virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel) override {}
    virtual void endImage() override {}

    virtual bool writeData(const void * data, int size) override {
        fwrite(data, size, 1, f);
        return true;
    }

private:
    FILE *f;
};

bool is_valid_extension(const std::string& extension)
{
    if (extension == ".png")
        return true;
    if (extension == ".jpg")
        return true;
    if (extension == ".jpeg")
        return true;

    return false;
}

std::string get_cached_path(const std::string& path)
{
    std::stringstream path_ss;

    const char *key = path.c_str();
    uint64_t hash = wn_hash(key, path.length(), 1000);
    path_ss << ".cache/" << hash << ".wnt";

    return path_ss.str();
}

void bitmap_compress_recursive(const std::string& directory)
{
    if (!fs_exists(".cache")) {
        fs_createdir(".cache/");
    }
    
    nvtt_error_handler error_handler;
    nvtt::Context context;
    context.enableCudaAcceleration(true);
    
    if (context.isCudaAccelerationEnabled()) {
        log("[bitmap_compressor] Compressing with CUDA");
    } else {
        log("[bitmap_compressor] Compressing without CUDA");
    }

    nvtt::CompressionOptions compression_options;
    compression_options.setFormat(nvtt::Format::Format_BC7);

    for (const auto& dir_entry : std::filesystem::recursive_directory_iterator(directory)) {
        std::string entry_path = dir_entry.path().string();
        std::replace(entry_path.begin(), entry_path.end(), '\\', '/');
        
        std::string cached = get_cached_path(entry_path);
        
        if (!is_valid_extension(fs_getextension(entry_path))) {
            continue;
        }

        if (fs_exists(cached)) {
            log("[bitmap_compressor] %s already compressed -- skipping.", entry_path.c_str());
            continue;
        }

        nvtt::Surface image;
        if (!image.load(entry_path.c_str())) {
            log("[nvtt] Failed to load texture");
        }

        i32 mip_count = image.countMipmaps();
        texture_writer writer(cached, image.width(), image.height(), mip_count, 7);

        nvtt::OutputOptions output_options;
        output_options.setErrorHandler(reinterpret_cast<nvtt::ErrorHandler*>(&error_handler));
        output_options.setOutputHandler(reinterpret_cast<nvtt::OutputHandler*>(&writer));

        for (i32 i = 0; i < mip_count; i++) {
            if (!context.compress(image, 0, i, compression_options, output_options)) {
                log("[bitmap_compressor] failed to compress texture!");
            }

            if (i == mip_count - 1) break;

            // Prepare the next mip:
            image.toLinearFromSrgb();
            image.premultiplyAlpha();

            image.buildNextMipmap(nvtt::MipmapFilter_Box);
        
            image.demultiplyAlpha();
            image.toSrgb();
        }

        log("[bitmap_compressor] compressed %s to %s", entry_path.c_str(), cached.c_str());
    }
}

void uncompressed_bitmap_load(uncompressed_bitmap *bitmap, const std::string& path)
{
    std::string cached = get_cached_path(path);
    if (!fs_exists(cached)) {
        stbi_set_flip_vertically_on_load(true);

        i32 channels = 0;
        stbi_uc *buffer = stbi_load(path.c_str(), &bitmap->width, &bitmap->height, &channels, STBI_rgb_alpha);
        if (!buffer) {
            log("[bitmap] failed to load bitmap %s", path.c_str());
            throw_error("Failed to load bitmap");
        }

        bitmap->pixels.resize(bitmap->width * bitmap->height * 4);
        bitmap->levels = 1;
        memcpy(bitmap->pixels.data(), buffer, bitmap->pixels.size());
        delete buffer;   
    } else {
        u64 size = fs_filesize(cached);
        u64 buffer_size = size - sizeof(bitmap_header);
        bitmap->pixels.resize(buffer_size);

        bitmap_header header;

        FILE* f = fopen(cached.c_str(), "rb+");
        fread(&header, sizeof(header), 1, f);
        fread(bitmap->pixels.data(), bitmap->pixels.size(), 1, f);
        fclose(f);

        bitmap->width = header.width;
        bitmap->height = header.height;
        bitmap->levels = header.levels;
        bitmap->compressed = true;
    }
}
