#pragma once

#include <asio.hpp>
#include <vector>
#include <string>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <atomic>
#include "protocol/protocol_data.hpp"
#include "../include/ecs/engine.hpp" // Assure-toi que le chemin est bon
#include "../include/server/GameManager.hpp" // ✅ Inclure GameManager
#include "protocol/serializer.hpp" // Pour PlayerEventMessage
#include <iostream>
#include <cstdlib> // Pour rand()
// Déclaration anticipée
class GameManager;

class Room {
public:
    enum class State { WAITING_FOR_PLAYERS,  STARTING,  PLAYING, GAME_OVER, FINISHED };

    // ✅ Constructeur prend une référence au GameManager
    explicit Room(uint32_t roomId, GameManager& gameManager, const ProtocolData::RoomConfig& config);

    void addPlayer(uint32_t playerId, const asio::ip::udp::endpoint& endpoint);
    void removePlayer(uint32_t playerId);
    void handleInput(uint32_t playerId, const ProtocolData::PlayerInput &input);
    void update(float deltaTime);

    ProtocolData::Snapshot getSnapshot() const;

    std::vector<Engine::PlayerScoreInfo> getFinalPlayerScores() const;
    // --- Accesseurs ---
    bool isFull() const;
    bool isEmpty() const;
    uint32_t getId() const;
    State getCurrentState() const;
    void setState(State newState); // Pour passer à FINISHED après broadcast
    std::vector<asio::ip::udp::endpoint> getPlayerEndpoints() const;
    size_t getPlayerCount() const; // ✅ Fonction ajoutée
    const ProtocolData::RoomConfig& getConfig() const { return m_config; }
    const Engine* getEngine() const { return m_engine.get(); }
    Engine* getEngine() { return m_engine.get(); }
    void broadcastDeltaSnapshot();
private:
    uint32_t m_id;
    GameManager& m_gameManagerRef; // ✅ Référence au GameManager
    std::unique_ptr<Engine> m_engine;
    std::atomic<State> m_currentState{State::WAITING_FOR_PLAYERS};
    std::chrono::steady_clock::time_point m_gameStartTime;
    // Map pour stocker les joueurs de cette room (ID Joueur -> Endpoint)
    std::unordered_map<uint32_t, asio::ip::udp::endpoint> m_players; 
        
    ProtocolData::RoomConfig m_config;  
    std::chrono::steady_clock::time_point m_lastEnemySpawnTime;
    float m_spawnInterval;

    // ✅ NOUVEAU : Stocke le dernier snapshot envoyé à chaque joueur
    std::unordered_map<uint32_t, ProtocolData::Snapshot> m_playerLastSnapshots;
    
    // ✅ NOUVEAU : Numéro de séquence des snapshots
    uint32_t m_snapshotSequence = 0;
    
    // ✅ NOUVEAU : Calcule la différence entre deux snapshots
    std::vector<ProtocolData::EntityChange> computeDiff(
        const ProtocolData::Snapshot& oldSnap,
        const ProtocolData::Snapshot& newSnap
    );
    
    // ✅ NOUVEAU : Envoie un delta snapshot à un joueur
    void sendDeltaSnapshot(uint32_t playerId, const ProtocolData::DeltaSnapshot& delta);
};