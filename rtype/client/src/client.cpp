
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
        std::cout << "[CLIENT] SNAPSHOT reçu (" << len << " octets)." << std::endl;
        break;
    case MessageType::ERROR:
        std::cout << "[CLIENT] ERROR reçu." << std::endl;
        break;
    default:
        std::cout << "[CLIENT] Message reçu type " << int(type) << " (" << len << " octets)." << std::endl;
        break;
    }
}