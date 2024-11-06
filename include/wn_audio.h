//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-26 18:26:06
//

#pragma once

#include <string>

#include "fmod.hpp"

struct audio_device
{
    FMOD::System* system;
};

struct audio_source
{
    FMOD::Sound* sound;
    FMOD::Channel* channel;
};

extern audio_device audio;

/// @note(ame): audio system

void audio_init();
void audio_update();
void audio_exit();

/// @note(ame): audio source

void audio_source_load(audio_source *source, const std::string& path);
void audio_source_play(audio_source *source);
void audio_source_free(audio_source *source);
