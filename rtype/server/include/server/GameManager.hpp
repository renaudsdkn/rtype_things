#pragma once

#include "protocol/protocol_data.hpp"
#include "server.hpp" // Contient UdpServer
#include "PlayerManager.hpp"
#include "message_handler.hpp"
#include "room.hpp" // Assure-toi que ce header est correct
#include "ScoreFileWriter.hpp"
#include "ThreadSafeQueue.hpp"
#include <memory>
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <optional>

class UdpServer; // Déclaration anticipée
class Room;
class GameManager
{
public:
    // Constructeur mis à jour (prend UdpServer par shared_ptr)
    GameManager(
        std::shared_ptr<ThreadSafeQueue<NetworkPacket>> incoming,
        std::shared_ptr<UdpServer> server); // ✅ Prend shared_ptr

    void run();
    void stop();

    // --- Méthodes appelées par MessageHandler ---
    void handleNewPlayer(uint32_t playerId, const asio::ip::udp::endpoint &endpoint);
    void handlePlayerDisconnect(uint32_t playerId);
    void handlePlayerInput(uint32_t playerId, const ProtocolData::PlayerInput &input);
    void handleListRoomsRequest(uint32_t playerId);
    void handleCreateRoomRequest(uint32_t playerId, const ProtocolData::RoomConfig &config);
    void handleJoinRoomRequest(uint32_t playerId, uint32_t roomId);
    void handlePlayerLeft(uint32_t playerId); // Appelée par disconnect ou leave_room
    void sendToPlayer(uint32_t playerId, const std::vector<uint8_t> &data);
    // --- Helpers ---
    void sendToEndpoint(const asio::ip::udp::endpoint &endpoint, const std::vector<uint8_t> &data);
    void broadcastToRoom(uint32_t roomId, const std::vector<uint8_t> &data, std::optional<uint32_t> excludePlayerId = std::nullopt);
    Room *getRoomById(uint32_t roomId);
    ProtocolData::RoomList getRoomListData() const;
    std::optional<uint32_t> getRoomIdForPlayer(uint32_t playerId) const;

    const PlayerManager &getPlayerManager() const { return m_playerManager; }
    bool forceEndRoom(uint32_t roomId);
    bool kickPlayer(uint32_t playerId);
    void requestShutdown();
    std::vector<uint32_t> getPlayersInRoom(uint32_t roomId) const;
    PlayerManager &getPlayerManager() { return m_playerManager; }
     // ✅ CORRIGER : Retourne const ref sur vector de unique_ptr (pas shared_ptr)
    const std::vector<std::unique_ptr<Room>>& getRooms() const { return m_rooms; }
    

private:
    void processNetworkInputs();
    void updateGame(float deltaTime);
    void broadcastSnapshots();
    void broadcastGameEvents();
    void cleanupPlayers();
    Room *findAvailableRoomOrCreate(); // Renommé pour plus de clarté

    std::atomic<bool> m_running{true};
    std::shared_ptr<ThreadSafeQueue<NetworkPacket>> m_incomingMessages;
    std::shared_ptr<UdpServer> m_server; // ✅ Stocké comme shared_ptr

    PlayerManager m_playerManager;
    MessageHandler m_messageHandler;

    std::vector<std::unique_ptr<Room>> m_rooms;             // Liste des rooms
    std::unordered_map<uint32_t, Room *> m_playerToRoomMap; // Map Joueur ID -> Pointeur Room (ou nullptr si lobby)
    std::unordered_map<uint32_t, Room *> m_idToRoomMap;     // ✅ Map Room ID -> Pointeur Room
    mutable std::mutex m_roomsMutex;
    std::chrono::steady_clock::time_point m_lastCleanupTime;
};