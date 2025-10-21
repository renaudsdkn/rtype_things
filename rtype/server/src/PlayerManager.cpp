#include "../include/server/PlayerManager.hpp"
#include <iostream>


uint32_t PlayerManager::addPlayer(const asio::ip::udp::endpoint& ep) {
    std::string key = makeKey(ep);

    if (players_.count(key))
        return players_[key].id;

    uint32_t newId = nextId_++;
    players_[key] = PlayerInfo{newId, ep, std::chrono::steady_clock::now()};
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
    std::vector<uint32_t> removedPlayerIDs; // ✅ Liste des IDs à retourner
    // bool removed = false; // Pas vraiment nécessaire si on regarde la taille du vecteur retourné

    for (auto it = players_.begin(); it != players_.end(); /* pas d'incrément ici */) {
        if (std::chrono::duration_cast<std::chrono::seconds>(now - it->second.lastSeen) > timeout) {
            std::cout << "[THREAD JEU] Joueur " << it->second.id << " déconnecté (timeout): "
                      << it->second.endpoint.address().to_string() << std::endl;

            removedPlayerIDs.push_back(it->second.id); // ✅ Ajoute l'ID AVANT de supprimer
            it = players_.erase(it); // Supprime l'entrée de la map
        } else {
            ++it; // Incrémente seulement si on n'a pas supprimé
        }
    }
    return removedPlayerIDs; // ✅ Retourne la liste des IDs
}
std::optional<uint32_t> PlayerManager::getPlayerIdByEndpoint(const asio::ip::udp::endpoint& ep) const {
    std::string key = makeKey(ep);
    if (players_.count(key))
        return players_.at(key).id;
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