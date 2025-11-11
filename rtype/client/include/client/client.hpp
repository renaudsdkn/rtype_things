// Dans rtype/client/include/client/client.hpp
#pragma once

#include <asio.hpp>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <cstdint>
#include <iostream>
#include <functional>                 // ✅ Pour std::function
#include "protocol/protocol_data.hpp" // ✅ Pour Snapshot et Welcome

// Dans rtype/client/include/client/client.hpp
#pragma once
// ... (includes asio, thread, atomic, etc.)
#include <functional> // Pour std::function
#include "protocol/protocol_data.hpp"

class RTypeClient
{
public:
    // ✅ Définir TOUS les types de callbacks
    using SnapshotHandler = std::function<void(const ProtocolData::Snapshot &)>;
    using PlayerEventHandler = std::function<void(const ProtocolData::PlayerEvent &)>;
    using RoomListHandler = std::function<void(const ProtocolData::RoomList &)>;
    using RoomResponseHandler = std::function<void(ProtocolData::MessageType, const ProtocolData::RoomResponse &)>;
    using PlayerNotificationHandler = std::function<void(ProtocolData::MessageType, const ProtocolData::PlayerRoomNotification &)>;
    using GameStartingHandler = std::function<void()>; // ✅ Pour lancer le jeu
    using WelcomeHandler = std::function<void(uint32_t, const ProtocolData::Welcome &)>;
    using DeltaSnapshotHandler = std::function<void(const ProtocolData::DeltaSnapshot &)>;
    asio::io_context m_io;
    asio::ip::udp::socket m_socket;
    // ... (autres membres)
    asio::ip::udp::endpoint m_server_endpoint;
    std::thread m_receiver;
    RTypeClient(const std::string &server_ip, unsigned short server_port);
    ~RTypeClient();

    void start();
    void stop();

    // --- Actions d'envoi ---
    void send_connect(const std::string &nickname = "");
    void send_input(const ProtocolData::PlayerInput &input);
    void send_disconnect();
    void sendRaw(const std::vector<uint8_t> &data);
    // ✅ Nouvelles méthodes d'envoi pour le lobby
    void sendListRoomsRequest();
    void sendCreateRoomRequest();
    void sendJoinRoomRequest(uint32_t roomId);
    void sendLeaveRoomRequest(uint32_t roomId);

private:
    void receive_loop();
    // ✅ Doit prendre le buffer brut pour le MessageFactory
    void handle_message(const uint8_t *buffer_data, size_t len);

    std::atomic<bool> m_running{false};
    uint32_t m_playerId = 0;

    // ✅ Stocker tous les callbacks
    SnapshotHandler m_snapshotHandler;
    WelcomeHandler m_welcomeHandler;
    PlayerEventHandler m_playerEventHandler;
    RoomListHandler m_roomListHandler;
    RoomResponseHandler m_roomResponseHandler;
    PlayerNotificationHandler m_playerNotificationHandler;
    GameStartingHandler m_gameStartingHandler;
    DeltaSnapshotHandler m_DeltaSnapshotHandler;

public:
    // ✅ Setters pour les callbacks (à implémenter dans client.cpp)
    void setSnapshotHandler(SnapshotHandler handler) { m_snapshotHandler = std::move(handler); }
    void setWelcomeHandler(WelcomeHandler handler) { m_welcomeHandler = std::move(handler); }
    void setPlayerEventHandler(PlayerEventHandler handler) { m_playerEventHandler = std::move(handler); }
    void setRoomListHandler(RoomListHandler handler) { m_roomListHandler = std::move(handler); }
    void setRoomResponseHandler(RoomResponseHandler handler) { m_roomResponseHandler = std::move(handler); }
    void setPlayerNotificationHandler(PlayerNotificationHandler handler) { m_playerNotificationHandler = std::move(handler); }
    void setGameStartingHandler(GameStartingHandler handler) { m_gameStartingHandler = std::move(handler); }
    void setDeltaSnapshotHandler(DeltaSnapshotHandler handler) { m_DeltaSnapshotHandler = std::move(handler);}
};