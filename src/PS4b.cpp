// PS4b Web Browser
#include "ps4b/presenter.h"
#include "ps4b/surface.h"
#include "ps4b/artwork.h"
#include "ps4b/ps4_services.h"
#include "ps4b/touch_cursor.h"
#include "ps4b/stick_cursor.h"
#include "ps4/html_renderer.h"
#include "net/http_client.h"
#include "browser_navigation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <time.h>

#if __has_include(<orbis/libkernel.h>)
#include <orbis/libkernel.h>
#endif
#if __has_include(<orbis/Pad.h>)
#include <orbis/Pad.h>
#endif

using orbisfox::Surface;
using orbisfox::Presenter;

namespace {

static ps4_browser::litehtml_renderer g_renderer;
static Presenter* g_hardware_presenter = nullptr;
static constexpr int TOP_BAR_HEIGHT = 38;

// FPS tracking
static uint64_t g_frame_count = 0;
static uint64_t g_last_time = 0;
static float g_fps = 0.0f;

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
            "<p>Unable to retrieve: <code>" + std::string(url ? url : "") + "</code></p>"
            "<p style='color:#94a3b8;'>Please verify the URL or network status.</p>"
            "</body></html>";
        g_renderer.load_html(error_page, url ? url : "");
    }
}

void draw_browser_frame(Surface& s, int cursor_x, int cursor_y) {
    // Fill surface base background
    orbisfox::fill_rect(s, 0, 0, s.width, s.height, {19, 27, 47, 255});

    // Render HTML & CSS document content below top bar
    if (g_renderer.is_initialized()) {
        g_renderer.render(s.pixels, s.width, s.height, 0, TOP_BAR_HEIGHT, nullptr);
    }

    // Top address & navigation bar overlay
    orbisfox::fill_rect(s, 0, 0, s.width, TOP_BAR_HEIGHT, {11, 19, 43, 255});
    orbisfox::fill_rect(s, 10, 6, s.width - 20, 26, {28, 37, 65, 255});

    // URL bar text display
    const char* url = ps4b::nav_get_url();
    if (url) {
        orbisfox::text(s, 20, 11, url, 2, {225, 233, 248, 255});
    }

    // Draw cursor indicator (crosshair)
    if (cursor_x >= 0 && cursor_y >= 0) {
        orbisfox::fill_rect(s, cursor_x - 6, cursor_y - 1, 13, 3, {245, 145, 45, 255});
        orbisfox::fill_rect(s, cursor_x - 1, cursor_y - 6, 3, 13, {245, 145, 45, 255});
    }

    // Handle mouse events for HTML interactivity
    g_renderer.handle_mouse_move(cursor_x, cursor_y - TOP_BAR_HEIGHT);

    // Draw debug overlay if enabled
    if (ps4b::nav_is_debug_enabled()) {
        // FPS
        char fps_text[32];
        snprintf(fps_text, sizeof(fps_text), "FPS: %.1f", g_fps);
        orbisfox::fill_rect(s, 15, 15, strlen(fps_text) * 8 + 12, 20, {0, 0, 0, 191});
        orbisfox::text(s, 20, 18, fps_text, 1, {232, 232, 141, 255});

        // MB transferred
        size_t mb_transferred = ps4b::net_get_bytes_transferred() / (1024 * 1024);
        char mb_text[64];
        snprintf(mb_text, sizeof(mb_text), "MB TRANSFERED THROUGH SESSION: %zuMB", mb_transferred);
        orbisfox::fill_rect(s, 15, 40, strlen(mb_text) * 8 + 12, 20, {0, 0, 0, 191});
        orbisfox::text(s, 20, 43, mb_text, 1, {255, 123, 92, 255});

        // Current technology
        const char* tech = ps4b::net_get_current_tech();
        char tech_text[32];
        snprintf(tech_text, sizeof(tech_text), "CURRENT TECH: %s", tech);
        orbisfox::fill_rect(s, 15, 65, strlen(tech_text) * 8 + 12, 20, {0, 0, 0, 191});
        orbisfox::text(s, 20, 68, tech_text, 1, {160, 224, 232, 255});
    }
}

} // namespace

class PS4bPres : public Presenter {
public:
    bool init(int w, int h) override {
        ps4b::net_init();
        g_renderer.init(w, h - TOP_BAR_HEIGHT);
        ps4b::nav_set_listener(on_page_navigate);
        ps4b::nav_set_url("about:blank");

        // Chain hardware presenter if available (on PS4 hardware)
        g_hardware_presenter = &orbisfox::videoout_presenter();
        if (!g_hardware_presenter->init(w, h)) {
            orbisfox::log("PS4b: Hardware VideoOut presenter unavailable; running in software buffer mode");
            g_hardware_presenter = nullptr;
        } else {
            orbisfox::log("PS4b: Hardware VideoOut presenter successfully initialized");
        }

        orbisfox::log("PS4b Presenter: Initialized (%dx%d) [HTTP:%s TLS:%s]",
                      w, h,
                      ps4b::is_net_ready() ? "active" : "offline",
                      ps4b::is_ssl_ready() ? "active" : "offline");
        return true;
    }

    bool present(const Surface& s) override {
        // Submit rendered surface to display hardware when active
        if (g_hardware_presenter) {
            return g_hardware_presenter->present(s);
        }
        return true;
    }

    void stop() override {
        if (g_hardware_presenter) {
            g_hardware_presenter->stop();
            g_hardware_presenter = nullptr;
        }
        ps4b::net_shutdown();
    }

    const char* name() const override {
        return "PS4b Browser";
    }

    const char* details() const override {
        return "LiteHTML+CSS3/TLS-HTTP+VideoOut";
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

    orbisfox::hide_splash_screen();

    // Initialize controller for interactive navigation
    orbisfox::Controller controller;
    bool controller_connected = controller.init();
    orbisfox::log("PS4b: Controller status: %s", controller_connected ? "connected" : "unavailable");

    orbisfox::TouchCursor touch;
    touch.set_resolution(controller.touch_resolution_x(), controller.touch_resolution_y());

    orbisfox::StickCursor1_5 stick_x, stick_y;
    int cursor_x = width / 2;
    int cursor_y = height / 2;
    uint32_t prev_buttons = 0;
    bool running = true;

    // FPS timing
    uint64_t fps_update_interval = 2000000; // 2 seconds in microseconds
    uint64_t last_fps_update = 0;
#if __has_include(<orbis/libkernel.h>)
    last_fps_update = sceKernelGetProcessTime();
#else
    last_fps_update = 0;
#endif

    // Interactive rendering and event loop
    while (running) {
        g_frame_count++;

#if __has_include(<orbis/libkernel.h>)
        uint64_t current_time = sceKernelGetProcessTime();
#else
        uint64_t current_time = g_frame_count * 16000;
#endif

        // Update FPS every 2 seconds
        if (current_time - last_fps_update >= fps_update_interval) {
            uint64_t elapsed_us = current_time - last_fps_update;
            g_fps = (g_frame_count * 1000000.0f) / elapsed_us;
            g_frame_count = 0;
            last_fps_update = current_time;
        }

        OrbisPadData input{};
        if (controller.poll(input)) {
            uint32_t pressed = input.buttons & ~prev_buttons;
            prev_buttons = input.buttons;

            // Options button: exit browser cleanly via system service
            if (pressed & ORBIS_PAD_BUTTON_OPTIONS) {
                orbisfox::log("PS4b: Exit requested via Options button");
                orbisfox::request_system_exit();
                running = false;
                break;
            }

            // Cross button: reload page or click element
            if (pressed & ORBIS_PAD_BUTTON_CROSS) {
                g_renderer.handle_mouse_click(cursor_x, cursor_y - TOP_BAR_HEIGHT);
                ps4b::nav_reload();
            }

            // Triangle button: go to home (about:blank)
            if (pressed & ORBIS_PAD_BUTTON_TRIANGLE) {
                ps4b::nav_home();
            }

            // L1 button: history back
            if (pressed & ORBIS_PAD_BUTTON_L1) {
                ps4b::nav_back();
            }

            // R1 button: history forward
            if (pressed & ORBIS_PAD_BUTTON_R1) {
                ps4b::nav_forward();
            }

            // Left analog stick cursor motion
            int sx = stick_x.update(input.leftStick.x);
            int sy = stick_y.update(input.leftStick.y);
            cursor_x = std::max(0, std::min(width - 1, cursor_x + sx));
            cursor_y = std::max(0, std::min(height - 1, cursor_y + sy));

            // Touchpad cursor motion
            size_t touch_count = input.touch.fingers > 2 ? 2 : input.touch.fingers;
            if (touch_count > 0) {
                orbisfox::TouchPoint points[2];
                for (size_t i = 0; i < touch_count; ++i) {
                    points[i].x = input.touch.touch[i].x;
                    points[i].y = input.touch.touch[i].y;
                    points[i].id = input.touch.touch[i].finger;
                }
                touch.update(points, touch_count, width, height, cursor_x, cursor_y);
            }
        }

        // Draw frame onto surface buffer
        draw_browser_frame(surface, cursor_x, cursor_y);

        // Submit surface through presenter (hardware VideoOut scanout)
        if (!presenter->present(surface)) {
            orbisfox::log("PS4b: Frame presentation failed");
            break;
        }

#if __has_include(<orbis/libkernel.h>)
        sceKernelUsleep(16000); // ~60fps target rate
#endif
    }

    presenter->stop();
    controller.stop();
    free(pixels);
    orbisfox::log("PS4b Browser exiting cleanly");
    return EXIT_SUCCESS;
}
