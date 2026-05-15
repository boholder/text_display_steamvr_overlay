#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_TCPSERVER_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_TCPSERVER_H

#include <sockpp/tcp_acceptor.h>

#include "log.h"
#include "Settings.h"
#include "Subtitle.h"

/**
 * For checking if the TCP server port setting option has changed
 */
static int current_port = 0;

namespace tcp_server
{

static bool start_server(sockpp::tcp_acceptor& server)
{
    current_port = settings.tcp_server_port;
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
        if (settings.tcp_server_port_changed())
        {
            server.shutdown();
            if (!start_server(server))
                return;
            current_port = settings.tcp_server_port;
        }

        // accept a new client connection
        sockpp::inet_address peer;
        if (auto conn_result = server.accept(TCP_SERVER_TIMEOUT, &peer); !conn_result)
        {
            if (conn_result != std::errc::timed_out)
            {
                SPDLOG_ERROR("Error accepting connection: {}", conn_result.error_message());
                return;
            }
        }
        else
        {
            auto peer_addr = peer.to_string();
            SPDLOG_INFO("Accept connection with [{}]", peer_addr);

            sockpp::tcp_socket socket = conn_result.release();
            socket.read_timeout(TCP_SOCKET_TIMEOUT);
            sockpp::result<size_t> r;

            char buf[TCP_SERVER_BUFFER_SIZE];
            auto* const begin = reinterpret_cast<char*>(&buf);
            std::fill_n(begin, TCP_SERVER_BUFFER_SIZE, 0);
            unsigned long long last_len = 0;

            // keep connection alive, but check port changing every TCP_SOCKET_TIMEOUT
            while (true)
            {
                if (settings.tcp_server_port_changed())
                {
                    SPDLOG_INFO("Option [TCP Server Port] changed, close connection with [{}]", peer_addr);
                    socket.close();
                    break; // to listening new connection
                }

                // only zeroing non-zero (used for storing last data) parts in every loop
                std::fill_n(begin, last_len, 0);

                r = socket.read(buf, sizeof(buf)); // blocking I/O until socket timeout

                if (r.value() > 0)
                {
                    SPDLOG_DEBUG("[{}] sends: [{}]", peer_addr, buf);
                    last_len = r.value();
                    Subtitle::append(buf);
                }
                else if (r.error().value() == 0 && r.value() == 0)
                {
                    // a successful read that returns a value of zero indicates that
                    // the peer actively closed the connection
                    // ref: https://github.com/fpagliughi/sockpp/issues/99#issuecomment-4263496155
                    SPDLOG_INFO("Connection with [{}] closed by peer", peer_addr);
                    break; // to listening new connection
                }
                else
                {
                    SPDLOG_ERROR("Error reading from connection with [{}], close it: {}", peer_addr, r.error().message());
                    socket.close();
                    break; // to listening new connection
                }
            }
        }
    }
}

} // namespace tcp_server

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_TCPSERVER_H
