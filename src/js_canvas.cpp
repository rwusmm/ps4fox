#define _POSIX_C_SOURCE 200809L
#include "orbisfox/js_canvas.h"

#if __has_include(<jsapi.h>)
#include <jsapi.h>
#include <js/Initialization.h>
#include <js/Context.h>
#include <js/Stack.h>
#include <js/GlobalObject.h>
#include <js/Realm.h>
#include <js/RealmOptions.h>
#include <js/CompilationAndEvaluation.h>
#include <js/SourceText.h>
#include <js/CallAndConstruct.h>
#include <js/Interrupt.h>
#include <js/Conversions.h>
#include <js/CharacterEncoding.h>
#include <js/PropertyAndElement.h>
#include <js/GCAPI.h>
#include <mozilla/Utf8.h>
#include <atomic>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <new>

namespace orbisfox {
namespace {
constexpr size_t source_limit = 128 * 1024;
constexpr uint32_t command_limit = 512;
static bool runtime_initialized = false;
static bool runtime_shut_down = false;
static unsigned live_contexts = 0;
static const JSClass global_class = {"OrbisFox", JSCLASS_GLOBAL_FLAGS, &JS::DefaultGlobalClassOps,
                                    nullptr, nullptr, nullptr};
uint64_t now_ns() {
    timespec t{};
    if (clock_gettime(CLOCK_MONOTONIC, &t) != 0) return 0;
    return static_cast<uint64_t>(t.tv_sec) * 1000000000ULL + static_cast<uint64_t>(t.tv_nsec);
}
constexpr const char* self_test = R"JS(
(() => {
  let n = 0;
  function check(v) { if (!v) throw Error('self-test ' + n); ++n; }
  check(Array.from({length:64}, (_, i) => i*i).reduce((a,b) => a+b, 0) === 85344);
  check(2n ** 64n === 18446744073709551616n);
  check(new Map([['fox',42]]).get('fox') === 42);
  check(new Set([1,1,2]).size === 2);
  return n;
})()
)JS";
}

struct JsCanvas::State {
    JSContext* cx = nullptr;
    JS::PersistentRootedObject global;
    JS::PersistentRootedValue render_fn;
    Logger logger = nullptr;
    char last_error[192]{};
    uint32_t command_count = 0;
    unsigned passed = 0;
};

JsCanvas::~JsCanvas() { stop(); }
const char* JsCanvas::error() const { return error_; }
uint32_t JsCanvas::commands() const { return state_ ? state_->command_count : 0; }
unsigned JsCanvas::self_tests() const { return state_ ? state_->passed : 0; }

bool JsCanvas::init(Logger logger) {
    if (state_) return true;
    if (!runtime_initialized) {
        if (!JS_Init()) return false;
        runtime_initialized = true;
    }
    state_ = new (std::nothrow) State();
    if (!state_) return false;
    state_->logger = logger;
    return true;
}

bool JsCanvas::evaluate_integer(const char*, size_t, int& result, unsigned) {
    result = 0;
    return false;
}

bool JsCanvas::load_demo(const char*) {
    return false;
}

bool JsCanvas::render(Surface&, int, int, bool, double) {
    return false;
}

void JsCanvas::stop() {
    if (!state_) return;
    delete state_;
    state_ = nullptr;
}

void shutdown_javascript_runtime() {
    if (runtime_initialized && !runtime_shut_down) {
        JS_ShutDown();
        runtime_shut_down = true;
    }
}

} // namespace orbisfox

#else

// Standalone fallback when SpiderMonkey headers (jsapi.h) are not available
namespace orbisfox {

struct JsCanvas::State {};

JsCanvas::~JsCanvas() { stop(); }
const char* JsCanvas::error() const { return error_; }
uint32_t JsCanvas::commands() const { return 0; }
unsigned JsCanvas::self_tests() const { return 0; }

bool JsCanvas::init(Logger logger) {
    if (logger) logger("SpiderMonkey JS not enabled (headless/stub mode)");
    return true;
}

bool JsCanvas::evaluate_integer(const char*, size_t, int& result, unsigned) {
    result = 0;
    return false;
}

bool JsCanvas::load_demo(const char*) {
    return false;
}

bool JsCanvas::render(Surface&, int, int, bool, double) {
    return false;
}

void JsCanvas::stop() {}

void shutdown_javascript_runtime() {}

} // namespace orbisfox

#endif
