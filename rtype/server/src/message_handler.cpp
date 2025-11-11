#include "../include/server/message_handler.hpp"
#include "../include/server/GameManager.hpp"
#include "../include/server/server.hpp"
#include <cstring>

MessageHandler::MessageHandler(PlayerManager &manager, GameManager &gameManager)
    : m_playerManager(manager),
      m_gameManagerRef(gameManager)
{
}

void MessageHandler::handleMessage(
    const std::unique_ptr<Protocol::IMessage> &message,
    const asio::ip::udp::endpoint &remote_endpoint)
{
    using namespace ProtocolData;

    // On récupère l'ID du joueur, sauf si c'est un message CONNECT
    std::optional<uint32_t> playerIdOpt;
    if (message->getType() != MessageType::CONNECT)
    {
        playerIdOpt = m_playerManager.getPlayerIdByEndpoint(remote_endpoint);
        if (!playerIdOpt)
        {
            std::cerr << "[MSG HANDLER] Message " << static_cast<int>(message->getType())
                      << " reçu d'un joueur inconnu (non-connecté): "
                      << remote_endpoint.address().to_string() << std::endl;
            return;
        }
    }

    switch (message->getType())
    {
    case MessageType::CONNECT:
    {
        std::string ip = remote_endpoint.address().to_string();

        // Vérifie si déjà connecté
        if (m_playerManager.hasPlayer(remote_endpoint))
        {
            std::cout << "[MSG HANDLER] Joueur déjà connecté : " << ip << std::endl;
            return;
        }

        // ✅ 1. Récupère le pseudo demandé
        auto *connectMsg = static_cast<Protocol::ConnectMessage *>(message.get());
        std::string requestedNickname = connectMsg->getData().nickname;
        std::cout << "[SERVER] CONNECT reçu de " << remote_endpoint
                  << " avec pseudo: '" << requestedNickname << "'" << std::endl;

        // ✅ 2. Valide et nettoie
        std::string sanitized = m_playerManager.validateAndSanitizeNickname(requestedNickname);

        // ✅ 3. Vérifie longueur minimum
        if (sanitized.length() < 3)
        {
            std::cout << "[SERVER] Pseudo refusé: trop court (" << sanitized.length() << " caractères)" << std::endl;

            // Envoie un WELCOME de refus
            Protocol::WelcomeMessage rejectMsg("Pseudo trop court (min 3 caractères)");
            auto data = rejectMsg.serialize();
            m_gameManagerRef.sendToEndpoint(remote_endpoint, data);
            return;
        }

        // ✅ 4. Vérifie disponibilité
        if (!m_playerManager.isNicknameAvailable(sanitized))
        {
            std::cout << "[SERVER] Pseudo refusé: déjà utilisé" << std::endl;

            Protocol::WelcomeMessage rejectMsg("Pseudo déjà utilisé, choisissez-en un autre");
            auto data = rejectMsg.serialize();
            m_gameManagerRef.sendToEndpoint(remote_endpoint, data);
            return;
        }

        // ✅ 5. Accepter : Assigner ID et stocker pseudo
        uint32_t playerId = m_playerManager.addPlayer(remote_endpoint);
        m_playerManager.setNickname(playerId, sanitized);

        std::cout << "[SERVER] Assigned ID " << playerId << " to player " << remote_endpoint
                  << " ('" << sanitized << "')" << std::endl;

        // ✅ 6. Envoie WELCOME de confirmation
        Protocol::WelcomeMessage acceptMsg(playerId, sanitized);
        auto data = acceptMsg.serialize();
        m_gameManagerRef.sendToEndpoint(remote_endpoint, data);

        // ✅ 7. Ajoute au lobby
        m_gameManagerRef.handleNewPlayer(playerId, remote_endpoint);
        break;
    }

    case MessageType::INPUT:
    {
        if (playerIdOpt)
        {
            auto *inputMsg = static_cast<Protocol::PlayerInputMessage *>(message.get());
            ProtocolData::PlayerInput input = inputMsg->getData();
            input.playerId = *playerIdOpt;
            m_gameManagerRef.handlePlayerInput(*playerIdOpt, input);
        }
        break;
    }

    case MessageType::DISCONNECT:
    {
        if (playerIdOpt)
        {
            uint32_t playerId = *playerIdOpt;
            m_gameManagerRef.handlePlayerDisconnect(playerId);
            m_playerManager.removePlayer(remote_endpoint);
            std::cout << "[SERVER] Joueur " << playerId << " déconnecté (demande)." << std::endl;
        }
        break;
    }

    case MessageType::LIST_ROOMS_REQUEST:
    {
        if (playerIdOpt)
        {
            m_gameManagerRef.handleListRoomsRequest(*playerIdOpt);
        }
        break;
    }

    case MessageType::CREATE_ROOM_REQUEST:
{
    if (playerIdOpt)
    {
        auto *createMsg = static_cast<Protocol::CreateRoomRequestMessage *>(message.get());
        const ProtocolData::RoomConfig& config = createMsg->getData().config;
        
        std::cout << "[MSG HANDLER DEBUG] Config reçue:" << std::endl;
        std::cout << "  - Nom: '" << config.roomName << "'" << std::endl;
        std::cout << "  - Difficulté: " << (int)config.difficulty << std::endl;
        std::cout << "  - Max joueurs: " << (int)config.maxPlayers << std::endl;
        
        m_gameManagerRef.handleCreateRoomRequest(*playerIdOpt, config);
    }
    break;
}

    case MessageType::JOIN_ROOM_REQUEST:
    {
        if (playerIdOpt)
        {
            try
            {
                auto *joinMsg = static_cast<Protocol::JoinRoomRequestMessage *>(message.get());
                uint32_t roomIdToJoin = joinMsg->getData().roomId;
                m_gameManagerRef.handleJoinRoomRequest(*playerIdOpt, roomIdToJoin);
            }
            catch (const std::exception &e)
            {
                std::cerr << "[MSG HANDLER ERREUR] Échec lecture JOIN_ROOM_REQUEST: " << e.what() << std::endl;
            }
        }
        break;
    }

    case MessageType::LEAVE_ROOM_REQUEST:
    {
        if (playerIdOpt)
        {
            m_gameManagerRef.handlePlayerLeft(*playerIdOpt);
        }
        break;
    }

    default:
        std::cout << "[THREAD JEU] Message inconnu: " << static_cast<int>(message->getType()) << std::endl;
        break;
    }
}