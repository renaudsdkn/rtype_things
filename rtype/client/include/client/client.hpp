// Dans rtype/client/include/client/client.hpp
#pragma once

#include <asio.hpp>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <cstdint>
#include <iostream>
#include <functional> // ✅ Pour std::function
#include "protocol/protocol_data.hpp" // ✅ Pour Snapshot et Welcome

class RTypeClient {
public:
    // ✅ Types de callbacks
    using SnapshotHandler = std::function<void(const ProtocolData::Snapshot&)>;
    using WelcomeHandler = std::function<void(uint32_t)>;
    using PlayerEventHandler = std::function<void(const ProtocolData::PlayerEvent&)>;

    // ✅ Constructeur SIMPLE (sans GameClient)
    RTypeClient(const std::string& server_ip, unsigned short server_port);
    ~RTypeClient();

    void start();
    void stop();

    // Envoi (inchangé)
    void send_connect();
    void send_input(const ProtocolData::PlayerInput& input);
    void send_ping();
    void send_disconnect();

    // ✅ Méthodes pour définir les callbacks
    void setSnapshotHandler(SnapshotHandler handler);
    void setWelcomeHandler(WelcomeHandler handler);
    void setPlayerEventHandler(PlayerEventHandler handler);

    uint32_t getPlayerId() const { return m_playerId; } // Optionnel
 
private:
    void receive_loop();
    // ✅ Utiliser le buffer brut pour MessageFactory
    void handle_message(const uint8_t* buffer_data, size_t len);

    asio::io_context m_io; // Pourrait être externe
    asio::ip::udp::socket m_socket;
    asio::ip::udp::endpoint m_server_endpoint;

    std::thread m_receiver;
    std::atomic<bool> m_running{false};
    uint32_t m_playerId = 0;

    // ✅ Membres pour stocker les callbacks
    SnapshotHandler m_snapshotHandler;
    WelcomeHandler m_welcomeHandler;
    PlayerEventHandler m_playerEventHandler;
    

    // ❌ PAS de référence GameClient& m_gameClient;
};