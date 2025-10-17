#pragma once
#include <unordered_map>
#include <string>
#include <chrono>
#include <asio.hpp>
#include <optional>

struct PlayerInfo {
    uint32_t id;
    asio::ip::udp::endpoint endpoint;
    std::chrono::steady_clock::time_point lastSeen;
};

class PlayerManager {
public:
    uint32_t addPlayer(const asio::ip::udp::endpoint& ep);
    bool hasPlayer(const asio::ip::udp::endpoint& ep) const;
    void updatePlayerActivity(const asio::ip::udp::endpoint& ep);
    void removePlayer(const asio::ip::udp::endpoint& ep);
    void removeInactivePlayers(std::chrono::seconds timeout);
    void removePlayerById(uint32_t id);
    //std::optional<uint32_t> getPlayerIdByEndpoint(const asio::ip::udp::endpoint& ep) const;
    std::optional<uint32_t> getPlayerIdByEndpoint(const asio::ip::udp::endpoint& ep) const;
    std::vector<asio::ip::udp::endpoint> getAllEndpoints() const;

private:
    std::string makeKey(const asio::ip::udp::endpoint& ep) const;

    std::unordered_map<std::string, PlayerInfo> players_;
    uint32_t nextId_ = 1;  // compteur d’IDs uniques
    bool removed = false;
};
