#include "litehtml_wrapper.h"
#include <litehtml/html.h>
#include <litehtml/borders.h>
#include <litehtml/style.h>
#include <litehtml/el_base.h>
#include <litehtml/el_div.h>
#include <litehtml/el_text.h>
#include <litehtml/el_image.h>
#include <litehtml/el_anchor.h>
#include <litehtml/el_script.h>
#include <litehtml/flex_item.h>
#include <litehtml/flex_line.h>
#include <litehtml/render_item.h>
#include <litehtml/render_block.h>
#include <litehtml/render_flex.h>
#include <litehtml/render_inline.h>
#include <litehtml/render_table.h>
#include <litehtml/scroll_view.h>
#include <litehtml/formatting_context.h>
#include <litehtml/css_length.h>
#include <litehtml/css_margins.h>
#include <litehtml/css_position.h>
#include <litehtml/css_selector.h>
#include <litehtml/css_tokenizer.h>
#include <litehtml/css_values.h>
#include <litehtml/css_properties.h>
#include <litehtml/css_parser.h>
#include <litehtml/master_css.h>
#include <litehtml/stylesheet.h>
#include <litehtml/encoding.h>
#include <litehtml/pixel_type.h>
#include <litehtml/iterators.h>
#include <litehtml/background.h>
#include <litehtml/font_description.h>
#include <litehtml/num_cvt.h>
#include <litehtml/encodings.h>
#include <litehtml/url.h>
#include <litehtml/url_path.h>
#include <litehtml/codepoint.h>
#include <litehtml/string_id.h>
#include <litehtml/types.h>

#include <orbis/libkernel.h>
#include <orbis/graphics.h>
#include <orbis/video.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace ps4_browser {

litehtml_renderer::litehtml_renderer()
    : m_width(0), m_height(0), m_doc(nullptr), m_render(nullptr) {
}

litehtml_renderer::~litehtml_renderer() {
    if (m_render) {
        // render is managed by the document, no explicit delete needed
    }
}

bool litehtml_renderer::init(int width, int height) {
    m_width = width;
    m_height = height;

    // Create the litehtml document
    m_doc = litehtml::document::create();
    if (!m_doc) {
        return false;
    }

    // Create the render object
    m_render = m_doc->create_render();
    if (!m_render) {
        return false;
    }

    // Set the window size
    m_render->set_width(width);
    m_render->set_height(height);

    // Initialize with default white background
    m_render->set_bg_color(litehtml::color(255, 255, 255));

    return true;
}

bool litehtml_renderer::load_html(const std::string& html) {
    if (!m_doc) {
        return false;
    }

    // Parse the HTML content
    m_doc->parse(html.c_str(), html.size(), nullptr, 0);

    return true;
}

bool litehtml_renderer::load_css(const std::string& css_path) {
    if (!m_doc) {
        return false;
    }

    // Read CSS file from path
    FILE* fp = fopen(css_path.c_str(), "rb");
    if (!fp) {
        return false;
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Read file content
    std::string css_content(fsize, '\0');
    fread(&css_content[0], 1, fsize, fp);
    fclose(fp);

    // Parse and add CSS to document
    m_doc->parse_css(css_content.c_str(), css_content.size());

    return true;
}

bool litehtml_renderer::load_image(const std::string& image_path) {
    if (!m_doc) {
        return false;
    }

    // Load image from file
    litehtml::image::ptr img = litehtml::image::create_from_file(image_path.c_str());
    if (!img) {
        return false;
    }

    // Add image to document
    m_doc->add_image(img);

    return true;
}

void litehtml_renderer::render(uint8_t* framebuffer, int fb_width, int fb_height, const litehtml::rect* clip_rect) {
    if (!m_doc || !m_render || !framebuffer) {
        return;
    }

    // Render the HTML
    m_render->render();

    // Get the rendered area
    int rendered_width = m_render->width();
    int rendered_height = m_render->height();

    // Determine clipping rectangle
    int clip_x = 0, clip_y = 0, clip_w = rendered_width, clip_h = rendered_height;
    if (clip_rect) {
        clip_x = clip_rect->left;
        clip_y = clip_rect->top;
        clip_w = clip_rect->right - clip_rect->left;
        clip_h = clip_rect->bottom - clip_rect->top;
        if (clip_w < 0) clip_w = 0;
        if (clip_h < 0) clip_h = 0;
    }

    // Clip to framebuffer dimensions
    int copy_width = (clip_w < fb_width) ? clip_w : fb_width;
    int copy_height = (clip_h < fb_height) ? clip_h : fb_height;

    // Adjust source start position based on clip rect
    int src_start_x = clip_x;
    int src_start_y = clip_y;

    // Copy the rendered pixels to the framebuffer
    // litehtml uses BGRA order, we'll copy accordingly
    // Optimized: use row pointers for better cache performance
    for (int y = 0; y < copy_height; y++) {
        // Get source row pointer
        auto* row_ptr = m_render->pixel(src_start_x, src_start_y + y);
        // Calculate destination row stride
        int fb_row_idx = ((clip_y + y) * fb_width + clip_x) * 4;
        
        // Copy each pixel in the row
        for (int x = 0; x < copy_width; x++) {
            // Assuming BGRA order from litehtml
            framebuffer[fb_row_idx + 0] = row_ptr[x].b;  // B
            framebuffer[fb_row_idx + 1] = row_ptr[x].g;  // G
            framebuffer[fb_row_idx + 2] = row_ptr[x].r;  // R
            framebuffer[fb_row_idx + 3] = row_ptr[x].a;  // A
            fb_row_idx += 4;
        }
    }
}

void litehtml_renderer::resize(int width, int height) {
    m_width = width;
    m_height = height;
    if (m_render) {
        m_render->set_width(width);
        m_render->set_height(height);
    }
}

} // namespace ps4_browser