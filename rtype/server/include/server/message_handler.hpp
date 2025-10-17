#pragma once
#include "../include/protocol/protocol_data.hpp"
#include "../include/protocol/serializer.hpp"
#include "PlayerManager.hpp"
#include <asio.hpp>
#include <iostream>

class MessageHandler
{
private:
    PlayerManager &playerManager_;

public:
    explicit MessageHandler(PlayerManager &manager);

    void handleMessage(
        const std::unique_ptr<Protocol::IMessage> &message,
        asio::ip::udp::socket &socket,
        const asio::ip::udp::endpoint &remote_endpoint);
};
