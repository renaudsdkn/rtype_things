#pragma once

#include <asio.hpp>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <iostream>
#include "../include/protocol/serializer.hpp"

class RTypeClient {
public:
    RTypeClient(const std::string& server_ip, unsigned short server_port);
    ~RTypeClient();

    void start();
    void stop();

    // Exemples d'actions côté client
    void send_connect();
    void send_input(const ProtocolData::PlayerInput& input);
    void send_ping();
    void send_disconnect();

    // Pour récupérer l'id joueur après le WELCOME
    uint32_t getPlayerId() const { return m_playerId; }

private:
    void receive_loop();
    void handle_message(const ProtocolData::PacketHeader* header, const uint8_t* data, size_t len);

    asio::io_context m_io;
    asio::ip::udp::socket m_socket;
    asio::ip::udp::endpoint m_server_endpoint;

    std::thread m_receiver;
    std::atomic<bool> m_running{false};
    uint32_t m_playerId = 0; // Id attribué par le serveur
};