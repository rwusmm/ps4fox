#include "orbisfox/presenter.h"
#include "orbisfox/artwork.h"
#include "orbisfox/ps4_services.h"
#include "orbisfox/swgl_canvas.h"
#include "orbisfox/touch_cursor.h"
#include "orbisfox/rust_runtime.h"
#include "orbisfox/stick_cursor.h"
#include "orbisfox/js_canvas.h"
#include "ps4_browser/litehtml_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <orbis/libkernel.h>

static int clamp(int value, int low, int high) { return value < low ? low : value > high ? high : value; }

ps4_browser::litehtml_renderer html_renderer;

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
#ifdef ORBISFOX_VIDEOOUT_ONLY
    constexpr bool videoout_only = true;
#else
    constexpr bool videoout_only = false;
#endif
    Diagnostics diagnostics = run_platform_probes(videoout_only);
    constexpr int width = 1280, height = 720;
    auto* pixels = static_cast<uint8_t*>(malloc((size_t)width * height * 4));
    if (!pixels) { log("Surface allocation failed"); close_log(); return EXIT_FAILURE; }
    Surface surface{pixels, width, height, (size_t)width * 4};
#ifdef ORBISFOX_VIDEOOUT_ONLY
    Presenter* presenter = &videoout_presenter();
    if (!presenter->init(width, height)) {
        log("VideoOut-only initialization failed"); free(pixels); close_log(); return EXIT_FAILURE;
    }
#else
    // 0.03 blocks in Piglet's first eglSwapBuffers on the tested console.
    // Keep the operational VideoOut path as the default, not a blocked input loop.
    Presenter* presenter = &videoout_presenter();
    bool test_piglet = access("/data/orbisfox/force-piglet", F_OK) == 0
        && access("/data/orbisfox/force-videoout", F_OK) != 0;
    if (test_piglet) {
        log("Experimental Piglet requested by marker");
        presenter = &piglet_presenter();
        if (!presenter->init(width, height)) presenter = &videoout_presenter();
    }
    if (presenter == &videoout_presenter() && !presenter->init(width, height)) {
        log("No usable presenter"); free(pixels); close_log(); return EXIT_FAILURE;
    }
#endif
    diagnostics.renderer = presenter->name(); diagnostics.graphics = presenter->details();

    // Initialize litehtml renderer
    if (!html_renderer.init(width, height)) {
        log("Litehtml initialization failed");
    } else {
        log("Litehtml renderer initialized");
    }

    int page = 0, x = width / 2, y = height / 2;
    // Prove graphics independently of UserService/Pad initialization. In 0.02
    // an input-service crash prevented us from submitting even a single frame.
    log("First UI frame: CPU drawing begin - BEFORE controller initialization");
    bool splash_loaded = load_artwork("/app0/assets/splash.rgba", surface);
    log("OrbisFox splash artwork: %s", splash_loaded ? "loaded" : "unavailable - using UI fallback");
    if (!splash_loaded) draw_probe(surface, diagnostics, page, x, y);
    log("First UI frame: presenter begin");
    if (!presenter->present(surface)) {
        log("First UI frame failed");
        presenter->stop(); free(pixels); close_log(); return EXIT_FAILURE;
    }
    log("First UI frame: presented successfully");
    hide_splash_screen();
    // Display the user's artwork ourselves as well as shipping the PS4 menu
    // pictures. Keep the system splash until a complete application frame exists.
    if (splash_loaded) sceKernelUsleep(900000);

    // Load a basic HTML page for demonstration
    if (html_renderer.is_initialized()) {
        std::string basic_html = "<html><body><h1>PS4 Browser Litehtml</h1>"
                                 "<p>Welcome to the PS4 browser using litehtml!</p>"
                                 "<link rel=\"stylesheet\" href=\"/app0/style.css\"/>"
                                 "<img src=\"/app0/logo.rgba\" alt=\"Logo\"/>"
                                 "</body></html>";
        html_renderer.load_html(basic_html);

        // Try to load CSS if it exists
        if (access("/app0/style.css", F_OK) == 0) {
            html_renderer.load_css("/app0/style.css");
        }

        // Try to load image if it exists
        if (access("/app0/logo.rgba", F_OK) == 0) {
            html_renderer.load_image("/app0/logo.rgba");
        }
    }

    // URL bar state - demo placeholder
    std::string url_bar = "http://";

    // History stack for back/forward navigation
    #define MAX_HISTORY 32
    struct history_entry {
        std::string url;
        std::string title;
    };
    history_entry history_stack[MAX_HISTORY];
    int history_pos = -1;  // last entry index
    int forward_pos = -1;  // for forward navigation

    Controller controller;
    if (access("/data/orbisfox/skip-controller", F_OK) == 0) {
        log("Controller: skipped by /data/orbisfox/skip-controller marker");
        diagnostics.controller = "DISABLED BY MARKER";
    } else {
        bool connected = controller.init();
        diagnostics.controller = connected ? "PAD HANDLE OPEN" : "UNAVAILABLE";
        if (!connected) {
            log("Controller not available");
        }
    }

    // Main loop
    bool running = true;
    while (running) {
        // Process events
        sceCtrlData pad;
        memset(&pad, 0, sizeof(pad));
        int ret = sceCtrlPeekPositive(0, &pad, 1);
        if (ret >= 0) {
            // Check for cross (PS button) to exit
            if (pad.button & SCE_CTRL_CROSS) {
                running = false;
            }
            // Check for circle to go back
            if (pad.button & SCE_CTRL_CIRCLE) {
                // Handle back navigation
                if (html_renderer.is_initialized() && history_pos >= 0) {
                    // Save current position to forward stack before going back
                    if (forward_pos < MAX_HISTORY - 1) {
                        forward_pos++;
                        if (history_pos + 1 < MAX_HISTORY) {
                            history_stack[forward_pos].url = history_stack[history_pos].url;
                            history_stack[forward_pos].title = history_stack[history_pos].title;
                        }
                    }
                    history_pos--;
                    if (history_pos >= 0) {
                        // Navigate back in history
                        url_bar = history_stack[history_pos].url;
                        std::string nav_html = "<html><body><h1>PS4 Browser Litehtml</h1>"
                                               "<p>Back: " + history_stack[history_pos].url + "</p>"
                                               "<form method=\"get\"><input type=\"text\" name=\"url\" value=\"" + url_bar + "\"/></form>"
                                               "</body></html>";
                        html_renderer.load_html(nav_html);
                    }
                }
            }
            // Check for square to follow link (navigate to URL)
            if (pad.button & SCE_CTRL_SQUARE) {
                if (html_renderer.is_initialized()) {
                    // Navigate to URL bar content
                    if (!url_bar.empty()) {
                        // Clear forward stack when navigating to a new page
                        forward_pos = -1;
                        
                        // Add to history if it's a new navigation
                        if (history_pos < MAX_HISTORY - 1) {
                            history_pos++;
                            if (history_pos >= 0) {
                                // Only add if we're not at the current position
                                bool new_entry = true;
                                if (history_pos > 0) {
                                    new_entry = (history_stack[history_pos-1].url.compare(url_bar) != 0);
                                }
                                if (new_entry) {
                                    history_stack[history_pos].url = url_bar;
                                    history_stack[history_pos].title = url_bar;  // simple title extraction
                                } else {
                                    // Reload current position's title
                                    history_stack[history_pos].title = history_stack[history_pos-1].title;
                                }
                            }
                        }
                        
                        std::string nav_html = "<html><body><h1>PS4 Browser Litehtml</h1>"
                                               "<p>Navigating to: " + url_bar + "</p>"
                                               "<form method=\"get\"><input type=\"text\" name=\"url\" value=\"" + url_bar + "\"/></form>"
                                               "<p>Square: Link clicked</p>"
                                               "</body></html>";
                        html_renderer.load_html(nav_html);
                    }
                }
            }
            // Check for up/down for scrolling
            if (pad.button & SCE_CTRL_UP) {
                if (html_renderer.is_initialized()) {
                    // Scroll up - adjust viewport for litehtml
                    // m_render->scroll(0, -20);
                }
            }
            if (pad.button & SCE_CTRL_DOWN) {
                if (html_renderer.is_initialized()) {
                    // Scroll down - adjust viewport for litehtml
                    // m_render->scroll(0, 20);
                }
            }
            if (pad.button & SCE_CTRL_LEFT) {
                if (html_renderer.is_initialized()) {
                    // Scroll left - adjust viewport for litehtml
                    // m_render->scroll(-20, 0);
                }
            }
            if (pad.button & SCE_CTRL_RIGHT) {
                if (html_renderer.is_initialized()) {
                    // Scroll right - adjust viewport for litehtml
                    // m_render->scroll(20, 0);
                }
            }
            // Touchpad button (press down on touchpad)
            if (pad.button & SCE_CTRL_TOUCH) {
                if (html_renderer.is_initialized()) {
                    // Toggle some action or show touch interface
                    // For now, just note the touch event
                }
            }
            // Touchpad sliding (gesture)
            if (pad.psm) {
                // psm (pointing stick module) data available
                // Handle touch gestures for scrolling/navigation
                if (pad.psm->gesture == SCE_PSM_GESTURE_TAP) {
                    // Tap gesture - could be used for link clicks
                    if (html_renderer.is_initialized()) {
                        // TODO: Implement tap handling
                    }
                }
                if (pad.psm->gesture == SCE_PSM_GESTURE_SWIPE_LEFT) {
                    // Swipe left - navigate back
                    if (html_renderer.is_initialized() && history_pos > 0) {
                        history_pos--;
                        if (history_pos >= 0) {
                            url_bar = history_stack[history_pos].url;
                            std::string nav_html = "<html><body><h1>PS4 Browser Litehtml</h1>"
                                                   "<p>Swipe Back: " + history_stack[history_pos].url + "</p>"
                                                   "<form method=\"get\"><input type=\"text\" name=\"url\" value=\"" + url_bar + "\"/></form>"
                                                   "</body></html>";
                            html_renderer.load_html(nav_html);
                        }
                    }
                }
                if (pad.psm->gesture == SCE_PSM_GESTURE_SWIPE_RIGHT) {
                    // Swipe right - navigate forward
                    if (html_renderer.is_initialized() && forward_pos >= 0) {
                        url_bar = history_stack[forward_pos].url;
                        forward_pos--;
                        if (forward_pos >= -1) {
                            std::string nav_html = "<html><body><h1>PS4 Browser Litehtml</h1>"
                                   "<p>Swipe Forward: " + (forward_pos >= 0 ? history_stack[forward_pos].url : url_bar) + "</p>"
                                   "<form method=\"get\"><input type=\"text\" name=\"url\" value=\"" + url_bar + "\"/></form>"
                                   "</body></html>";
                            html_renderer.load_html(nav_html);
                        }
                    }
                }
                if (pad.psm->gesture == SCE_PSM_GESTURE_SWIPE_UP) {
                    // Swipe up - scroll up
                    if (html_renderer.is_initialized()) {
                        // TODO: Implement scroll up
                    }
                }
                if (pad.psm->gesture == SCE_PSM_GESTURE_SWIPE_DOWN) {
                    // Swipe down - scroll down
                    if (html_renderer.is_initialized()) {
                        // TODO: Implement scroll down
                    }
                }
            }
        }

        // Render litehtml if initialized
        if (html_renderer.is_initialized()) {
            html_renderer.render(pixels, width, height, nullptr);
        }

        // Present the frame
        if (!presenter->present(surface)) {
            log("Presentation failed");
            break;
        }

        sceKernelUsleep(16000);
    }

    // Shutdown
    controller.stop();
    html_renderer.~litehtml_renderer();
    presenter->stop();
    free(pixels);
    log("OrbisFox exiting: 0");
    close_log();
    return EXIT_SUCCESS;
}