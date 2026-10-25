#include "browser_navigation.h"
#include "ps4b/presenter.h"
#include <string.h>
#include <ctype.h>

namespace ps4b {

static PageEntry g_pages[32];
static int g_num_pages = 0;
static int g_active = -1;
static char g_url_bar[MAX_URL] = "http://about:blank";
static NavCallback g_listener = nullptr;
static bool g_debug_enabled = false;

// URL validation - checks for valid URL format
bool is_valid_url(const char* url) {
    if (!url || strlen(url) == 0 || strlen(url) >= MAX_URL) {
        return false;
    }

    // Check for null bytes
    if (strchr(url, '\0') != url + strlen(url)) {
        return false;
    }

    // Check for dangerous protocols
    if (strstr(url, "javascript:") != nullptr) {
        return false;
    }
    if (strstr(url, "data:") != nullptr) {
        return false;
    }
    if (strstr(url, "vbscript:") != nullptr) {
        return false;
    }

    // Check for valid scheme (http, https, about, b://)
    const char* scheme_end = strstr(url, "://");
    if (scheme_end == nullptr) {
        // Check for about:blank or similar
        if (strncmp(url, "about:", 6) == 0) {
            return true;
        }
        if (strncmp(url, "b://", 4) == 0) {
            return true;
        }
        return false;
    }

    // Validate scheme
    size_t scheme_len = scheme_end - url;
    if (scheme_len == 4 && strncmp(url, "http", 4) == 0) {
        return true;
    }
    if (scheme_len == 5 && strncmp(url, "https", 5) == 0) {
        return true;
    }

    return false;
}

// URL sanitization - removes dangerous characters and normalizes
bool sanitize_url(char* buffer, size_t buffer_size, const char* url) {
    if (!buffer || buffer_size == 0 || !url) {
        return false;
    }

    size_t i = 0;
    size_t j = 0;

    // Skip leading whitespace
    while (url[i] && isspace(static_cast<unsigned char>(url[i]))) {
        i++;
    }

    // Copy safe characters
    while (url[i] && j < buffer_size - 1) {
        unsigned char c = static_cast<unsigned char>(url[i]);

        // Allow alphanumeric, basic URL characters
        if (isalnum(c) || c == ':' || c == '/' || c == '.' || c == '-' ||
            c == '_' || c == '~' || c == '?' || c == '=' || c == '&' ||
            c == '%' || c == '+' || c == '#') {
            buffer[j++] = c;
        }
        // Replace control characters and other unsafe chars with nothing
        else if (c < 32 || c == 127) {
            // Skip control characters
        }
        else {
            // For other characters, percent-encode could be added here
            // For now, skip them
        }
        i++;
    }

    buffer[j] = '\0';

    // Ensure URL has scheme if needed
    if (j > 0 && strstr(buffer, "://") == nullptr && strncmp(buffer, "about:", 6) != 0 && strncmp(buffer, "b://", 4) != 0) {
        // Assume http if no scheme and not about/b protocol
        if (j + 7 < buffer_size) {
            memmove(buffer + 7, buffer, j + 1);
            memcpy(buffer, "http://", 7);
            j += 7;
        }
    }

    return j > 0;
}

void nav_set_listener(NavCallback callback) {
    g_listener = callback;
}

const char* nav_get_url() {
    return g_url_bar;
}

void nav_set_url(const char* u) {
    if (!u) return;

    // Validate and sanitize URL
    if (!is_valid_url(u)) {
        orbisfox::log("PS4b Nav: Invalid URL rejected: %s", u);
        return;
    }

    char sanitized[MAX_URL];
    if (!sanitize_url(sanitized, MAX_URL, u)) {
        orbisfox::log("PS4b Nav: URL sanitization failed: %s", u);
        return;
    }

    strncpy(g_url_bar, sanitized, MAX_URL - 1);
    g_url_bar[MAX_URL - 1] = '\0';
    orbisfox::log("PS4b Nav: Set URL: %s", g_url_bar);

    // Append to history stack if not duplicate of active
    if (g_active < 0 || strcmp(g_pages[g_active].url, g_url_bar) != 0) {
        if (g_active + 1 < static_cast<int>(sizeof(g_pages) / sizeof(g_pages[0]))) {
            g_active++;
            strncpy(g_pages[g_active].url, g_url_bar, MAX_URL - 1);
            g_pages[g_active].url[MAX_URL - 1] = '\0';
            g_pages[g_active].loaded = true;
            g_num_pages = g_active + 1;
        }
    }

    if (g_listener) {
        g_listener(g_url_bar);
    }
}

bool nav_back() {
    if (g_active > 0) {
        g_active--;
        strncpy(g_url_bar, g_pages[g_active].url, MAX_URL - 1);
        g_url_bar[MAX_URL - 1] = '\0';
        orbisfox::log("PS4b Nav: Back to %s", g_url_bar);
        if (g_listener) g_listener(g_url_bar);
        return true;
    }
    orbisfox::log("PS4b Nav: No back history");
    return false;
}

bool nav_forward() {
    if (g_active + 1 < g_num_pages) {
        g_active++;
        strncpy(g_url_bar, g_pages[g_active].url, MAX_URL - 1);
        g_url_bar[MAX_URL - 1] = '\0';
        orbisfox::log("PS4b Nav: Forward to %s", g_url_bar);
        if (g_listener) g_listener(g_url_bar);
        return true;
    }
    orbisfox::log("PS4b Nav: No forward history");
    return false;
}

bool nav_reload() {
    if (g_url_bar[0]) {
        orbisfox::log("PS4b Nav: Reload %s", g_url_bar);
        if (g_listener) g_listener(g_url_bar);
        return true;
    }
    return false;
}

bool nav_home() {
    nav_set_url("about:blank");
    return true;
}

void nav_set_debug_enabled(bool enabled) {
    g_debug_enabled = enabled;
    orbisfox::log("PS4b Nav: Debug interface %s", enabled ? "enabled" : "disabled");
}

bool nav_is_debug_enabled() {
    return g_debug_enabled;
}

} // namespace ps4b

// JavaScript exported C callables for webpage interaction
extern "C" {
bool js_load(const char* u) {
    if (!ps4b::is_valid_url(u)) {
        orbisfox::log("PS4b JS: Invalid URL rejected: %s", u);
        return false;
    }
    ps4b::nav_set_url(u);
    return true;
}

bool js_back() {
    return ps4b::nav_back();
}

bool js_forward() {
    return ps4b::nav_forward();
}

bool js_reload() {
    return ps4b::nav_reload();
}

bool js_home() {
    return ps4b::nav_home();
}

void js_toggle_debug(bool enabled) {
    ps4b::nav_set_debug_enabled(enabled);
}
}
