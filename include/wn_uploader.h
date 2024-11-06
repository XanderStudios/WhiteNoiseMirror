//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-05 17:12:45
//

#pragma once

#include <vector>

#include "wn_bitmap.h"
#include "wn_d3d12.h"
#include "wn_output.h"

struct upload_job
{
    /// @todo(ame): support geometry
    uncompressed_bitmap bitmap;
    texture* output_tex;
};

struct uploader_ctx
{
    std::vector<upload_job> jobs;
};

void uploader_ctx_enqueue(const std::string& path, texture *out);
void uploader_ctx_flush();
