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

namespace ProtocolData
{
    // types de messages
    enum class MessageType : uint8_t
    {
        CONNECT = 0x01,
        DISCONNECT = 0x02,
        INPUT = 0x03,
        WELCOME = 0x04,
        SNAPSHOT = 0x05,
        PLAYER_EVENT = 0x06,
        LIST_ROOMS_REQUEST = 0x07,   // Client demande la liste des rooms
        ROOM_LIST_RESPONSE = 0x08,   // Serveur envoie la liste
        CREATE_ROOM_REQUEST = 0x09,  // Client demande à créer une room
        CREATE_ROOM_RESPONSE = 0x10, // Serveur confirme la création (ou échec)
        JOIN_ROOM_REQUEST = 0x11,    // Client demande à rejoindre une room
        JOIN_ROOM_RESPONSE = 0x12,   // Serveur confirme l'entrée (ou échec)
        PLAYER_JOINED_ROOM = 0x13,   // Notification: un joueur a rejoint *votre* room
        PLAYER_LEFT_ROOM = 0x14,     // Notification: un joueur a quitté *votre* room
        GAME_STARTING = 0x15,
        LEAVE_ROOM_REQUEST = 0x16,
        ERROR = 0x17,
        DELTA_SNAPSHOT = 0x20
    };
    struct RoomInfo
    {
        uint32_t roomId;
        uint8_t playerCount;
        uint8_t maxPlayers; // Ex: 4
        uint8_t roomState;  // Ex: 0=Waiting, 1=Playing
        // Tu pourrais ajouter un nom de room ici (char name[32];)
    } __attribute__((packed));
    // Message envoyé par le serveur contenant la liste
    // (La structure Snapshot est un bon exemple de vecteur)
    struct RoomList
    {
        std::vector<RoomInfo> rooms;
    };

    struct JoinRoomRequest
    {
        uint32_t roomId;
    } __attribute__((packed));

    // Réponse du serveur à CREATE ou JOIN

    struct RoomResponse
    {
        uint8_t success; // 1 = OK, 0 = Erreur (pleine, existe pas...)
        uint32_t roomId; // ID de la room créée/rejointe si succès
    } __attribute__((packed));

    // Notification de join/left (envoyée aux autres joueurs de la room)

    struct PlayerRoomNotification
    {
        uint32_t roomId;   // Dans quelle room ?
        uint32_t playerId; // Qui a rejoint/quitté ?
        char nickname[21];
    } __attribute__((packed));

    enum class PlayerEventType : uint8_t
    {
        GAME_OVER = 1,
        RESPAWN = 2,
        POWERUP = 3,
        SHOOT = 4
    };

    // on va encore explquer ca? bon, inputs du joueur
    struct PlayerInput
    {
        uint32_t playerId;
        uint8_t up : 1;
        uint8_t down : 1;
        uint8_t right : 1;
        uint8_t left : 1;
        uint8_t shoot : 1;
    } __attribute__((packed));
    // tous les packets ont cette entete pour qu'on sache le type de message
    struct PacketHeader
    {
        uint16_t size; // taille totale du message
        uint8_t type;  // type du message
    } __attribute__((packed));
    // etat individuel de chaque entite
    struct entity_state
    {
        uint32_t id, score;
        uint8_t type; // type d'entite : player, missile. enemy
        float x, y, vx, vy;
        uint8_t damage, xp, level, health;
    } __attribute__((packed));

    // en gros, c'est la game state
    struct Snapshot
    {
        std::vector<entity_state> entities;
    };

    struct Welcome
    {
        uint32_t playerId; // identifiant attribué par le serveur
        uint8_t accepted;  // 1 = Accepté, 0 = Refusé
        char reason[64];   // Raison du refus si accepted = 0
        char confirmedNickname[21];
    } __attribute__((packed));

    struct SpawnEntity
    {
        entity_state entity;
    } __attribute__((packed));

    struct MoveEntity
    {
        uint32_t id;
        float x, y;
    } __attribute__((packed));

    struct DestroyEntity
    {
        uint32_t id;
    } __attribute__((packed));

    struct PlayerEvent
    {
        uint32_t playerId;
        PlayerEventType type;
    } __attribute__((packed));
    struct LeaveRoomRequest
    {
        uint32_t roomId;
    } __attribute__((packed));
    struct ConnectRequest
    {
        char nickname[21]; // 20 caractères + '\0'
        // Padding potentiel pour alignement
    } __attribute__((packed));

    struct RoomConfig
    {
        char roomName[32];            // Nom personnalisé
        uint8_t difficulty;           // 0=Easy, 1=Normal, 2=Hard
        uint8_t maxPlayers;           // 2-6 joueurs
        uint8_t enemySpeedMultiplier; // 50-150% (stocké comme 50-150)
        uint8_t spawnRateMultiplier;  // 50-200% (50=lent, 200=rapide)
        uint8_t friendlyFire;         // 0=Off, 1=On
        uint8_t powerUpsEnabled;      // 0=Off, 1=On
        uint8_t survivalMode;         // 0=Normal, 1=Infinite
    } __attribute__((packed));
    // APRÈS (avec config)
    struct CreateRoomRequest
    {
        RoomConfig config;
    } __attribute__((packed));
    // ✅ NOUVEAU : Type de changement d'entité
    enum class EntityChangeType : uint8_t
    {
        CREATED = 0,  // Nouvelle entité
        UPDATED = 1,  // Entité modifiée (position/rotation/etc.)
        DESTROYED = 2 // Entité détruite
    };

    struct EntityChange
{
    uint32_t entityId;           // ID de l'entité (network ID)
    EntityChangeType changeType; // CREATED/UPDATED/DESTROYED

    // ✅ CORRIGÉ : Utilise entity_state (pas EntityData)
    entity_state data; // Données complètes (seulement si CREATED/UPDATED)

    // Flags pour optimiser (optionnel)
    uint8_t changedFields; // Bitmask : 0x01=position, 0x02=rotation, 0x04=velocity, etc.
} __attribute__((packed));

// ✅ NOUVEAU : Snapshot différentiel
struct DeltaSnapshot
{
    uint32_t snapshotId;               // Numéro de séquence
    uint16_t changeCount;              // Nombre de changements
    std::vector<EntityChange> changes; // Liste des changements

    // Optionnel : timestamp pour interpolation
    uint32_t timestamp;
};
}
