#pragma once

#include <stddef.h>

namespace ps4b {

constexpr size_t MAX_URL = 1024;

struct PageEntry {
    char url[MAX_URL];
    char title[64];
    bool loaded;
    int status_code;
    size_t content_length;
};

// URL validation and sanitization
bool is_valid_url(const char* url);
bool sanitize_url(char* buffer, size_t buffer_size, const char* url);

// Navigation controller
void nav_set_url(const char* url);
const char* nav_get_url();
bool nav_back();
bool nav_forward();
bool nav_reload();
bool nav_home();

// Callback invoked when navigation occurs
using NavCallback = void (*)(const char* url);
void nav_set_listener(NavCallback callback);

// Debug interface control
void nav_set_debug_enabled(bool enabled);
bool nav_is_debug_enabled();

} // namespace ps4b
