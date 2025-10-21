#include "../include/server/server.hpp"

UdpServer::UdpServer(asio::io_context &io_context, unsigned short port,
                     std::shared_ptr<ThreadSafeQueue<NetworkPacket>> incomingMessages)
    : socket_(io_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), port)),
      m_incomingMessages(incomingMessages)
{
    std::cout << "[THREAD RESEAU] Serveur UDP démarré sur le port " << port << std::endl;
}

void UdpServer::startReceive() {
    startReceiveLoop();
}

void UdpServer::startReceiveLoop()
{
    auto buffer = std::make_shared<std::array<char, 1024>>();
    auto senderEndpoint = std::make_shared<asio::ip::udp::endpoint>();

    socket_.async_receive_from(
        asio::buffer(*buffer), *senderEndpoint,
        [this, buffer, senderEndpoint](const asio::error_code &error, std::size_t bytes_recvd)
        {
            if (!error && bytes_recvd > 0)
            {
                std::vector<uint8_t> data(buffer->begin(), buffer->begin() + bytes_recvd);
                m_incomingMessages->push(NetworkPacket{*senderEndpoint, std::move(data)});
            }
            startReceiveLoop();
        });
}

void UdpServer::send(const std::vector<uint8_t>& data, const asio::ip::udp::endpoint& endpoint) {
    socket_.async_send_to(asio::buffer(data), endpoint,
        [](const asio::error_code& ec, std::size_t) {
            if (ec) std::cerr << "[THREAD RESEAU] Erreur d'envoi: " << ec.message() << std::endl;
        });
}