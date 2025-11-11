#pragma once
#include <asio.hpp>
#include <array>
#include <iostream>
#include <memory>
#include "ThreadSafeQueue.hpp"

class UdpServer : public std::enable_shared_from_this<UdpServer>
{
public:
    UdpServer(asio::io_context &io_context, unsigned short port,
              std::shared_ptr<ThreadSafeQueue<NetworkPacket>> incomingMessages);
    void startReceive();
    void send(const std::vector<uint8_t>& data, const asio::ip::udp::endpoint& endpoint);
private:
    void startReceiveLoop();
    asio::ip::udp::socket socket_;
    std::shared_ptr<ThreadSafeQueue<NetworkPacket>> m_incomingMessages;
};