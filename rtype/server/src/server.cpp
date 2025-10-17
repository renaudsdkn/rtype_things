#include "../include/server/server.hpp"

 UdpServer::UdpServer(asio::io_context &io_context, unsigned short port)
        : socket_(io_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), port)),
          playerManager_(),
          handler_(playerManager_),
          cleanup_timer_(io_context)
    {
        std::cout << "[SERVER] UDP Server started on port " << port << std::endl;
        start_receive();
        start_cleanup_timer();
    }
void UdpServer::start_receive()
{
    socket_.async_receive_from(
        asio::buffer(recv_buffer_), remote_endpoint_,
        [this](const asio::error_code &error, std::size_t bytes_recvd)
        {
            if (!error)
            {
                process_datagram(bytes_recvd);
                start_receive();
            }
            else
            {
                std::cerr << "[ERROR] Receive failed: " << error.message() << std::endl;
            }
        });
}

void UdpServer::process_datagram(std::size_t length)
{
    std::vector<uint8_t> buffer(recv_buffer_.begin(), recv_buffer_.begin() + length);

    try
    {
        playerManager_.updatePlayerActivity(remote_endpoint_);
        auto message = Protocol::MessageFactory::deserialize(buffer);
        handler_.handleMessage(message, socket_, remote_endpoint_);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERROR] Deserialization failed from "
                  << remote_endpoint_.address().to_string() << ": "
                  << e.what() << std::endl;
    }
}

void UdpServer::start_cleanup_timer()
{
    cleanup_timer_.expires_after(std::chrono::seconds(2));
    cleanup_timer_.async_wait([this](const asio::error_code &ec)
                              {
        if (!ec) {
            cleanup_inactive_clients();
            start_cleanup_timer();
        } });
}

void UdpServer::cleanup_inactive_clients()
{
    playerManager_.removeInactivePlayers(std::chrono::seconds(5));
}

