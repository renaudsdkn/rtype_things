#pragma once
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include "ThreadSafeQueue.hpp"
#include "server.hpp"
#include "PlayerManager.hpp"
#include "message_handler.hpp"
#include "room.hpp"

class GameManager {
public:
    GameManager(
        std::shared_ptr<ThreadSafeQueue<NetworkPacket>> incoming,
        std::shared_ptr<UdpServer> server);
    void run();
    void stop();

    // Logique de jeu appelée par le MessageHandler
    void handleNewPlayer(uint32_t playerId, const asio::ip::udp::endpoint& endpoint);
    void handlePlayerDisconnect(uint32_t playerId);
    void handlePlayerInput(uint32_t playerId, const ProtocolData::PlayerInput& input);
    
private:
    void processNetworkInputs();
    void updateGame(float deltaTime);
    void broadcastSnapshots();
    void cleanupPlayers();
    Room* findAvailableRoom();

    std::atomic<bool> m_running{true};
    std::shared_ptr<ThreadSafeQueue<NetworkPacket>> m_incomingMessages;
    std::shared_ptr<UdpServer> m_server;
    
    PlayerManager m_playerManager;
    MessageHandler m_messageHandler;
    
    std::vector<std::unique_ptr<Room>> m_rooms;
    std::unordered_map<uint32_t, Room*> m_playerToRoomMap;
    std::chrono::steady_clock::time_point m_lastCleanupTime;
};