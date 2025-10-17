#include "room.hpp"
#include <sstream>
#include <iostream>

Room::Room(int roomId) : id_(roomId) {
    fireballs_.resize(MAX_FIREBALLS, {0.0f, 0.0f, 600.0f, false});
}

bool Room::addPlayer(const asio::ip::udp::endpoint& endpoint, int playerId) {
    if (players_.size() >= MAX_PLAYERS_PER_ROOM) return false;
    players_.push_back({endpoint, playerId, 1920.0f / 4.0f, 1080.0f / 2.0f, 0, std::chrono::steady_clock::now()});
    std::cout << "[GAME] Player " << playerId << " joined room " << id_ << std::endl;
    return true;
}

void Room::removePlayer(const asio::ip::udp::endpoint& endpoint) {
    for (auto it = players_.begin(); it != players_.end(); ++it) {
        if (it->endpoint == endpoint) {
            std::cout << "[GAME] Player " << it->id << " left room " << id_ << std::endl;
            players_.erase(it);
            break;
        }
    }
}

bool Room::isEmpty() const {
    return players_.empty();
}

int Room::getId() const {
    return id_;
}