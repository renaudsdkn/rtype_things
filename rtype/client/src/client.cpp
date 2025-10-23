
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
// Dans client.cpp

// ... (autres fonctions) ...

void RTypeClient::stop()
{
    std::cout << "[CLIENT] Arrêt demandé..." << std::endl;
    m_running = false; // Signale au thread de s'arrêter logiquement

    // ✅ Ferme le socket depuis ce thread (le thread principal)
    // Cela va provoquer le déblocage immédiat de receive_from() dans l'autre thread,
    // qui retournera une erreur spécifique.
    if (m_socket.is_open()) {
        asio::error_code ec;
        // Optionnel: Tenter une fermeture plus propre
        m_socket.shutdown(asio::ip::udp::socket::shutdown_both, ec);
        // Fermer le socket
        m_socket.close(ec);
        if (ec) {
             std::cerr << "[CLIENT] Erreur lors de la fermeture du socket: " << ec.message() << std::endl;
        } else {
             std::cout << "[CLIENT] Socket fermé." << std::endl;
        }
    }

    // Attend maintenant que le thread receive_loop se termine effectivement.
    // Comme le socket est fermé, receive_from va retourner, la boucle while(m_running)
    // sera fausse (ou on sortira à cause de l'erreur), et le thread finira.
    if (m_receiver.joinable()) {
        std::cout << "[CLIENT] Attente de la fin du thread réseau..." << std::endl;
        m_receiver.join(); // Attend la fin du thread
        std::cout << "[CLIENT] Thread réseau terminé." << std::endl;
    } else {
         std::cout << "[CLIENT] Thread réseau non joignable." << std::endl;
    }
}

void RTypeClient::receive_loop()
{
    std::cout << "[CLIENT] Thread de réception démarré." << std::endl;
    std::array<uint8_t, 2048> recv_buffer; // Utilise std::array
    asio::ip::udp::endpoint sender_endpoint;

    while (m_running) // Vérifie AVANT l'appel bloquant
    {
        asio::error_code error;
        size_t len = 0;
        try {
            // Appel potentiellement bloquant
            len = m_socket.receive_from(asio::buffer(recv_buffer), sender_endpoint, 0, error);
        } catch (const std::system_error& e) {
            // Gère l'exception si le socket est fermé pendant l'appel
             if (m_running) { // N'affiche l'erreur que si on ne s'attendait pas à s'arrêter
                 std::cerr << "[CLIENT NET] receive_from exception: " << e.what() << std::endl;
             }
            break; // Sort de la boucle
        }

        if (!m_running) break; // Re-vérifie APRÈS l'appel bloquant

        // ✅ Gérer l'erreur spécifique causée par la fermeture du socket
        if (error == asio::error::bad_descriptor /* Linux? */ ||
            error == asio::error::operation_aborted /* Peut arriver aussi */ ||
            error.value() == 9 /* Bad file descriptor, souvent sur close */
            /* Ajouter d'autres codes d'erreur Windows si nécessaire */
            ) {
            std::cout << "[CLIENT] Socket fermé, sortie de la boucle de réception." << std::endl;
            break; // Sortir proprement
        }

        // Gérer les autres erreurs ou conditions de continuation
        if (error || len < sizeof(ProtocolData::PacketHeader) || sender_endpoint != m_server_endpoint) {
             if (error) {
                 // Optionnel: Logguer les erreurs non fatales
                 // std::cerr << "[CLIENT NET WARNING] receive_from error: " << error.message() << std::endl;
             }
            continue; // Ignore le paquet et continue
        }

        // Si tout va bien, traiter le message
        handle_message(recv_buffer.data(), len);
    }
    std::cout << "[CLIENT] Thread de réception terminé." << std::endl;
}

// ... (handle_message et le reste) ...

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
                /*std::cout << "    - ID: " << entity.id
                          << ", T: " << static_cast<int>(entity.type)          // Affiche le type comme un nombre
                          << ", P: (" << entity.x << ", " << entity.y << ")"   // Affiche la position
                          << ", V: (" << entity.vx << ", " << entity.vy << ")" // Affiche la vélocité
                          << ", Dmg: " << static_cast<int>(entity.damage)      // Affiche les dégâts
                          << ", XP: " << static_cast<int>(entity.xp)           // Affiche l'XP
                          << ", Lvl: " << static_cast<int>(entity.level)       // Affiche le niveau
                          << std::endl;*/
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