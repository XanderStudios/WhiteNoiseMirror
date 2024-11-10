//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-05 17:14:53
//

#include "wn_uploader.h"
#include "wn_video.h"
#include "wn_timer.h"

uploader_ctx uploader;

void uploader_ctx_enqueue(const std::string& path, texture *out)
{
    upload_job job;
    job.path = path;
    uncompressed_bitmap_load(&job.bitmap, path);
    job.output_tex = out;
    uploader.jobs.push_back(job);

    log("[uploader::texture] Enqueued texture %s", path.c_str());
}

void uploader_ctx_flush()
{
    timer t;
    timer_init(&t);

    std::vector<buffer> temporary_buffers;

    command_buffer cmd_buf = {};
    command_buffer_init(&cmd_buf, D3D12_COMMAND_LIST_TYPE_DIRECT, false);
    command_buffer_begin(&cmd_buf, false);

    for (auto& job : uploader.jobs) {
        buffer staging = {};

        texture_init(job.output_tex, job.bitmap.width, job.bitmap.height, job.bitmap.compressed ? DXGI_FORMAT_BC7_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 0, job.bitmap.levels, false, job.path);
        u64 texture_size = texture_get_size(job.output_tex);
        
        /// @note(ame): this is very ugly and i should not have any D3D12 code in here, but bare with me, it works, so I'm just not gonna touch it lol
        D3D12_RESOURCE_DESC desc = job.output_tex->resource.resource->GetDesc();
        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(desc.MipLevels);
        std::vector<u32> num_rows(desc.MipLevels);
        std::vector<u64> row_sizes(desc.MipLevels);
        uint64_t totalSize = 0;

        video.device->GetCopyableFootprints(&desc, 0, desc.MipLevels, 0, footprints.data(), num_rows.data(), row_sizes.data(), &totalSize);

        u8 *data;
        u8 *pixels = reinterpret_cast<u8*>(job.bitmap.pixels.data());
        buffer_init(&staging, texture_size, 0, BufferType_Copy, false, job.path + " Staging");
        buffer_map(&staging, 0, 0, reinterpret_cast<void**>(&data));
        memset(data, 0, texture_size);
        for (i32 i = 0; i < desc.MipLevels; i++) {
            for (i32 j = 0; j < num_rows[i]; j++) {
                memcpy(data, pixels, row_sizes[i]);

                data += footprints[i].Footprint.RowPitch;
                pixels += row_sizes[i];
            }
        }
        buffer_unmap(&staging);
        command_buffer_copy_buffer_to_texture(&cmd_buf, job.output_tex, &staging, job.bitmap.levels);
        
        temporary_buffers.push_back(staging);
    }

    command_buffer_end(&cmd_buf);
    command_queue_submit(&video.graphics_queue, { &cmd_buf });
    video_wait();

    /// @note(ame): free staging
    for (auto& staging : temporary_buffers) {
        buffer_free(&staging);
    }
    temporary_buffers.clear();

    f32 secs = TIMER_SECONDS(timer_elasped(&t));
    log("[uploader::texture] Uploader flush took %f seconds", secs);

    uploader.jobs.clear();
}
