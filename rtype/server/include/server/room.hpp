#pragma once
#include <asio.hpp>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <memory>
#include <chrono> //
#include "../include/ecs/engine.hpp" // ✅ INCLURE L'EN-TÊTE DE TA LIBRAIRIE
#include "../include/protocol/protocol_data.hpp"

class Room {
public:
    explicit Room(uint32_t roomId);

    // --- Fonctions appelées par le GameManager ---
    void addPlayer(uint32_t playerId, const asio::ip::udp::endpoint& endpoint);
    void removePlayer(uint32_t playerId);
    void handleInput(uint32_t playerId, const ProtocolData::PlayerInput& input);
    void update(float deltaTime);

    // --- Fonctions utilitaires ---
    bool isFull() const;
    bool isEmpty() const;
    uint32_t getId() const;
    std::vector<asio::ip::udp::endpoint> getPlayerEndpoints() const;
    ProtocolData::Snapshot getSnapshot() const;

private:
    uint32_t m_id;
    std::unique_ptr<Engine> m_engine; // ✅ CHAQUE ROOM POSSÈDE SON MOTEUR
    // ...
    std::chrono::steady_clock::time_point m_lastEnemySpawnTime; // ✅ Suivi du temps
    float m_spawnInterval = 5.0f; // ✅ Toutes les 5 secondes (par exemple)
    std::unordered_map<uint32_t, asio::ip::udp::endpoint> m_players;
};