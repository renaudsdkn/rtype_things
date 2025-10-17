/*
** EPITECH PROJECT, 2025
** G-CPP-500
** File description:
** protocol.hpp
*/

#pragma once

#include <iostream>
#include <cstdint>
#include <vector>
#include <memory>
#include <cstring> // memcpy
#include <stdexcept>

namespace ProtocolData {
      //types de messages
    enum class MessageType : uint8_t{
        CONNECT = 0x01,
        DISCONNECT = 0x02,
        INPUT = 0x03,
        WELCOME = 0x04,
        SNAPSHOT = 0x05,
        SPAWN_ENTITY = 0x06,
        MOVE_ENTITY = 0x07,
        DESTROY_ENTITY = 0x08,
        PLAYER_EVENT = 0x09,
        PING = 0x0A,    //pour verifier si le client est tjrs connecte
        PING_RESPONSE = 0x0B,    //reponse envoiyee suite au ping
        ERROR = 0x0C
    };

    enum class PlayerEventType : uint8_t {
        DEATH = 1,
        RESPAWN = 2,
        POWERUP = 3,
        SHOOT = 4
    };

    //on va encore explquer ca? bon, inputs du joueur
    struct PlayerInput {
        uint32_t playerId;
        uint8_t up : 1;
        uint8_t down : 1;
        uint8_t right : 1;
        uint8_t left : 1;
        uint8_t shoot : 1;
    }__attribute__((packed));
    //tous les packets ont cette entete pour qu'on sache le type de message
    struct PacketHeader {
        uint16_t size;  //taille totale du message
        uint8_t type; //type du message
    }__attribute__((packed));
    //etat individuel de chaque entite
    struct entity_state {
        uint32_t id;
        uint8_t type; //type d'entite : player, missile. enemy
    }__attribute__((packed));

    //en gros, c'est la game state
    struct Snapshot {
        std::vector<entity_state> entities;
    };

    struct Welcome {
        uint32_t playerId; // identifiant attribué par le serveur
    }__attribute__((packed));

    struct SpawnEntity {
        entity_state entity; 
    }__attribute__((packed));

    struct MoveEntity {
        uint32_t id;  
        float x, y;  
    }__attribute__((packed));

    struct DestroyEntity {
       uint32_t id;
    }__attribute__((packed));

    struct PlayerEvent {
        uint32_t playerId;
        PlayerEventType type;
    } __attribute__((packed));
}
