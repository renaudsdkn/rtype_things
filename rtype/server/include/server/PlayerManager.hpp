#pragma once

#include <unordered_map>
#include <string>
#include <chrono>
#include <vector>
#include <optional>
#include <asio.hpp>

struct PlayerInfo {
    uint32_t id;
    asio::ip::udp::endpoint endpoint;
    std::chrono::steady_clock::time_point lastSeen;
    std::string nickname; // ✅ AJOUTÉ
};

class PlayerManager {
public:
    uint32_t addPlayer(const asio::ip::udp::endpoint& ep);
    bool hasPlayer(const asio::ip::udp::endpoint& ep) const;
    void updatePlayerActivity(const asio::ip::udp::endpoint& ep);
    void removePlayer(const asio::ip::udp::endpoint& ep);
    void removePlayerById(uint32_t id);
    std::vector<uint32_t> removeInactivePlayers(std::chrono::seconds timeout);
    std::optional<uint32_t> getPlayerIdByEndpoint(const asio::ip::udp::endpoint& ep) const;
    std::optional<asio::ip::udp::endpoint> getEndpointById(uint32_t id) const;
    std::vector<asio::ip::udp::endpoint> getAllEndpoints() const;

    // ✅ NOUVELLES MÉTHODES pour les pseudos
    bool isNicknameAvailable(const std::string& nickname) const;
    bool setNickname(uint32_t playerId, const std::string& nickname);
    std::string getNickname(uint32_t playerId) const;
    std::string validateAndSanitizeNickname(const std::string& input) const;
    std::optional<uint32_t> getIdByNickname(const std::string& nickname)const;

private:
    std::unordered_map<std::string, PlayerInfo> players_;
    uint32_t nextId_ = 1;
    std::string makeKey(const asio::ip::udp::endpoint& ep) const;
    mutable std::mutex m_mutex; 
};