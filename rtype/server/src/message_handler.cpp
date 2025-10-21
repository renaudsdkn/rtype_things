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
{}

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

        if (m_playerManager.hasPlayer(remote_endpoint)) {
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
        auto idOpt = m_playerManager.getPlayerIdByEndpoint(remote_endpoint);
        if (!idOpt) return;
        
        auto* inputMsg = static_cast<Protocol::PlayerInputMessage*>(message.get());
        ProtocolData::PlayerInput input = inputMsg->getData();
        input.playerId = *idOpt; // S'assurer que l'ID est le bon
        
        m_gameManager.handlePlayerInput(*idOpt, input);
        break;
    }
    
    case MessageType::DISCONNECT:
    {
        auto idOpt = m_playerManager.getPlayerIdByEndpoint(remote_endpoint);
        if (!idOpt) return;
        
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