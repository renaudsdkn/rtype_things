/*
** EPITECH PROJECT, 2025
** G-CPP-500
** File description:
** room.hpp
*/

#ifndef ROOM_HPP
    #define ROOM_HPP

#include <asio.hpp>
#include <vector>
#include <chrono>
#include "../include/protocol/Factory.hpp"

constexpr int MAX_FIREBALLS = 50;
constexpr int MAX_PLAYERS_PER_ROOM = 4;

class Room {
public:
    // Structure pour un joueur
    struct Player {
        asio::ip::udp::endpoint endpoint;
        int id;
        float x;
        float y;
        int score;
        std::chrono::steady_clock::time_point last_seen;
    };

    // Structure pour une boule de feu
    struct Fireball {
        float x;
        float y;
        float speed;
        bool active;
    };

    // Constructeur
    explicit Room(int roomId);

    // Interface publique
    bool addPlayer(const asio::ip::udp::endpoint& endpoint, int playerId);
    void removePlayer(const asio::ip::udp::endpoint& endpoint);
    void update(const std::vector<uint8_t>& inputData, const asio::ip::udp::endpoint& endpoint, float deltaTime);
    void sendState(asio::ip::udp::socket& socket);
    void cleanupInactivePlayers(std::chrono::steady_clock::time_point now, std::chrono::seconds timeout);
    bool isEmpty() const;
    int getId() const;

private:
    int id_;
    std::vector<Player> players_;
    std::vector<Fireball> fireballs_;
};

#endif