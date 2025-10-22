#include "../include/server/message_handler.hpp"
#include <cstring>

#include "../include/server/message_handler.hpp"
#include "../include/server/GameManager.hpp"
#include "../include/server/server.hpp"
#include <cstring>

MessageHandler::MessageHandler(
    PlayerManager &manager,
    GameManager &gameManager,
    UdpServer &server)
    : m_playerManager(manager),
      m_gameManager(gameManager),
      m_server(server)
{
}

void MessageHandler::handleMessage(
    const std::unique_ptr<Protocol::IMessage> &message,
    const asio::ip::udp::endpoint &remote_endpoint)
{
    using namespace ProtocolData;

    switch (message->getType())
    {
    case MessageType::CONNECT:
    {
        std::string ip = remote_endpoint.address().to_string();
        std::cout << "[THREAD JEU] Tentative de connexion de " << ip << std::endl;

        if (m_playerManager.hasPlayer(remote_endpoint))
        {
            std::cout << "[THREAD JEU] Joueur déjà connecté : " << ip << std::endl;
            // TODO: Renvoyer un WELCOME ?
            return;
        }

        uint32_t newId = m_playerManager.addPlayer(remote_endpoint);
        m_gameManager.handleNewPlayer(newId, remote_endpoint);

        ProtocolData::Welcome welcome{htonl(newId)};
        Protocol::WelcomeMessage response(welcome);
        auto data = response.serialize();
        m_server.send(data, remote_endpoint);
        break;
    }
    case MessageType::INPUT:
    {
        // 1. Récupérer l'ID du joueur
        auto idOpt = m_playerManager.getPlayerIdByEndpoint(remote_endpoint);
        if (!idOpt)
        {
            // Log d'avertissement si l'input vient d'un joueur inconnu
            std::cerr << "[THREAD JEU][WARN] Input reçu d'un endpoint inconnu: "
                      << remote_endpoint.address().to_string() << ":" << remote_endpoint.port() << "\n";
            return;
        }
        uint32_t playerId = *idOpt;

        // 2. Récupérer les données de l'input
        //    (Assure-toi que PlayerInputMessage a une méthode getData())
        auto *inputMsg = static_cast<Protocol::PlayerInputMessage *>(message.get());
        const ProtocolData::PlayerInput &inputData = inputMsg->getData();

        // 3. ✅ Afficher le Log Détaillé
        std::cout << "[THREAD JEU][INPUT] Reçu de Joueur ID: " << playerId
                  << " [ U:" << (inputData.up ? 'X' : '-') // Affiche X si true, - si false
                  << " D:" << (inputData.down ? 'X' : '-')
                  << " L:" << (inputData.left ? 'X' : '-')
                  << " R:" << (inputData.right ? 'X' : '-')
                  << " S:" << (inputData.shoot ? 'X' : '-')
                  << " ]" << std::endl;

        // 4. Transmettre l'input au GameManager (comme avant)
        //    Note: L'inputData contient déjà les booléens, pas besoin de recréer une structure
        m_gameManager.handlePlayerInput(playerId, inputData);
        break;
    }
    case MessageType::DISCONNECT:
    {
        auto idOpt = m_playerManager.getPlayerIdByEndpoint(remote_endpoint);
        if (!idOpt)
            return;

        uint32_t playerId = *idOpt;
        m_gameManager.handlePlayerDisconnect(playerId);
        m_playerManager.removePlayer(remote_endpoint);
        std::cout << "[THREAD JEU] Joueur " << playerId << " déconnecté." << std::endl;

        // TODO: Notifier les autres joueurs
        break;
    }

    default:
        std::cout << "[THREAD JEU] Message inconnu: " << static_cast<int>(message->getType()) << std::endl;
        break;
    }
}