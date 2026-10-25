#include "browser_navigation.h"
#include "ps4b/presenter.h"
#include <string.h>

namespace ps4b {

static PageEntry g_pages[32];
static int g_num_pages = 0;
static int g_active = -1;
static char g_url_bar[MAX_URL] = "http://about:blank";
static NavCallback g_listener = nullptr;

void nav_set_listener(NavCallback callback) {
    g_listener = callback;
}

const char* nav_get_url() {
    return g_url_bar;
}

void nav_set_url(const char* u) {
    if (!u) return;

    strncpy(g_url_bar, u, MAX_URL - 1);
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

} // namespace ps4b

// JavaScript exported C callables for webpage interaction
extern "C" {
bool js_load(const char* u) {
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
}
