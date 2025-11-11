#pragma once
#include "../include/protocol/protocol_data.hpp"
#include "../include/protocol/serializer.hpp"
#include "PlayerManager.hpp"
#include <asio.hpp>
#include <iostream>
#include <memory> // Pour std::unique_ptr

// Déclaration anticipée
class GameManager;

class MessageHandler
{
private:
    PlayerManager &m_playerManager;
    GameManager &m_gameManagerRef; // ✅ Renommé pour plus de clarté

public:
    // ✅ Constructeur modifié
    explicit MessageHandler(PlayerManager &manager, GameManager &gameManager);

    // ✅ Signature modifiée (plus besoin du socket)
    void handleMessage(
        const std::unique_ptr<Protocol::IMessage> &message,
        const asio::ip::udp::endpoint &remote_endpoint);
};