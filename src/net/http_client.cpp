#include "http_client.h"
#include "ps4b/presenter.h"
#include <stdlib.h>
#include <string.h>
#include <vector>

#if __has_include(<orbis/libkernel.h>)
#include <orbis/libkernel.h>
#endif
#if __has_include(<orbis/Sysmodule.h>)
#include <orbis/Sysmodule.h>
#endif
#if __has_include(<orbis/Http.h>)
#include <orbis/Http.h>
#endif
#if __has_include(<orbis/Ssl.h>)
#include <orbis/Ssl.h>
#endif
#if __has_include(<orbis/Net.h>)
#include <orbis/Net.h>
#endif

namespace ps4b {

static bool g_http_initialized = false;
static bool g_ssl_initialized = false;
static int32_t g_net_pool_id = -1;
static int32_t g_http_ctx_id = -1;
static int32_t g_ssl_ctx_id = -1;

// Statistics tracking
static size_t g_bytes_transferred = 0;
static char g_current_tech[16] = "HTTP";

bool is_net_ready() {
    return g_http_initialized;
}

bool is_ssl_ready() {
    return g_ssl_initialized;
}

bool net_init() {
    if (g_http_initialized) return true;

#ifdef _PS4
    // Load required system modules for Networking, SSL, and HTTP
    sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NET);
    sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SSL);
    sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_HTTP);

    // Initialize Net subsystem and pool
    int32_t netRc = sceNetInit();
    orbisfox::log("PS4b Net: sceNetInit rc=0x%08x", (unsigned)netRc);

    g_net_pool_id = sceNetPoolCreate("ps4b_net_pool", 64 * 1024, 0);
    orbisfox::log("PS4b Net: sceNetPoolCreate id=%d", g_net_pool_id);

    // Initialize SSL (64KB memory pool)
    g_ssl_ctx_id = sceSslInit(64 * 1024);
    if (g_ssl_ctx_id >= 0) {
        g_ssl_initialized = true;
        orbisfox::log("PS4b Net: TLS/SSL initialized (ctx=%d)", g_ssl_ctx_id);
    } else {
        orbisfox::log("PS4b Net: sceSslInit rc=0x%08x", (unsigned)g_ssl_ctx_id);
    }

    // Initialize HTTP client context with memory pool and ssl context
    int32_t memPoolId = g_net_pool_id >= 0 ? g_net_pool_id : 0;
    int32_t sslId = g_ssl_ctx_id >= 0 ? g_ssl_ctx_id : 0;
    g_http_ctx_id = sceHttpInit(memPoolId, sslId, 128 * 1024);
    if (g_http_ctx_id >= 0) {
        g_http_initialized = true;
        orbisfox::log("PS4b Net: HTTP initialized (ctx=%d)", g_http_ctx_id);
    } else {
        orbisfox::log("PS4b Net: sceHttpInit rc=0x%08x", (unsigned)g_http_ctx_id);
    }
#else
    g_http_initialized = true;
    g_ssl_initialized = true;
#endif

    return g_http_initialized;
}

void net_shutdown() {
#ifdef _PS4
    if (g_http_ctx_id >= 0) {
        sceHttpTerm(g_http_ctx_id);
        g_http_ctx_id = -1;
        g_http_initialized = false;
    }
    if (g_ssl_ctx_id >= 0) {
        sceSslTerm();
        g_ssl_ctx_id = -1;
        g_ssl_initialized = false;
    }
    if (g_net_pool_id >= 0) {
        sceNetPoolDestroy(g_net_pool_id);
        g_net_pool_id = -1;
    }
    sceNetTerm();
#else
    g_http_initialized = false;
    g_ssl_initialized = false;
#endif
}

bool http_fetch(const char* url, uint8_t** out_data, size_t* out_size) {
    if (!url || !out_data || !out_size) return false;
    *out_data = nullptr;
    *out_size = 0;

    orbisfox::log("PS4b Net: Fetching URL: %s", url);

    // Determine current technology
    if (strncmp(url, "https://", 8) == 0) {
        strcpy(g_current_tech, "HTTPS");
    } else if (strncmp(url, "http://", 7) == 0) {
        strcpy(g_current_tech, "HTTP");
    } else if (strncmp(url, "b://", 4) == 0) {
        strcpy(g_current_tech, "LOCAL");
    }

    // Local home and blank pages
    if (strcmp(url, "about:blank") == 0 || strcmp(url, "http://about:blank") == 0) {
        const char* blank_html =
            "<!DOCTYPE html><html><head><style>"
            "body { background: #131b2f; color: #e1e9f8; font-family: sans-serif; padding: 24px; }"
            "h1 { color: #5bc0be; font-size: 32px; border-bottom: 2px solid #3a506b; padding-bottom: 8px; }"
            "p { font-size: 18px; line-height: 1.6; color: #cbd5e1; }"
            ".card { background: #1c2541; padding: 16px; border-radius: 8px; border: 1px solid #3a506b; margin-top: 16px; }"
            "code { background: #0b132b; color: #6fffe9; padding: 2px 6px; border-radius: 4px; }"
            "</style></head><body>"
            "<h1>PS4b Web Browser</h1>"
            "<div class='card'>"
            "<p>Browser engine online. Enter a URL to browse the web.</p>"
            "<p>Engine components: <code>LiteHTML Layout</code>, <code>CSS3 Styles</code>, <code>TLS 1.2/1.3</code>, and <code>HTTP/1.1 Client</code>.</p>"
            "</div></body></html>";
        size_t len = strlen(blank_html);
        *out_data = static_cast<uint8_t*>(malloc(len + 1));
        if (*out_data) {
            memcpy(*out_data, blank_html, len + 1);
            *out_size = len;
            return true;
        }
        return false;
    }

    // Settings page
    if (strcmp(url, "b://settings") == 0) {
        const char* settings_html =
            "<!DOCTYPE html><html><head><style>"
            "body { background: #131b2f; color: #e1e9f8; font-family: sans-serif; padding: 24px; }"
            "h1 { color: #5bc0be; font-size: 32px; border-bottom: 2px solid #3a506b; padding-bottom: 8px; }"
            ".card { background: #1c2541; padding: 16px; border-radius: 8px; border: 1px solid #3a506b; margin-top: 16px; }"
            ".setting-row { display: flex; justify-content: space-between; align-items: center; padding: 12px 0; border-bottom: 1px solid #3a506b; }"
            ".setting-row:last-child { border-bottom: none; }"
            ".toggle { position: relative; width: 50px; height: 26px; background: #3a506b; border-radius: 13px; cursor: pointer; transition: background 0.3s; }"
            ".toggle.active { background: #5bc0be; }"
            ".toggle::after { content: ''; position: absolute; top: 3px; left: 3px; width: 20px; height: 20px; background: #e1e9f8; border-radius: 50%; transition: transform 0.3s; }"
            ".toggle.active::after { transform: translateX(24px); }"
            "label { font-size: 16px; color: #cbd5e1; }"
            "</style></head><body>"
            "<h1>PS4b Settings</h1>"
            "<div class='card'>"
            "<div class='setting-row'>"
            "<label>Dev Debug Interface</label>"
            "<div class='toggle' id='debugToggle' onclick='toggleDebug()'></div>"
            "</div>"
            "</div>"
            "<script>"
            "function toggleDebug() {"
            "  var toggle = document.getElementById('debugToggle');"
            "  toggle.classList.toggle('active');"
            "  js_toggle_debug(toggle.classList.contains('active'));"
            "}"
            "</script></body></html>";
        size_t len = strlen(settings_html);
        *out_data = static_cast<uint8_t*>(malloc(len + 1));
        if (*out_data) {
            memcpy(*out_data, settings_html, len + 1);
            *out_size = len;
            return true;
        }
        return false;
    }

#ifdef _PS4
    if (!g_http_initialized && !net_init()) {
        orbisfox::log("PS4b Net: HTTP initialization failed");
        return false;
    }

    int32_t tmplId = sceHttpCreateTemplate(g_http_ctx_id, "Mozilla/5.0 (PlayStation 4) PS4b/1.0", 1, 0);
    if (tmplId < 0) {
        orbisfox::log("PS4b Net: sceHttpCreateTemplate failed: 0x%08x", (unsigned)tmplId);
        return false;
    }

    // HTTP/2 support can be enabled with sceHttpSetEnableHttp2 if available in SDK

    int32_t connId = sceHttpCreateConnectionWithURL(tmplId, url, true);
    if (connId < 0) {
        orbisfox::log("PS4b Net: sceHttpCreateConnectionWithURL failed: 0x%08x", (unsigned)connId);
        sceHttpDeleteTemplate(tmplId);
        return false;
    }

    int32_t reqId = sceHttpCreateRequestWithURL2(connId, "GET", url, 0);
    if (reqId < 0) {
        orbisfox::log("PS4b Net: sceHttpCreateRequestWithURL2 failed: 0x%08x", (unsigned)reqId);
        sceHttpDeleteConnection(connId);
        sceHttpDeleteTemplate(tmplId);
        return false;
    }

    int32_t sendRc = sceHttpSendRequest(reqId, nullptr, 0);
    if (sendRc < 0) {
        orbisfox::log("PS4b Net: sceHttpSendRequest failed: 0x%08x", (unsigned)sendRc);
        sceHttpDeleteRequest(reqId);
        sceHttpDeleteConnection(connId);
        sceHttpDeleteTemplate(tmplId);
        return false;
    }

    int32_t statusCode = 0;
    sceHttpGetStatusCode(reqId, &statusCode);
    orbisfox::log("PS4b Net: HTTP status code: %d", statusCode);

    std::vector<uint8_t> buffer;
    uint8_t chunk[4096];
    int32_t readBytes = 0;

    while ((readBytes = sceHttpReadData(reqId, chunk, sizeof(chunk))) > 0) {
        buffer.insert(buffer.end(), chunk, chunk + readBytes);
        net_add_bytes_transferred(readBytes);
        if (buffer.size() > 8 * 1024 * 1024) break; // 8MB safety cap
    }

    sceHttpDeleteRequest(reqId);
    sceHttpDeleteConnection(connId);
    sceHttpDeleteTemplate(tmplId);

    if (!buffer.empty()) {
        *out_data = static_cast<uint8_t*>(malloc(buffer.size() + 1));
        if (*out_data) {
            memcpy(*out_data, buffer.data(), buffer.size());
            (*out_data)[buffer.size()] = '\0';
            *out_size = buffer.size();
            return true;
        }
    }
#endif

    return false;
}

// Statistics functions
void net_add_bytes_transferred(size_t bytes) {
    ps4b::g_bytes_transferred += bytes;
}

size_t net_get_bytes_transferred() {
    return ps4b::g_bytes_transferred;
}

void net_reset_bytes_transferred() {
    ps4b::g_bytes_transferred = 0;
}

const char* net_get_current_tech() {
    return ps4b::g_current_tech;
}

} // namespace ps4b
