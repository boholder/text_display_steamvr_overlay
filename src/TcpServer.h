#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_TCPSERVER_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_TCPSERVER_H

#include "sockpp/tcp_acceptor.h"
#include <spdlog/spdlog.h>
#include <chrono>

namespace tcp_server
{

static int tcp_server_thread()
{
    SPDLOG_DEBUG("TCP server thread id: {}", std::this_thread::get_id()._Get_underlying_id());

    sockpp::initialize();

    in_port_t port = settings.tcp_server_port;
    std::error_code ec;
    sockpp::tcp_acceptor acc{port, 4, sockpp::tcp_acceptor::REUSE, ec};
    if (ec)
    {
        SPDLOG_ERROR("Error creating the acceptor: {}", ec.message());
        return 1;
    }
    SPDLOG_INFO("TCP server started on port {}", port);

    auto timeout = TCP_SERVER_TIMEOUT;
    while (true)
    {
        sockpp::inet_address peer;

        // Accept a new client connection
        if (auto res = acc.accept(timeout, &peer); !res)
        {
            if (res == std::errc::timed_out)
            {
                timeout = TCP_SERVER_TIMEOUT;
            }
            else
            {
                SPDLOG_ERROR("Error accepting connection: {}", res.error_message());
            }
        }
        else
        {
            auto peer_addr = peer.to_string();
            SPDLOG_INFO("Connection with [{}] accepted", peer_addr);

            sockpp::tcp_socket sock = res.release();
            sockpp::result<size_t> r;
            char buf[512];

            while (sock.is_open())
            {
                auto* const begin = reinterpret_cast<char*>(&buf);
                char* end = begin + sizeof(buf);
                std::fill(begin, end, 0);

                r = sock.read(buf, sizeof(buf)); // blocking

                if (r.value() > 0)
                {
                    SPDLOG_DEBUG("[{}] sends: [{}]", peer_addr, buf);
                }
                else if (r.error().value() == 0 && r.value() == 0)
                {
                    // a successful read that returns a value of zero indicates that the connection is closed
                    // ref: https://github.com/fpagliughi/sockpp/issues/99#issuecomment-4263496155
                    SPDLOG_INFO("Connection with [{}] closed", peer_addr);
                    break;
                }
            }
        }
    }
}

} // namespace tcp_server

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_TCPSERVER_H
