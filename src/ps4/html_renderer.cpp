#include "html_renderer.h"
#include <litehtml.h>
#include <litehtml/document_container.h>
#include <litehtml/master_css.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <algorithm>
#include <map>
#include <vector>

namespace ps4_browser {

namespace {

// Standard compact 8x8 ASCII font bitmap (ASCII 32 to 126)
static const uint8_t font8x8_basic[96][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 32 (space)
    {0x18, 0x3c, 0x3c, 0x18, 0x18, 0x00, 0x18, 0x00}, // 33 !
    {0x66, 0x66, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00}, // 34 "
    {0x6c, 0x6c, 0xfe, 0x6c, 0xfe, 0x6c, 0x6c, 0x00}, // 35 #
    {0x18, 0x3e, 0x60, 0x3c, 0x06, 0x7c, 0x18, 0x00}, // 36 $
    {0x00, 0x63, 0x66, 0x0c, 0x18, 0x33, 0x63, 0x00}, // 37 %
    {0x1c, 0x36, 0x1c, 0x3b, 0x6e, 0x66, 0x3b, 0x00}, // 38 &
    {0x18, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00}, // 39 '
    {0x0c, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0c, 0x00}, // 40 (
    {0x30, 0x18, 0x0c, 0x0c, 0x0c, 0x18, 0x30, 0x00}, // 41 )
    {0x00, 0x66, 0x3c, 0xff, 0x3c, 0x66, 0x00, 0x00}, // 42 *
    {0x00, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x00, 0x00}, // 43 +
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x08}, // 44 ,
    {0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00}, // 45 -
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00}, // 46 .
    {0x00, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x00}, // 47 /
    {0x3c, 0x66, 0x6e, 0x76, 0x66, 0x66, 0x3c, 0x00}, // 48 0
    {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x00}, // 49 1
    {0x3c, 0x66, 0x06, 0x0c, 0x18, 0x30, 0x7e, 0x00}, // 50 2
    {0x3c, 0x66, 0x06, 0x1c, 0x06, 0x66, 0x3c, 0x00}, // 51 3
    {0x06, 0x0e, 0x1e, 0x36, 0x66, 0x7f, 0x06, 0x00}, // 52 4
    {0x7e, 0x60, 0x7c, 0x06, 0x06, 0x66, 0x3c, 0x00}, // 53 5
    {0x1c, 0x30, 0x60, 0x7c, 0x66, 0x66, 0x3c, 0x00}, // 54 6
    {0x7e, 0x66, 0x06, 0x0c, 0x18, 0x18, 0x18, 0x00}, // 55 7
    {0x3c, 0x66, 0x66, 0x3c, 0x66, 0x66, 0x3c, 0x00}, // 56 8
    {0x3c, 0x66, 0x66, 0x3e, 0x06, 0x0c, 0x38, 0x00}, // 57 9
    {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00}, // 58 :
    {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x08, 0x00}, // 59 ;
    {0x06, 0x0c, 0x18, 0x30, 0x18, 0x0c, 0x06, 0x00}, // 60 <
    {0x00, 0x00, 0x7e, 0x00, 0x7e, 0x00, 0x00, 0x00}, // 61 =
    {0x60, 0x30, 0x18, 0x0c, 0x18, 0x30, 0x60, 0x00}, // 62 >
    {0x3c, 0x66, 0x0c, 0x18, 0x18, 0x00, 0x18, 0x00}, // 63 ?
    {0x3c, 0x66, 0x6e, 0x6e, 0x60, 0x62, 0x3c, 0x00}, // 64 @
    {0x18, 0x3c, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x00}, // 65 A
    {0x7c, 0x66, 0x66, 0x7c, 0x66, 0x66, 0x7c, 0x00}, // 66 B
    {0x3c, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3c, 0x00}, // 67 C
    {0x78, 0x6c, 0x66, 0x66, 0x66, 0x6c, 0x78, 0x00}, // 68 D
    {0x7e, 0x60, 0x60, 0x7c, 0x60, 0x60, 0x7e, 0x00}, // 69 E
    {0x7e, 0x60, 0x60, 0x7c, 0x60, 0x60, 0x60, 0x00}, // 70 F
    {0x3c, 0x66, 0x60, 0x6e, 0x66, 0x66, 0x3c, 0x00}, // 71 G
    {0x66, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x66, 0x00}, // 72 H
    {0x3c, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00}, // 73 I
    {0x0e, 0x06, 0x06, 0x06, 0x06, 0x66, 0x3c, 0x00}, // 74 J
    {0x66, 0x6c, 0x78, 0x70, 0x78, 0x6c, 0x66, 0x00}, // 75 K
    {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7e, 0x00}, // 76 L
    {0x63, 0x77, 0x7f, 0x6b, 0x63, 0x63, 0x63, 0x00}, // 77 M
    {0x66, 0x76, 0x7e, 0x7e, 0x6e, 0x66, 0x66, 0x00}, // 78 N
    {0x3c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00}, // 79 O
    {0x7c, 0x66, 0x66, 0x7c, 0x60, 0x60, 0x60, 0x00}, // 80 P
    {0x3c, 0x66, 0x66, 0x66, 0x6e, 0x3c, 0x0e, 0x00}, // 81 Q
    {0x7c, 0x66, 0x66, 0x7c, 0x78, 0x6c, 0x66, 0x00}, // 82 R
    {0x3c, 0x66, 0x60, 0x3c, 0x06, 0x66, 0x3c, 0x00}, // 83 S
    {0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}, // 84 T
    {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00}, // 85 U
    {0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00}, // 86 V
    {0x63, 0x63, 0x63, 0x6b, 0x7f, 0x77, 0x63, 0x00}, // 87 W
    {0x66, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x66, 0x00}, // 88 X
    {0x66, 0x66, 0x66, 0x3c, 0x18, 0x18, 0x18, 0x00}, // 89 Y
    {0x7e, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x7e, 0x00}, // 90 Z
    {0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00}, // 91 [
    {0x00, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x00}, // 92 backslash
    {0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c, 0x00}, // 93 ]
    {0x08, 0x1c, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00}, // 94 ^
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00}, // 95 _
    {0x18, 0x18, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00}, // 96 `
    {0x00, 0x00, 0x3c, 0x06, 0x3e, 0x66, 0x3b, 0x00}, // 97 a
    {0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x7c, 0x00}, // 98 b
    {0x00, 0x00, 0x3c, 0x66, 0x60, 0x66, 0x3c, 0x00}, // 99 c
    {0x06, 0x06, 0x3e, 0x66, 0x66, 0x66, 0x3b, 0x00}, // 100 d
    {0x00, 0x00, 0x3c, 0x66, 0x7e, 0x60, 0x3c, 0x00}, // 101 e
    {0x1c, 0x30, 0x78, 0x30, 0x30, 0x30, 0x30, 0x00}, // 102 f
    {0x00, 0x00, 0x3b, 0x66, 0x66, 0x3e, 0x06, 0x3c}, // 103 g
    {0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x00}, // 104 h
    {0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3c, 0x00}, // 105 i
    {0x06, 0x00, 0x06, 0x06, 0x06, 0x66, 0x3c, 0x00}, // 106 j
    {0x60, 0x60, 0x66, 0x6c, 0x78, 0x6c, 0x66, 0x00}, // 107 k
    {0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00}, // 108 l
    {0x00, 0x00, 0x66, 0x7f, 0x7f, 0x6b, 0x63, 0x00}, // 109 m
    {0x00, 0x00, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x00}, // 110 n
    {0x00, 0x00, 0x3c, 0x66, 0x66, 0x66, 0x3c, 0x00}, // 111 o
    {0x00, 0x00, 0x7c, 0x66, 0x66, 0x7c, 0x60, 0x60}, // 112 p
    {0x00, 0x00, 0x3e, 0x66, 0x66, 0x3e, 0x06, 0x06}, // 113 q
    {0x00, 0x00, 0x7c, 0x66, 0x60, 0x60, 0x60, 0x00}, // 114 r
    {0x00, 0x00, 0x3e, 0x60, 0x3c, 0x06, 0x7c, 0x00}, // 115 s
    {0x30, 0x30, 0x78, 0x30, 0x30, 0x36, 0x1c, 0x00}, // 116 t
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3b, 0x00}, // 117 u
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00}, // 118 v
    {0x00, 0x00, 0x63, 0x6b, 0x7f, 0x3e, 0x36, 0x00}, // 119 w
    {0x00, 0x00, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x00}, // 120 x
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x3e, 0x06, 0x3c}, // 121 y
    {0x00, 0x00, 0x7e, 0x0c, 0x18, 0x30, 0x7e, 0x00}, // 122 z
    {0x0e, 0x18, 0x18, 0x70, 0x18, 0x18, 0x0e, 0x00}, // 123 {
    {0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00}, // 124 |
    {0x70, 0x18, 0x18, 0x0e, 0x18, 0x18, 0x70, 0x00}, // 125 }
    {0x3b, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 126 ~
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // 127
};

} // namespace

class litehtml_renderer::container_impl : public litehtml::document_container {
public:
    uint8_t* m_target_fb = nullptr;
    int m_fb_width = 1280;
    int m_fb_height = 720;
    int m_viewport_w = 1280;
    int m_viewport_h = 720;
    std::string m_base_url;

    container_impl(int w, int h)
        : m_viewport_w(w), m_viewport_h(h), m_fb_width(w), m_fb_height(h) {}

    void set_target(uint8_t* fb, int w, int h) {
        m_target_fb = fb;
        m_fb_width = w;
        m_fb_height = h;
    }

    void put_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        if (!m_target_fb || x < 0 || x >= m_fb_width || y < 0 || y >= m_fb_height || a == 0) return;
        int idx = (y * m_fb_width + x) * 4;
        if (a == 255) {
            m_target_fb[idx + 0] = r;
            m_target_fb[idx + 1] = g;
            m_target_fb[idx + 2] = b;
            m_target_fb[idx + 3] = 255;
        } else {
            float alpha = a / 255.0f;
            float inv_alpha = 1.0f - alpha;
            m_target_fb[idx + 0] = static_cast<uint8_t>(r * alpha + m_target_fb[idx + 0] * inv_alpha);
            m_target_fb[idx + 1] = static_cast<uint8_t>(g * alpha + m_target_fb[idx + 1] * inv_alpha);
            m_target_fb[idx + 2] = static_cast<uint8_t>(b * alpha + m_target_fb[idx + 2] * inv_alpha);
            m_target_fb[idx + 3] = 255;
        }
    }

    void fill_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        int x0 = std::max(0, x);
        int y0 = std::max(0, y);
        int x1 = std::min(m_fb_width, x + w);
        int y1 = std::min(m_fb_height, y + h);
        for (int py = y0; py < y1; ++py) {
            for (int px = x0; px < x1; ++px) {
                put_pixel(px, py, r, g, b, a);
            }
        }
    }

    // document_container interface
    litehtml::uint_ptr create_font(const litehtml::font_description& descr, const litehtml::document* /*doc*/,
                                   litehtml::font_metrics* fm) override {
        int size = static_cast<int>(descr.size) > 0 ? static_cast<int>(descr.size) : 16;
        if (fm) {
            fm->ascent = static_cast<int>(size * 0.8f);
            fm->descent = static_cast<int>(size * 0.2f);
            fm->height = size;
            fm->x_height = static_cast<int>(size * 0.5f);
        }
        return static_cast<litehtml::uint_ptr>(size);
    }

    void delete_font(litehtml::uint_ptr /*hFont*/) override {}

    litehtml::pixel_t text_width(const char* text, litehtml::uint_ptr hFont) override {
        if (!text) return 0;
        int size = static_cast<int>(hFont);
        if (size <= 0) size = 16;
        float scale = size / 8.0f;
        if (scale < 1.0f) scale = 1.0f;
        float char_width = 8.0f * scale;
        return static_cast<litehtml::pixel_t>(strlen(text) * char_width);
    }

    void draw_text(litehtml::uint_ptr /*hdc*/, const char* text, litehtml::uint_ptr hFont,
                   litehtml::web_color color, const litehtml::position& pos) override {
        if (!text || !m_target_fb) return;
        int size = static_cast<int>(hFont);
        if (size <= 0) size = 16;
        float scale = size / 8.0f;
        if (scale < 1.0f) scale = 1.0f;

        int cur_x = pos.x;
        int cur_y = pos.y;

        for (size_t i = 0; text[i]; ++i) {
            uint8_t ch = static_cast<uint8_t>(text[i]);
            if (ch < 32 || ch > 126) ch = '?';
            const uint8_t* glyph = font8x8_basic[ch - 32];

            for (int r = 0; r < 8; ++r) {
                for (int c = 0; c < 8; ++c) {
                    if (glyph[r] & (1 << (7 - c))) {
                        int px0 = cur_x + static_cast<int>(c * scale);
                        int py0 = cur_y + static_cast<int>(r * scale);
                        int span = static_cast<int>(scale);
                        if (span <= 1) {
                            put_pixel(px0, py0, color.red, color.green, color.blue, color.alpha);
                        } else {
                            fill_rect(px0, py0, span, span, color.red, color.green, color.blue, color.alpha);
                        }
                    }
                }
            }
            cur_x += static_cast<int>(8 * scale);
        }
    }

    litehtml::pixel_t pt_to_px(float pt) const override {
        return static_cast<litehtml::pixel_t>(pt * 96.0f / 72.0f);
    }

    litehtml::pixel_t get_default_font_size() const override {
        return 16;
    }

    const char* get_default_font_name() const override {
        return "sans-serif";
    }

    void draw_solid_fill(litehtml::uint_ptr /*hdc*/, const litehtml::background_layer& layer,
                         const litehtml::web_color& color) override {
        fill_rect(static_cast<int>(layer.border_box.x), static_cast<int>(layer.border_box.y),
                  static_cast<int>(layer.border_box.width), static_cast<int>(layer.border_box.height),
                  color.red, color.green, color.blue, color.alpha);
    }

    void draw_borders(litehtml::uint_ptr /*hdc*/, const litehtml::borders& borders,
                      const litehtml::position& draw_pos, bool /*root*/) override {
        int top_w = static_cast<int>(borders.top.width);
        int bottom_w = static_cast<int>(borders.bottom.width);
        int left_w = static_cast<int>(borders.left.width);
        int right_w = static_cast<int>(borders.right.width);
        int dx = static_cast<int>(draw_pos.x);
        int dy = static_cast<int>(draw_pos.y);
        int dw = static_cast<int>(draw_pos.width);
        int dh = static_cast<int>(draw_pos.height);

        // Top border
        if (top_w > 0 && borders.top.color.alpha > 0) {
            fill_rect(dx, dy, dw, top_w,
                      borders.top.color.red, borders.top.color.green, borders.top.color.blue, borders.top.color.alpha);
        }
        // Bottom border
        if (bottom_w > 0 && borders.bottom.color.alpha > 0) {
            fill_rect(dx, dy + dh - bottom_w,
                      dw, bottom_w,
                      borders.bottom.color.red, borders.bottom.color.green, borders.bottom.color.blue, borders.bottom.color.alpha);
        }
        // Left border
        if (left_w > 0 && borders.left.color.alpha > 0) {
            fill_rect(dx, dy, left_w, dh,
                      borders.left.color.red, borders.left.color.green, borders.left.color.blue, borders.left.color.alpha);
        }
        // Right border
        if (right_w > 0 && borders.right.color.alpha > 0) {
            fill_rect(dx + dw - right_w, dy,
                      right_w, dh,
                      borders.right.color.red, borders.right.color.green, borders.right.color.blue, borders.right.color.alpha);
        }
    }

    void draw_linear_gradient(litehtml::uint_ptr /*hdc*/, const litehtml::background_layer& layer,
                              const litehtml::background_layer::linear_gradient& /*gradient*/) override {
        // Fallback to solid fill for linear gradients (complex API varies by litehtml version)
        fill_rect(layer.border_box.x, layer.border_box.y, layer.border_box.width, layer.border_box.height,
                  24, 32, 48, 255);
    }

    void draw_radial_gradient(litehtml::uint_ptr /*hdc*/, const litehtml::background_layer& layer,
                              const litehtml::background_layer::radial_gradient& /*gradient*/) override {
        // Fallback to solid fill for radial gradients (complex API varies by litehtml version)
        fill_rect(layer.border_box.x, layer.border_box.y, layer.border_box.width, layer.border_box.height,
                  24, 32, 48, 255);
    }

    void draw_conic_gradient(litehtml::uint_ptr /*hdc*/, const litehtml::background_layer& layer,
                             const litehtml::background_layer::conic_gradient& /*gradient*/) override {
        // Fallback to solid fill for conic gradients (complex to implement)
        fill_rect(layer.border_box.x, layer.border_box.y, layer.border_box.width, layer.border_box.height,
                  24, 32, 48, 255);
    }

    void draw_list_marker(litehtml::uint_ptr /*hdc*/, const litehtml::list_marker& marker) override {
        fill_rect(marker.pos.x, marker.pos.y + marker.pos.height / 3, 5, 5,
                  marker.color.red, marker.color.green, marker.color.blue, marker.color.alpha);
    }

    void load_image(const char* /*src*/, const char* /*baseurl*/, bool /*redraw_on_ready*/) override {}
    void get_image_size(const char* /*src*/, const char* /*baseurl*/, litehtml::size& sz) override {
        sz.width = 64;
        sz.height = 64;
    }
    void draw_image(litehtml::uint_ptr /*hdc*/, const litehtml::background_layer& layer,
                    const std::string& /*url*/, const std::string& /*base_url*/) override {
        fill_rect(layer.border_box.x, layer.border_box.y, layer.border_box.width, layer.border_box.height,
                  40, 50, 70, 255);
    }

    void get_viewport(litehtml::position& viewport) const override {
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = m_viewport_w;
        viewport.height = m_viewport_h;
    }

    void get_media_features(litehtml::media_features& media) const override {
        media.type = litehtml::media_type_screen;
        media.width = m_viewport_w;
        media.height = m_viewport_h;
        media.device_width = m_viewport_w;
        media.device_height = m_viewport_h;
        media.color = 8;
        media.color_index = 0;
        media.monochrome = 0;
        media.resolution = 96;
    }

    void get_language(std::string& language, std::string& culture) const override {
        language = "en";
        culture = "US";
    }

    litehtml::element::ptr create_element(const char* /*tag_name*/, const litehtml::string_map& /*attributes*/,
                                          const std::shared_ptr<litehtml::document>& /*doc*/) override {
        return nullptr;
    }

    void link(const std::shared_ptr<litehtml::document>& /*doc*/, const litehtml::element::ptr& /*el*/) override {}
    void on_anchor_click(const char* /*url*/, const litehtml::element::ptr& /*el*/) override {}
    void on_mouse_event(const litehtml::element::ptr& /*el*/, litehtml::mouse_event /*event*/) override {}
    void set_cursor(const char* /*cursor*/) override {}
    void transform_text(std::string& /*text*/, litehtml::text_transform /*tt*/) override {}
    void import_css(std::string& text, const std::string& /*url*/, std::string& /*baseurl*/) override {
        text.clear();
    }
    void set_clip(const litehtml::position& /*pos*/, const litehtml::border_radiuses& /*bdr_radius*/) override {}
    void del_clip() override {}
    void set_caption(const char* /*caption*/) override {}
    void set_base_url(const char* base_url) override {
        if (base_url) m_base_url = base_url;
    }
};

litehtml_renderer::litehtml_renderer()
    : m_width(1280), m_height(720), m_doc(nullptr) {
    m_container = std::make_unique<container_impl>(m_width, m_height);
}

litehtml_renderer::~litehtml_renderer() = default;

bool litehtml_renderer::init(int width, int height) {
    m_width = width;
    m_height = height;
    if (m_container) {
        m_container->m_viewport_w = width;
        m_container->m_viewport_h = height;
    }
    return true;
}

bool litehtml_renderer::load_html(const std::string& html, const std::string& base_url) {
    if (!m_container) {
        m_container = std::make_unique<container_impl>(m_width, m_height);
    }
    m_container->set_base_url(base_url.c_str());

    m_doc = litehtml::document::createFromString(html, m_container.get(), litehtml::master_css, m_user_css);
    if (!m_doc) {
        return false;
    }

    m_doc->render(m_width);
    return true;
}

bool litehtml_renderer::load_css(const std::string& css_path) {
    FILE* fp = fopen(css_path.c_str(), "rb");
    if (!fp) return false;

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    std::string css_content(fsize, '\0');
    fread(&css_content[0], 1, fsize, fp);
    fclose(fp);

    return append_css(css_content);
}

bool litehtml_renderer::append_css(const std::string& css_content) {
    m_user_css += "\n" + css_content;
    return true;
}

bool litehtml_renderer::load_image(const std::string& /*image_path*/) {
    return true;
}

void litehtml_renderer::render(uint8_t* framebuffer, int fb_width, int fb_height, int offset_x, int offset_y, const litehtml::position* clip_rect) {
    if (!m_doc || !m_container || !framebuffer) {
        return;
    }

    m_container->set_target(framebuffer, fb_width, fb_height);

    litehtml::position clip(0, 0, fb_width, fb_height);
    if (clip_rect) {
        clip = *clip_rect;
    }

    m_doc->draw(reinterpret_cast<litehtml::uint_ptr>(framebuffer), offset_x, offset_y, &clip);
}

void litehtml_renderer::resize(int width, int height) {
    m_width = width;
    m_height = height;
    if (m_container) {
        m_container->m_viewport_w = width;
        m_container->m_viewport_h = height;
    }
    if (m_doc) {
        m_doc->render(m_width);
    }
}

void litehtml_renderer::draw_box_shadow(int x, int y, int w, int h, int offset_x, int offset_y, int blur_radius, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!m_container) return;

    int shadow_x = x + offset_x;
    int shadow_y = y + offset_y;

    if (blur_radius <= 0) {
        // Simple shadow without blur
        m_container->fill_rect(shadow_x, shadow_y, w, h, r, g, b, a);
    } else {
        // Simple box blur approximation
        int blur_samples = blur_radius * 2 + 1;
        for (int dy = -blur_radius; dy <= blur_radius; ++dy) {
            for (int dx = -blur_radius; dx <= blur_radius; ++dx) {
                float dist = sqrtf(static_cast<float>(dx * dx + dy * dy));
                if (dist > blur_radius) continue;

                float alpha_mult = 1.0f - (dist / blur_radius);
                uint8_t shadow_a = static_cast<uint8_t>(a * alpha_mult);

                m_container->fill_rect(shadow_x + dx, shadow_y + dy, w, h, r, g, b, shadow_a);
            }
        }
    }
}

} // namespace ps4_browser
