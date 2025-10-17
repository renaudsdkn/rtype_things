#pragma once
#include "../include/protocol/serializer.hpp"
#include "../include/protocol/Factory.hpp"
#include "message_handler.hpp"
#include "PlayerManager.hpp"
#include <asio.hpp>
#include <array>
#include <iostream>
#include <chrono>

class UdpServer
{
public:
    explicit UdpServer(asio::io_context &io_context,unsigned short port);

private:
    void start_receive();
    void process_datagram(std::size_t length);
    void start_cleanup_timer();
    void cleanup_inactive_clients();

    asio::ip::udp::socket socket_;
    std::array<char, 1024> recv_buffer_;
    asio::ip::udp::endpoint remote_endpoint_;
    asio::steady_timer cleanup_timer_;
    PlayerManager playerManager_;
    MessageHandler handler_;
};
