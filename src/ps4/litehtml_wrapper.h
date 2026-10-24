#pragma once

#include <litehtml.h>
#include <litehtml/element.h>
#include <litehtml/document.h>
#include <litehtml/render.h>
#include <memory>
#include <string>

namespace ps4_browser {

class litehtml_renderer {
public:
    litehtml_renderer();
    ~litehtml_renderer();

    // Initialize litehtml with the given width/height
    bool init(int width, int height);

    // Load HTML content for rendering
    bool load_html(const std::string& html);

    // Load CSS content for styling
    bool load_css(const std::string& css_path);

    // Load image for rendering
    bool load_image(const std::string& image_path);

    // Render the HTML content to the surface buffer
    void render(uint8_t* framebuffer, int fb_width, int fb_height, const litehtml::rect* clip_rect = nullptr);

    // Resize the renderer
    void resize(int width, int height);

    // Get the rendered width/height
    int get_width() const { return m_width; }
    int get_height() const { return m_height; }

    // Check if initialized
    bool is_initialized() const { return m_doc != nullptr; }

private:
    int m_width;
    int m_height;
    litehtml::document::ptr m_doc;
    std::unique_ptr<litehtml::render> m_render;
};

} // namespace ps4_browser
