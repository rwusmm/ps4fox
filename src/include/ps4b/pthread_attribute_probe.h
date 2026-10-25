#pragma once
#include <stdint.h>
#include <stddef.h>
namespace orbisfox {
struct AttributeProbeResult { int init_rc; int destroy_rc; bool guard_ok; };
// Firmware attributes on Orbis/FreeBSD are pointer-sized handles (8 bytes),
// while on Linux glibc they may be 4 bytes. Check both sides of storage.
template<class Attr>
AttributeProbeResult probe_attribute(int (*init)(Attr*), int (*destroy)(Attr*)) {
    struct Guarded { uint64_t before; Attr value; uint64_t after; };
    constexpr uint64_t sentinel = 0x73546f7261476461ULL;
    Guarded storage{sentinel, {}, sentinel};
    AttributeProbeResult result{init(&storage.value), 0, true};
    result.guard_ok = storage.before == sentinel && storage.after == sentinel;
    if (result.init_rc == 0 && result.guard_ok) {
        result.destroy_rc = destroy(&storage.value);
        result.guard_ok = storage.before == sentinel && storage.after == sentinel;
    }
    return result;
}
}
