//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-26 18:45:02
//

#include "wn_audio.h"
#include "wn_output.h"

audio_device audio;

void audio_init()
{
    FMOD_RESULT result = FMOD::System_Create(&audio.system);
    if (result != FMOD_RESULT::FMOD_OK) {
        throw_error("Failed to create FMOD!");
    }

    result = audio.system->init(128, FMOD_INIT_NORMAL, nullptr);
    if (result != FMOD_RESULT::FMOD_OK) {
        throw_error("Failed to initialize FMOD!");
    }

    log("[audio] initialized fmod");

    audio_source_load(&audio.door_open, "assets/sfx/door_open.wav");
    audio_source_load(&audio.door_close, "assets/sfx/door_close.wav");
}

void audio_update()
{
    FMOD_RESULT result = audio.system->update();
    if (result != FMOD_RESULT::FMOD_OK) {
        throw_error("Failed to update audio system!");
    }
}

void audio_exit()
{
    audio_source_free(&audio.door_close);
    audio_source_free(&audio.door_open);
    audio.system->release();
}

/// @note(ame): audio source

void audio_source_load(audio_source *source, const std::string& path)
{
    FMOD_RESULT result = audio.system->createSound(path.c_str(), FMOD_DEFAULT, nullptr, &source->sound);
    if (result != FMOD_RESULT::FMOD_OK) {
        throw_error("Failed to create audio source!");
    }
}

void audio_source_play(audio_source *source)
{
    FMOD_RESULT result = audio.system->playSound(source->sound, nullptr, false, &source->channel);
    if (result != FMOD_RESULT::FMOD_OK) {
        throw_error("Failed to play audio source!");
    }
    source->channel->setVolume(1.0f);
}

void audio_source_free(audio_source *source)
{
    source->sound->release();
}
