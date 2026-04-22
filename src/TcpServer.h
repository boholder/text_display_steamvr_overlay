#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_TCPSERVER_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_TCPSERVER_H

#include <spdlog/spdlog.h>

namespace tcp_server
{

static void tcp_server_thread(int port)
{ spdlog::info("TCP server started on port {}", port); }

} // namespace tcp_server

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_TCPSERVER_H
