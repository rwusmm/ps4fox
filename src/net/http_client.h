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

} // namespace ps4b
