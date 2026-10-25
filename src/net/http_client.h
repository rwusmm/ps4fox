#pragma once

#include <stddef.h>
#include <stdint.h>

namespace ps4b {

// Initialize PS4 network, TLS/SSL, and HTTP subsystems
bool net_init();

// Terminate network and SSL contexts
void net_shutdown();

// Fetch web resource over HTTP/HTTPS with TLS
bool http_fetch(const char* url, uint8_t** out_data, size_t* out_size);

// Status queries
bool is_net_ready();
bool is_ssl_ready();

// Statistics for debug interface
void net_add_bytes_transferred(size_t bytes);
size_t net_get_bytes_transferred();
void net_reset_bytes_transferred();

// Get current HTTP technology being used (HTTP, HTTPS, or HTTP/2)
const char* net_get_current_tech();

} // namespace ps4b
