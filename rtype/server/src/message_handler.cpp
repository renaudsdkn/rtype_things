#include "../include/server/message_handler.hpp"
#include <cstring>

MessageHandler::MessageHandler(PlayerManager &manager)
    : playerManager_(manager)
{
}

void MessageHandler::handleMessage(
    const std::unique_ptr<Protocol::IMessage> &message,
    asio::ip::udp::socket &socket,
    const asio::ip::udp::endpoint &remote_endpoint)
{
    using namespace ProtocolData;

    switch (message->getType())
    {
    //  Un nouveau joueur se connecte
    case MessageType::CONNECT:
    {
        std::string ip = remote_endpoint.address().to_string();
        std::cout << "[GAME] Connection attempt from " << ip << std::endl;

        if (playerManager_.hasPlayer(remote_endpoint))
        {
            std::cout << "[SERVER] Player already connected: " << ip << std::endl;
            return;
        }

        // 🔹 On crée un nouveau joueur et on récupère son ID
        uint32_t newId = playerManager_.addPlayer(remote_endpoint);

        // 🔹 Envoi d’un message WELCOME avec le vrai ID attribué
        ProtocolData::Welcome welcome{htonl(newId)};
        Protocol::WelcomeMessage response(welcome);
        auto data = response.serialize();

        socket.async_send_to(
            asio::buffer(data),
            remote_endpoint,
            [](auto ec, auto)
            {
                if (ec)
                    std::cerr << "[ERROR] Send failed: " << ec.message() << "\n";
            });

        break;
    }
        //  Le joueur envoie une action
    case MessageType::INPUT:
    {
        auto idOpt = playerManager_.getPlayerIdByEndpoint(remote_endpoint);
        if (!idOpt)
        {
            std::cerr << "[WARN] Input from unknown player: "
                      << remote_endpoint.address().to_string() << "\n";
            return;
        }

        uint32_t playerId = *idOpt;
        std::cout << "[PROTO] Player " << playerId << " sent input\n";

        // Ici, tu pourras appeler le GameServer → HandleInput(playerId, input)
        break;
    }

    //  Une entité apparaît
    case MessageType::SPAWN_ENTITY:
    {
        std::cout << "[GAME] Spawn entity request received\n";
        break;
    }

    //  Une entité bouge
    case MessageType::MOVE_ENTITY:
    {
        std::cout << "[GAME] Move entity request received\n";
        break;
    }

    //  Une entité est détruite
    case MessageType::DESTROY_ENTITY:
    {
        std::cout << "[GAME] Destroy entity request received\n";
        break;
    }

    //  Snapshot de la partie (état du jeu)
    case MessageType::SNAPSHOT:
    {
        std::cout << "[GAME] Snapshot message received\n";
        break;
    }

    //  Un événement de joueur (mort, respawn, tir, etc.)
    case MessageType::PLAYER_EVENT:
    {
        std::cout << "[EVENT] Player event received\n";
        break;
    }

    //  Vérification de connexion
    case MessageType::PING:
    {
        std::cout << "[NET] Ping received, sending response...\n";

        ProtocolData::PacketHeader header{
            htons(sizeof(ProtocolData::PacketHeader)),
            static_cast<uint8_t>(MessageType::PING_RESPONSE)};

        std::vector<uint8_t> data(sizeof(header));
        std::memcpy(data.data(), &header, sizeof(header));

        socket.async_send_to(
            asio::buffer(data),
            remote_endpoint,
            [](auto ec, auto)
            {
                if (ec)
                    std::cerr << "[ERROR] Ping response failed: " << ec.message() << "\n";
            });
        break;
    }

    //  Réponse à un ping
    case MessageType::PING_RESPONSE:
    {
        std::cout << "[NET] Ping response received\n";
        break;
    }

    case MessageType::DISCONNECT:
    {
        std::string ip = remote_endpoint.address().to_string();
        std::cout << "[GAME] Disconnect request from " << ip << std::endl;

        auto idOpt = playerManager_.getPlayerIdByEndpoint(remote_endpoint);
        if (!idOpt)
        {
            std::cerr << "[WARN] Disconnect from unknown player: " << ip << std::endl;
            return;
        }

        uint32_t playerId = *idOpt;

        // 🔹 Supprimer le joueur de la liste active
        playerManager_.removePlayer(remote_endpoint);
        std::cout << "[SERVER] Player " << playerId << " disconnected." << std::endl;

        // 🔹 Notifier les autres joueurs (broadcast)
        ProtocolData::DestroyEntity destroyMsg{htonl(playerId)};
        Protocol::DestroyEntityMessage notif(destroyMsg);
        auto data = notif.serialize();

        for (auto &ep : playerManager_.getAllEndpoints())
        {
            if (ep != remote_endpoint)
            {
                socket.async_send_to(asio::buffer(data), ep,
                                     [](auto ec, auto)
                                     {
                                         if (ec)
                                             std::cerr << "[WARN] Failed to send DISCONNECT notification: "
                                                       << ec.message() << "\n";
                                     });
            }
        }

        break;
    }

    //  Erreur du protocole
    case MessageType::ERROR:
    {
        std::cerr << "[ERROR] Protocol error message received\n";
        break;
    }

    default:
        std::cout << "[WARN] Unknown message type: "
                  << static_cast<int>(message->getType()) << std::endl;
        break;
    }
}
