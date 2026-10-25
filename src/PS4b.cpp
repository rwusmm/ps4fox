// PS4b Web Browser
#include "ps4b/presenter.h"
#include "ps4b/surface.h"
#include "ps4b/artwork.h"
#include "ps4b/ps4_services.h"
#include "ps4/html_renderer.h"
#include "net/http_client.h"
#include "browser_navigation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#if __has_include(<orbis/libkernel.h>)
#include <orbis/libkernel.h>
#endif

using orbisfox::Surface;
using orbisfox::Presenter;

namespace {

static ps4_browser::litehtml_renderer g_renderer;

void on_page_navigate(const char* url) {
    uint8_t* payload = nullptr;
    size_t length = 0;

    if (ps4b::http_fetch(url, &payload, &length) && payload && length > 0) {
        std::string html(reinterpret_cast<char*>(payload), length);
        free(payload);
        g_renderer.load_html(html, url);
    } else {
        std::string error_page =
            "<!DOCTYPE html><html><body style='background:#131b2f;color:#e1e9f8;padding:24px;font-family:sans-serif;'>"
            "<h1 style='color:#e63946;border-bottom:1px solid #3a506b;padding-bottom:8px;'>Navigation Failed</h1>"
            "<p>Unable to retrieve: <code>" + std::string(url) + "</code></p>"
            "<p style='color:#94a3b8;'>Please verify the URL or network status.</p>"
            "</body></html>";
        g_renderer.load_html(error_page, url);
    }
}

void draw_browser_frame(Surface& s) {
    // Fill surface base background
    orbisfox::fill_rect(s, 0, 0, s.width, s.height, {19, 27, 47, 255});

    // Top address & navigation bar
    orbisfox::fill_rect(s, 0, 0, s.width, 38, {11, 19, 43, 255});
    orbisfox::fill_rect(s, 10, 6, s.width - 20, 26, {28, 37, 65, 255});

    // Render HTML & CSS document content below top bar
    if (g_renderer.is_initialized()) {
        g_renderer.render(s.pixels, s.width, s.height, nullptr);
    }
}

} // namespace

class PS4bPres : public Presenter {
public:
    bool init(int w, int h) override {
        ps4b::net_init();
        g_renderer.init(w, h);
        ps4b::nav_set_listener(on_page_navigate);
        ps4b::nav_set_url("about:blank");
        orbisfox::log("PS4b Presenter: Initialized (%dx%d) [HTTP:%s TLS:%s]",
                      w, h,
                      ps4b::is_net_ready() ? "active" : "offline",
                      ps4b::is_ssl_ready() ? "active" : "offline");
        return true;
    }

    bool present(const Surface& s) override {
        draw_browser_frame(const_cast<Surface&>(s));
        return true;
    }

    void stop() override {
        ps4b::net_shutdown();
    }

    const char* name() const override {
        return "PS4b Browser";
    }

    const char* details() const override {
        return "LiteHTML+CSS3/TLS-HTTP";
    }
};

static PS4bPres g_presenter;

extern "C" Presenter& ps4b_presenter() {
    return g_presenter;
}

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    orbisfox::log("PS4b Browser starting...");

    constexpr int width = 1280;
    constexpr int height = 720;

    auto* pixels = static_cast<uint8_t*>(malloc(static_cast<size_t>(width) * height * 4));
    if (!pixels) {
        orbisfox::log("PS4b: Failed to allocate framebuffer");
        return EXIT_FAILURE;
    }

    Surface surface{pixels, width, height, static_cast<size_t>(width) * 4};

    Presenter* presenter = &ps4b_presenter();
    if (!presenter->init(width, height)) {
        orbisfox::log("PS4b: Presenter initialization failed");
        free(pixels);
        return EXIT_FAILURE;
    }

    // Initial frame presentation
    if (!presenter->present(surface)) {
        orbisfox::log("PS4b: First frame presentation failed");
    }

    // Main rendering loop
    constexpr int max_frames = 120;
    for (int frame = 0; frame < max_frames; ++frame) {
        if (!presenter->present(surface)) {
            break;
        }
#if __has_include(<orbis/libkernel.h>)
        sceKernelUsleep(16000); // ~60fps
#endif
    }

    presenter->stop();
    free(pixels);
    orbisfox::log("PS4b Browser exiting cleanly");
    return EXIT_SUCCESS;
}
