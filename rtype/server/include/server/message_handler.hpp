#pragma once
#include "../include/protocol/serializer.hpp"
#include "PlayerManager.hpp"
#include <asio.hpp>
#include <iostream>

class GameManager;
class UdpServer;

class MessageHandler
{
private:
    PlayerManager &m_playerManager;
    GameManager &m_gameManager;
    UdpServer &m_server;
public:
    explicit MessageHandler(
        PlayerManager &manager,
        GameManager &gameManager,
        UdpServer &server);

    void handleMessage(
        const std::unique_ptr<Protocol::IMessage> &message,
        const asio::ip::udp::endpoint &remote_endpoint);
};