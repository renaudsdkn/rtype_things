
// Dans rtype/client/src/client.cpp
#include "../include/client/client.hpp"
// ❌ PAS d'include GameClient.hpp ici
#include "protocol/serializer.hpp" // Pour les classes Message
#include "protocol/Factory.hpp"    // ✅ Pour MessageFactory
#include <vector>
#include <memory> // Pour std::unique_ptr
#include <array>  // Pour std::array

using namespace Protocol;
using namespace ProtocolData;

// ✅ Constructeur SIMPLE
RTypeClient::RTypeClient(const std::string &server_ip, unsigned short server_port)
    : m_io(), // Initialise m_io
      m_socket(m_io),
      m_server_endpoint(asio::ip::make_address(server_ip), server_port)
// ❌ PAS d'initialisation m_gameClient
{
    m_socket.open(asio::ip::udp::v4());
}

// Destructeur, start, stop, send_* (inchangés)
RTypeClient::~RTypeClient() { stop(); }

void RTypeClient::start()
{
    m_running = true;

    // ✅ Socket NON-BLOQUANT
    m_socket.non_blocking(true);

    m_receiver = std::thread(&RTypeClient::receive_loop, this);
    std::cout << "[CLIENT] Client réseau démarré (mode non-bloquant)" << std::endl;
}

void RTypeClient::sendRaw(const std::vector<uint8_t> &data)
{
    m_socket.send_to(asio::buffer(data), m_server_endpoint);
}

void RTypeClient::send_connect(const std::string &nickname)
{
    Protocol::ConnectMessage msg(nickname); // ✅ Passe le pseudo
    auto data = msg.serialize();
    m_socket.send_to(asio::buffer(data), m_server_endpoint);
    std::cout << "[CLIENT] CONNECT envoyé avec pseudo: '" << nickname << "'" << std::endl;
}

void RTypeClient::send_input(const PlayerInput &input)
{
    PlayerInputMessage msg(input);
    auto data = msg.serialize();
    m_socket.send_to(asio::buffer(data), m_server_endpoint);
    std::cout << "[CLIENT] INPUT envoyé\n";
}

void RTypeClient::send_disconnect()
{
    ProtocolData::PacketHeader header{
        htons(sizeof(ProtocolData::PacketHeader)),                  // ✅ taille du header en big endian
        static_cast<uint8_t>(ProtocolData::MessageType::DISCONNECT) // ✅ bon enum
    };

    std::vector<uint8_t> buffer(sizeof(header));
    std::memcpy(buffer.data(), &header, sizeof(header));

    try
    {
        m_socket.send_to(asio::buffer(buffer), m_server_endpoint);
        std::cout << "[CLIENT] DISCONNECT envoyé à "
                  << m_server_endpoint.address().to_string() << ":" << m_server_endpoint.port() << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[CLIENT ERROR] Envoi DISCONNECT échoué : " << e.what() << std::endl;
    }
}

// ✅ Implémentation des setters pour les handlers

// ... (autres fonctions) ...

void RTypeClient::stop()
{
    std::cout << "[CLIENT] Arrêt demandé..." << std::endl;
    m_running = false; // Signale au thread de s'arrêter logiquement

    // ✅ Ferme le socket depuis ce thread (le thread principal)
    // Cela va provoquer le déblocage immédiat de receive_from() dans l'autre thread,
    // qui retournera une erreur spécifique.
    if (m_socket.is_open())
    {
        asio::error_code ec;
        // Optionnel: Tenter une fermeture plus propre
        m_socket.shutdown(asio::ip::udp::socket::shutdown_both, ec);
        // Fermer le socket
        m_socket.close(ec);
        if (ec)
        {
            std::cerr << "[CLIENT] Erreur lors de la fermeture du socket: " << ec.message() << std::endl;
        }
        else
        {
            std::cout << "[CLIENT] Socket fermé." << std::endl;
        }
    }

    // Attend maintenant que le thread receive_loop se termine effectivement.
    // Comme le socket est fermé, receive_from va retourner, la boucle while(m_running)
    // sera fausse (ou on sortira à cause de l'erreur), et le thread finira.
    if (m_receiver.joinable())
    {
        std::cout << "[CLIENT] Attente de la fin du thread réseau..." << std::endl;
        m_receiver.join(); // Attend la fin du thread
        std::cout << "[CLIENT] Thread réseau terminé." << std::endl;
    }
    else
    {
        std::cout << "[CLIENT] Thread réseau non joignable." << std::endl;
    }
}

// --- ✅ Nouvelles fonctions d'envoi (Implémentation) ---
void RTypeClient::sendListRoomsRequest()
{
    Protocol::ListRoomsRequestMessage msg;
    auto data = msg.serialize();
    m_socket.send_to(asio::buffer(data), m_server_endpoint);
    std::cout << "[CLIENT] LIST_ROOMS_REQUEST envoyé\n";
}
void RTypeClient::sendLeaveRoomRequest(uint32_t roomId)
{
    ProtocolData::LeaveRoomRequest data;
    data.roomId = roomId;
    Protocol::LeaveRoomRequestMessage request(data);
    auto tosend = request.serialize();
    m_socket.send_to(asio::buffer(tosend), m_server_endpoint);
    std::cout << "[CLIENT] LEAVE_ROOM envoyé\n";
}
void RTypeClient::sendCreateRoomRequest()
{
    Protocol::CreateRoomRequestMessage msg;
    auto data = msg.serialize();
    m_socket.send_to(asio::buffer(data), m_server_endpoint);
    std::cout << "[CLIENT] CREATE_ROOM_REQUEST envoyé\n";
}

void RTypeClient::sendJoinRoomRequest(uint32_t roomId)
{
    ProtocolData::JoinRoomRequest reqData;
    reqData.roomId = roomId; // La sérialisation gérera htonl
    Protocol::JoinRoomRequestMessage msg(reqData);
    auto data = msg.serialize();
    m_socket.send_to(asio::buffer(data), m_server_endpoint);
    std::cout << "[CLIENT] JOIN_ROOM_REQUEST (Room " << roomId << ") envoyé\n";
}

// --- receive_loop (Utilise le buffer brut) ---
void RTypeClient::receive_loop()
{
    std::array<uint8_t, 8192> recv_buffer; //
    asio::ip::udp::endpoint sender_endpoint;
    while (m_running)
    {
        asio::error_code error;
        size_t len = m_socket.receive_from(asio::buffer(recv_buffer), sender_endpoint, 0, error);
        if (error == asio::error::would_block)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        // Gérer la fermeture propre
        if (error == asio::error::operation_aborted || error == asio::error::bad_descriptor)
        {
            std::cout << "[CLIENT] Socket fermé, sortie de la boucle de réception." << std::endl;
            break;
        }
        if (error || len < sizeof(ProtocolData::PacketHeader) || sender_endpoint != m_server_endpoint)
        {
            if (error)
                std::cerr << "[CLIENT NET ERROR] receive_from: " << error.message() << std::endl;
            continue;
        }

        handle_message(recv_buffer.data(), len);
    }
    std::cout << "[CLIENT] Thread de réception terminé." << std::endl;
}

// --- handle_message (Modifié pour MessageFactory et callbacks) ---
void RTypeClient::handle_message(const uint8_t *buffer_data, size_t len)
{
    std::vector<uint8_t> message_buffer(buffer_data, buffer_data + len);

    try
    {
        // Utilise la Factory pour désérialiser
        std::unique_ptr<Protocol::IMessage> message = Protocol::MessageFactory::deserialize(message_buffer);
        if (!message)
            return; // Type inconnu ?

        auto type = message->getType();

        switch (type)
        {
        case MessageType::WELCOME:
        {
            auto *welcomeMsg = static_cast<Protocol::WelcomeMessage *>(message.get());
            const auto &welcomeData = welcomeMsg->getData();

            uint32_t playerId = welcomeData.playerId; // Déjà en host byte order (factory a fait ntohl)

            std::cout << "[CLIENT] WELCOME reçu. PlayerId: " << playerId
                      << ", Accepted: " << (int)welcomeData.accepted << std::endl;

            // ✅ Passe TOUTES les données au handler
            if (m_welcomeHandler)
            {
                m_welcomeHandler(playerId, welcomeData);
            }
            break;
        }
        case MessageType::SNAPSHOT:
        {
            auto *snapshotMsg = static_cast<Protocol::SnapshotMessage *>(message.get());
            if (m_snapshotHandler)
                m_snapshotHandler(snapshotMsg->getData());
            break;
        }
        case MessageType::PLAYER_EVENT:
        {
            auto *eventMsg = static_cast<Protocol::PlayerEventMessage *>(message.get());
            if (m_playerEventHandler)
                m_playerEventHandler(eventMsg->getData());
            break;
        }

        // --- ✅ GESTION DES RÉPONSES DU LOBBY ---
        case MessageType::ROOM_LIST_RESPONSE:
        {
            auto *listMsg = static_cast<Protocol::RoomListResponseMessage *>(message.get());
            if (m_roomListHandler)
                m_roomListHandler(listMsg->getData());
            break;
        }
        case MessageType::CREATE_ROOM_RESPONSE:
        case MessageType::JOIN_ROOM_RESPONSE:
        {
            // ✅ CORRECTION : Caster vers les classes spécifiques (Create/Join Room Response)
            // Note: Nous castons vers la classe spécifique qui contient le payload RoomResponse.
            // Le MessageType (type) est déjà distinct et est passé au handler.
            if (type == MessageType::CREATE_ROOM_RESPONSE)
            {
                auto *responseMsg = static_cast<Protocol::CreateRoomResponseMessage *>(message.get()); // ✅ CORRIGÉ
                if (m_roomResponseHandler)
                    m_roomResponseHandler(type, responseMsg->getData());
            }
            else
            {                                                                                        // JOIN_ROOM_RESPONSE
                auto *responseMsg = static_cast<Protocol::JoinRoomResponseMessage *>(message.get()); // ✅ CORRIGÉ
                if (m_roomResponseHandler)
                    m_roomResponseHandler(type, responseMsg->getData());
            }
            break;
        }
        case MessageType::PLAYER_JOINED_ROOM:
        case MessageType::PLAYER_LEFT_ROOM:
        {
            // ✅ CORRECTION : Caster vers la classe spécifique pour les notifications
            if (type == MessageType::PLAYER_JOINED_ROOM)
            {
                auto *notifMsg = static_cast<Protocol::PlayerJoinedRoomMessage *>(message.get()); // ✅ CORRIGÉ
                if (m_playerNotificationHandler)
                    m_playerNotificationHandler(type, notifMsg->getData());
            }
            else
            {                                                                                   // PLAYER_LEFT_ROOM
                auto *notifMsg = static_cast<Protocol::PlayerLeftRoomMessage *>(message.get()); // ✅ CORRIGÉ
                if (m_playerNotificationHandler)
                    m_playerNotificationHandler(type, notifMsg->getData());
            }
            break;
        }
        case MessageType::GAME_STARTING:
        {
            std::cout << "[CLIENT] GAME_STARTING reçu!" << std::endl;
            if (m_gameStartingHandler)
            {
                std::cout << "[CLIENT] ✅ Appel du handler GAME_STARTING" << std::endl;
                m_gameStartingHandler();
            }
            else
            {
                std::cout << "[CLIENT] ❌ Handler GAME_STARTING non défini!" << std::endl;
            }
            break;
        }

        case MessageType::DELTA_SNAPSHOT:
        {
            auto *deltaMsg = static_cast<Protocol::DeltaSnapshotMessage *>(message.get());

            // ✅ CORRIGÉ : Vérifier si handler existe, puis l'appeler
            if (m_DeltaSnapshotHandler)
            {
                m_DeltaSnapshotHandler(deltaMsg->getData());
            }
            else
            {
                std::cerr << "[CLIENT] ⚠️ DeltaSnapshot reçu mais handler non défini" << std::endl;
            }
            break;
        }

        default:
            std::cout << "[CLIENT] Message reçu type " << int(type) << " non géré." << std::endl;
            break;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "[CLIENT ERROR] Échec désérialisation: " << e.what() << std::endl;
    }
}