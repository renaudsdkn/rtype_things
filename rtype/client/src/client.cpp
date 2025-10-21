
#include "../include/client/client.hpp"
using namespace Protocol;
using namespace ProtocolData;

RTypeClient::RTypeClient(const std::string &server_ip, unsigned short server_port)
    : m_socket(m_io), m_server_endpoint(asio::ip::make_address(server_ip), server_port)
{
    m_socket.open(asio::ip::udp::v4());
}

RTypeClient::~RTypeClient()
{
    stop();
}

void RTypeClient::start()
{
    m_running = true;
    m_receiver = std::thread(&RTypeClient::receive_loop, this);
    send_connect();
}

void RTypeClient::stop()
{
    m_running = false;
    if (m_socket.is_open())
        m_socket.close();
    if (m_receiver.joinable())
        m_receiver.join();
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

void RTypeClient::receive_loop()
{
    uint8_t recv_buffer[1024];
    asio::ip::udp::endpoint sender_endpoint;
    while (m_running)
    {
        asio::error_code error;
        size_t len = m_socket.receive_from(asio::buffer(recv_buffer), sender_endpoint, 0, error);
        if (error || len < sizeof(PacketHeader))
            continue;

        auto *header = reinterpret_cast<const PacketHeader *>(recv_buffer);
        handle_message(header, recv_buffer + sizeof(PacketHeader), len - sizeof(PacketHeader));
    }
}

void RTypeClient::handle_message(const PacketHeader *header, const uint8_t *data, size_t len)
{
    auto type = static_cast<MessageType>(header->type);

    switch (type)
    {
    case MessageType::WELCOME:
    {
        if (len >= sizeof(Welcome))
        {
            auto *welcome = reinterpret_cast<const Welcome *>(data);
            m_playerId = ntohl(welcome->playerId);
            std::cout << "[CLIENT] WELCOME reçu. PlayerId: " << m_playerId << std::endl;
        }
        break;
    }
    case MessageType::PING_RESPONSE:
        std::cout << "[CLIENT] PING_RESPONSE reçu." << std::endl;
        break;
    case MessageType::SNAPSHOT:
    {
        std::cout << "[CLIENT] SNAPSHOT reçu (" << len << " octets)." << std::endl;

        // --- Début de la Désérialisation ---
        if (len < sizeof(uint32_t))
        { // Vérifie s'il y a au moins la place pour le compteur
            std::cerr << "[CLIENT ERROR] Snapshot trop petit pour contenir le nombre d'entités." << std::endl;
            break;
        }

        const uint8_t *ptr = data; // Pointeur pour parcourir les données reçues

        // 1. Lire le nombre d'entités (convertir depuis Big Endian)
        uint32_t count = ntohl(*reinterpret_cast<const uint32_t *>(ptr));
        ptr += sizeof(uint32_t);
        size_t expected_size = sizeof(uint32_t) + count * sizeof(ProtocolData::entity_state); // Taille attendue du payload

        if (len != expected_size)
        {
            std::cerr << "[CLIENT ERROR] Taille de Snapshot incohérente. Reçu: " << len
                      << ", Attendu: " << expected_size << " pour " << count << " entités." << std::endl;
            break;
        }

        std::cout << "  Contient " << count << " entités :" << std::endl;

        // 2. Lire chaque entité
        for (uint32_t i = 0; i < count; ++i)
        {
            // S'assurer qu'on ne dépasse pas la taille du buffer (sécurité)
            if (ptr + sizeof(ProtocolData::entity_state) > data + len)
            {
                std::cerr << "[CLIENT ERROR] Dépassement de buffer lors de la lecture de l'entité " << i << std::endl;
                break;
            }

            const ProtocolData::entity_state *state_ptr = reinterpret_cast<const ProtocolData::entity_state *>(ptr);

            uint32_t entity_id = ntohl(state_ptr->id); // Convertir l'ID
            uint8_t entity_type = state_ptr->type;
            // Lire x et y (si tu les as ajoutés à entity_state)
            float entity_x = state_ptr->x;
            float entity_y = state_ptr->y;

            // 3. Afficher les infos
            std::cout << "    - Entité ID: " << entity_id
                      << ", Type: " << static_cast<int>(entity_type)
                       << ", Pos: (" << entity_x << ", " << entity_y << ")" // Décommente quand x,y sont là
                      << std::endl;

            ptr += sizeof(ProtocolData::entity_state); // Avancer le pointeur
        }
        // --- Fin de la Désérialisation ---

        // Prochaine étape : Utiliser ces données pour mettre à jour l'ECS du client !

        break;
    }
    case MessageType::ERROR:
        std::cout << "[CLIENT] ERROR reçu." << std::endl;
        break;
    default:
        std::cout << "[CLIENT] Message reçu type " << int(type) << " (" << len << " octets)." << std::endl;
        break;
    }
}