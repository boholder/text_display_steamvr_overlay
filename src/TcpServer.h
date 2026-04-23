#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_TCPSERVER_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_TCPSERVER_H

#include "sockpp/tcp_acceptor.h"
#include <spdlog/spdlog.h>
#include <chrono>

#include "Settings.h"

namespace tcp_server
{

static bool start_server(sockpp::tcp_acceptor& server)
{
    in_port_t port = settings.tcp_server_port;
    std::error_code ec;
    server = {port, 4, sockpp::tcp_acceptor::REUSE, ec};
    if (ec)
    {
        SPDLOG_ERROR("Error creating the acceptor: {}", ec.message());
        return false;
    }
    SPDLOG_INFO("TCP server listens on port [{}]", port);
    return true;
}

static void tcp_server_thread()
{
    sockpp::initialize();

    sockpp::tcp_acceptor server;
    if (!start_server(server))
        return;

    while (true)
    {
        if (settings.is_tcp_server_port_changed())
        {
            server.shutdown();
            if (!start_server(server))
                return;
            settings.apply_tcp_server_port();
        }

        // accept a new client connection
        sockpp::inet_address peer;
        if (auto res = server.accept(TCP_SERVER_TIMEOUT, &peer); !res)
        {
            if (res != std::errc::timed_out)
            {
                SPDLOG_ERROR("Error accepting connection: {}", res.error_message());
                return;
            }
        }
        else
        {
            auto peer_addr = peer.to_string();
            SPDLOG_INFO("Accept connection with [{}]", peer_addr);

            sockpp::tcp_socket sock = res.release();
            sock.read_timeout(TCP_SOCKET_TIMEOUT);
            sockpp::result<size_t> r;

            char buf[TCP_SERVER_BUFFER_SIZE];
            auto* const begin = reinterpret_cast<char*>(&buf);
            std::fill_n(begin, TCP_SERVER_BUFFER_SIZE, 0);
            // only zeroing non-zero (used for storing last data) parts in every loop
            unsigned long long last_len = 0;

            // keep connection alive, but check port changing every TCP_SOCKET_TIMEOUT
            while (true)
            {
                if (settings.is_tcp_server_port_changed())
                {
                    SPDLOG_INFO("Option [TCP Server Port] changed, close connection with [{}]", peer_addr);
                    sock.close();
                    break; // to
                }

                std::fill_n(begin, last_len, 0);

                r = sock.read(buf, sizeof(buf)); // blocking

                if (r.value() > 0)
                {
                    SPDLOG_DEBUG("[{}] sends: [{}]", peer_addr, buf);
                    last_len = r.value();
                }
                else if (r.error().value() == 0 && r.value() == 0)
                {
                    // a successful read that returns a value of zero indicates that the connection is closed
                    // ref: https://github.com/fpagliughi/sockpp/issues/99#issuecomment-4263496155
                    SPDLOG_INFO("Connection with [{}] closed by peer", peer_addr);
                    break;
                }
            }
        }
    }
}

} // namespace tcp_server

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_TCPSERVER_H
