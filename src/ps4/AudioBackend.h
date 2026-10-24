#ifndef AUDIO_BACKEND_H
#define AUDIO_BACKEND_H

#include <stdint.h>

namespace ps4_browser {

struct AudioBackend {
    void* user = nullptr;
    int32_t (*open)(void*) = nullptr; // 256frames, 48kHz, S16stereo, default main output
    int32_t (*output)(void*, int32_t, const int16_t*) = nullptr;
    int32_t (*close)(void*, int32_t) = nullptr;
};

void set_audio_backend(const AudioBackend& backend);
bool audio_backend_ready();

} // namespace ps4_browser

#endif // AUDIO_BACKEND_H
