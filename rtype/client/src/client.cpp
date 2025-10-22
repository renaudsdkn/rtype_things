
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
    m_receiver = std::thread(&RTypeClient::receive_loop, this);
    // Optionnel: Thread pour m_io.run() si Asio est utilisé ailleurs
    send_connect();
}

void RTypeClient::stop()
{
    m_running = false;
    m_io.stop();
    // Fermer le socket peut causer une exception dans receive_from,
    // il vaut mieux gérer ça proprement dans receive_loop.
    // Ou fermer après avoir joint le thread.
    // if (m_socket.is_open()) m_socket.close(); // Peut être dangereux si receive_from est bloquant
    if (m_receiver.joinable())
        m_receiver.join();
    if (m_socket.is_open())
        m_socket.close(); // Fermer après join
}

void RTypeClient::send_connect()
{
    ConnectMessage msg;
    auto data = msg.serialize();
    m_socket.send_to(asio::buffer(data), m_server_endpoint);
    std::cout << "[CLIENT] CONNECT envoyé\n";
}

void RTypeClient::send_input(const PlayerInput &input)
{
    PlayerInputMessage msg(input);
    auto data = msg.serialize();
    m_socket.send_to(asio::buffer(data), m_server_endpoint);
    std::cout << "[CLIENT] INPUT envoyé\n";
}

void RTypeClient::send_ping()
{
    PacketHeader header{htons(sizeof(PacketHeader)), static_cast<uint8_t>(MessageType::PING)};
    std::vector<uint8_t> buffer(sizeof(header));
    std::memcpy(buffer.data(), &header, sizeof(header));
    m_socket.send_to(asio::buffer(buffer), m_server_endpoint);
    std::cout << "[CLIENT] PING envoyé\n";
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
void RTypeClient::setSnapshotHandler(SnapshotHandler handler)
{
    m_snapshotHandler = std::move(handler); // Stocke la fonction passée
}
void RTypeClient::setWelcomeHandler(WelcomeHandler handler)
{
    m_welcomeHandler = std::move(handler); // Stocke la fonction passée
}

// --- receive_loop (Passe le buffer brut) ---
void RTypeClient::receive_loop()
{
    std::array<uint8_t, 2048> recv_buffer;
    asio::ip::udp::endpoint sender_endpoint;
    while (m_running)
    {
        asio::error_code error;
        size_t len = 0;
        try
        {
            // Utiliser receive_from synchrone dans ce thread dédié
            len = m_socket.receive_from(asio::buffer(recv_buffer), sender_endpoint, 0, error);
        }
        catch (const std::system_error &e)
        {
            // Capturer l'exception si le socket est fermé pendant l'appel bloquant
            std::cerr << "[CLIENT NET] receive_from exception (normal si arrêt): " << e.what() << std::endl;
            break; // Sortir de la boucle si le socket est fermé
        }

        if (!m_running)
            break; // Vérifier après l'appel potentiellement bloquant

        if (error == asio::error::operation_aborted)
        {
            break; // Sortir proprement si io_context est stoppé
        }
        else if (error || len < sizeof(ProtocolData::PacketHeader) || sender_endpoint != m_server_endpoint)
        {
            if (error)
            {
                // std::cerr << "[CLIENT NET WARNING] receive_from error: " << error.message() << std::endl;
            }
            continue;
        }

        handle_message(recv_buffer.data(), len);
    }
    std::cout << "[CLIENT] Thread de réception terminé." << std::endl;
}

// --- handle_message (Utilise Factory et Appelle les Callbacks) ---
void RTypeClient::handle_message(const uint8_t *buffer_data, size_t len)
{
    std::vector<uint8_t> message_buffer(buffer_data, buffer_data + len);
    try
    {
        // Désérialise via la Factory
        std::unique_ptr<Protocol::IMessage> message = Protocol::MessageFactory::deserialize(message_buffer);
        if (!message)
            return;
        auto type = message->getType();

        switch (type)
        {
        case ProtocolData::MessageType::SNAPSHOT:
        {
            // Tente de désérialiser via la Factory (ou manuellement si tu préfères)
            std::unique_ptr<Protocol::IMessage> snapshotMessage;
            try
            {
                // Remplace 'message_buffer' par le vecteur contenant les données brutes du paquet
                // (Tu devras adapter cette partie en fonction de comment handle_message reçoit les données)
                std::vector<uint8_t> message_buffer(buffer_data, buffer_data + len); // Crée le vecteur à partir du pointeur brut
                snapshotMessage = Protocol::MessageFactory::deserialize(message_buffer);
                if (!snapshotMessage)
                {
                    std::cerr << "[CLIENT ERROR] SNAPSHOT: Désérialisation échouée (nullptr)." << std::endl;
                    break;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "[CLIENT ERROR] SNAPSHOT: Exception désérialisation: " << e.what() << std::endl;
                break;
            }

            // Cast pour accéder aux données spécifiques du Snapshot
            auto *snapshotMsg = static_cast<Protocol::SnapshotMessage *>(snapshotMessage.get());
            const ProtocolData::Snapshot &snapData = snapshotMsg->getData();

            // --- ✅ Affichage Détaillé du Contenu ---
            std::cout << "[CLIENT] SNAPSHOT reçu contenant " << snapData.entities.size() << " entités :" << std::endl;
            // Boucle sur chaque entité dans le snapshot
            for (const auto &entity : snapData.entities)
            {
                // Affiche les informations de chaque entité
                // Note : Les données comme 'id' sont déjà dans l'ordre de l'hôte grâce à la factory
                std::cout << "    - ID: " << entity.id
                          << ", T: " << static_cast<int>(entity.type)          // Affiche le type comme un nombre
                          << ", P: (" << entity.x << ", " << entity.y << ")"   // Affiche la position
                          << ", V: (" << entity.vx << ", " << entity.vy << ")" // Affiche la vélocité
                          << ", Dmg: " << static_cast<int>(entity.damage)      // Affiche les dégâts
                          << ", XP: " << static_cast<int>(entity.xp)           // Affiche l'XP
                          << ", Lvl: " << static_cast<int>(entity.level)       // Affiche le niveau
                          << std::endl;
            }
            // --- Fin Affichage ---

            // Appelle le callback SnapshotHandler s'il a été défini
            if (m_snapshotHandler)
            {
                m_snapshotHandler(snapData); // Passe le snapshot reçu au GameClient
            }
            else
            {
                // Optionnel : Log si le handler n'est pas prêt
                // std::cout << "[CLIENT WARNING] SnapshotHandler non défini." << std::endl;
            }
            break;
        }
        // ... (gérer PING_RESPONSE, ERROR, SPAWN_ENTITY, DESTROY_ENTITY...)
        default:
            break;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "[CLIENT ERROR] Échec désérialisation/traitement: " << e.what() << std::endl;
    }
}