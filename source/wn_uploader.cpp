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
        void *data;
        buffer staging = {};
        texture_init(job.output_tex, job.bitmap.width, job.bitmap.height, DXGI_FORMAT_R8G8B8A8_UNORM);
        u64 texture_size = texture_get_size(job.output_tex);
        buffer_init(&staging, texture_size, 0, BufferType_Copy);
        buffer_map(&staging, 0, 0, &data);
        memcpy(data, job.bitmap.pixels.data(), job.bitmap.pixels.size());
        buffer_unmap(&staging);
        command_buffer_copy_buffer_to_texture(&cmd_buf, job.output_tex, &staging);
        temporary_buffers.push_back(staging);
    }

    command_buffer_end(&cmd_buf);
    command_queue_submit(&video.graphics_queue, { &cmd_buf });
    video_wait();

    /// @note(ame): free staging
    for (auto& staging : temporary_buffers) {
        buffer_free(&staging);
    }

    f32 secs = TIMER_SECONDS(timer_elasped(&t));
    log("[uploader::texture] Uploader flush took %f seconds", secs);
}
