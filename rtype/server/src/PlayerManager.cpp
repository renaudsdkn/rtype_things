#include "../include/server/PlayerManager.hpp"
#include <iostream>
#include <algorithm>
#include <cctype>

uint32_t PlayerManager::addPlayer(const asio::ip::udp::endpoint& ep) {
    std::string key = makeKey(ep);

    if (players_.count(key))
        return players_[key].id;

    uint32_t newId = nextId_++;
    players_[key] = PlayerInfo{newId, ep, std::chrono::steady_clock::now(), ""};
    std::cout << "[SERVER] Assigned ID " << newId << " to player " << key << "\n";
    return newId;
}

bool PlayerManager::hasPlayer(const asio::ip::udp::endpoint& ep) const {
    return players_.count(makeKey(ep)) > 0;
}

void PlayerManager::updatePlayerActivity(const asio::ip::udp::endpoint& ep) {
    auto key = makeKey(ep);
    if (players_.count(key))
        players_.at(key).lastSeen = std::chrono::steady_clock::now();
}

void PlayerManager::removePlayer(const asio::ip::udp::endpoint& ep) {
    players_.erase(makeKey(ep));
}

void PlayerManager::removePlayerById(uint32_t id) {
    for (auto it = players_.begin(); it != players_.end();) {
        if (it->second.id == id) {
            std::cout << "[SERVER] Player " << id << " removed by ID\n";
            it = players_.erase(it);
        } else {
            ++it;
        }
    }
}

std::vector<uint32_t> PlayerManager::removeInactivePlayers(std::chrono::seconds timeout) {
    auto now = std::chrono::steady_clock::now();
    std::vector<uint32_t> removedPlayerIDs;
    bool removed = false;

    for (auto it = players_.begin(); it != players_.end(); ) {
        if (std::chrono::duration_cast<std::chrono::seconds>(now - it->second.lastSeen) > timeout) {
            std::cout << "[THREAD JEU] Joueur " << it->second.id << " déconnecté (timeout): "
                      << it->second.endpoint.address().to_string() << std::endl;

            removedPlayerIDs.push_back(it->second.id);
            it = players_.erase(it);
            removed = true;
        } else {
            ++it;
        }
    }
    if (removed)
        std::cout << "[THREAD JEU] PlayerManager cleanup done\n";
        
    return removedPlayerIDs;
}

std::optional<uint32_t> PlayerManager::getPlayerIdByEndpoint(const asio::ip::udp::endpoint& ep) const {
    std::string key = makeKey(ep);
    if (players_.count(key))
        return players_.at(key).id;
    return std::nullopt;
}

std::optional<asio::ip::udp::endpoint> PlayerManager::getEndpointById(uint32_t id) const
{
    for (const auto& pair : players_) {
        if (pair.second.id == id) {
            return pair.second.endpoint;
        }
    }
    return std::nullopt;
}
 
std::string PlayerManager::makeKey(const asio::ip::udp::endpoint& ep) const {
    return ep.address().to_string() + ":" + std::to_string(ep.port());
}

std::vector<asio::ip::udp::endpoint> PlayerManager::getAllEndpoints() const {
    std::vector<asio::ip::udp::endpoint> endpoints;
    endpoints.reserve(players_.size());
    for (const auto& [_, info] : players_) {
        endpoints.push_back(info.endpoint);
    }
    return endpoints;
}

// ✅ NOUVELLES FONCTIONS pour les pseudos

std::string PlayerManager::validateAndSanitizeNickname(const std::string& input) const {
    std::string sanitized;
    
    // Validation permissive : garde tous les caractères imprimables ASCII
    for (char c : input) {
        if (c >= 32 && c <= 126) {  // Caractères imprimables ASCII
            sanitized += c;
        }
    }
    
    // Trim espaces début/fin
    size_t start = sanitized.find_first_not_of(" \t\n\r");
    size_t end = sanitized.find_last_not_of(" \t\n\r");
    
    if (start == std::string::npos) {
        return "";  // Que des espaces
    }
    
    sanitized = sanitized.substr(start, end - start + 1);
    
    // Limiter à 20 caractères
    if (sanitized.length() > 20) {
        sanitized = sanitized.substr(0, 20);
    }
    
    return sanitized;
}

bool PlayerManager::isNicknameAvailable(const std::string& nickname) const {
    // Comparaison insensible à la casse
    std::string lowerNick = nickname;
    std::transform(lowerNick.begin(), lowerNick.end(), lowerNick.begin(), ::tolower);
    
    for (const auto& [key, info] : players_) {
        if (info.nickname.empty()) continue;  // Ignore les joueurs sans pseudo
        
        std::string existingLower = info.nickname;
        std::transform(existingLower.begin(), existingLower.end(), existingLower.begin(), ::tolower);
        
        if (existingLower == lowerNick) {
            return false;  // Déjà pris
        }
    }
    
    return true;
}

bool PlayerManager::setNickname(uint32_t playerId, const std::string& nickname) {
    for (auto& [key, info] : players_) {
        if (info.id == playerId) {
            info.nickname = nickname;
            std::cout << "[PlayerManager] Joueur " << playerId << " → Pseudo: '" << nickname << "'" << std::endl;
            return true;
        }
    }
    return false;  // Joueur inexistant
}

std::string PlayerManager::getNickname(uint32_t playerId) const {
    for (const auto& [key, info] : players_) {
        if (info.id == playerId) {
            return info.nickname.empty() ? ("Joueur" + std::to_string(playerId)) : info.nickname;
        }
    }
    return "Joueur" + std::to_string(playerId);  // Fallback
}

std::optional<uint32_t> PlayerManager::getIdByNickname(const std::string& nickname) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // ✅ CORRIGÉ : On cherche dans les PlayerInfo, pas dans les clés
    for (const auto& [key, info] : players_) {
        // ✅ Compare avec le VRAI pseudo stocké dans PlayerInfo
        if (info.nickname == nickname) {
            return info.id;
        }
    }
    
    std::cerr << "[PlayerManager] ⚠️ Pseudo '" << nickname 
              << "' introuvable dans la base de joueurs" << std::endl;
    
    return std::nullopt;
}