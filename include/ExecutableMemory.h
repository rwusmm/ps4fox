#ifndef EXECUTABLE_MEMORY_H
#define EXECUTABLE_MEMORY_H

#include <stddef.h>
#include <stdint.h>

namespace ps4_browser {

constexpr size_t jitArenaBytes = 64 * 1024 * 1024;
constexpr size_t jitNativePage = 16384;

struct JitBackend {
    void* user = nullptr;
    int32_t (*create)(void*, size_t, int*) = nullptr;
    int32_t (*map)(void*, int, void**) = nullptr;
    int32_t (*protect)(void*, void*, size_t, int) = nullptr;
    bool (*query)(void*, void*, size_t, int) = nullptr;
    int32_t (*unmap)(void*, void*, size_t) = nullptr;
    int32_t (*close)(void*, int) = nullptr;
};

struct JitStats {
    bool ready = false;
    bool claimed = false;
    uint32_t failedPhase = 0;
    int32_t lastResult = 0;
    size_t reservedBytes = 0;
    size_t committedBytes = 0;
};

bool prepare_executable_memory(const JitBackend& backend);
void* reserve_executable_memory(size_t bytes);
void release_executable_memory(void* address, size_t bytes);
bool protect_executable_memory(void* address, size_t bytes, int protection);
void shutdown_executable_memory();
JitStats jit_memory_stats();
const char* jit_gate_reason(const JitStats& state);

} // namespace ps4_browser

#endif // EXECUTABLE_MEMORY_H
