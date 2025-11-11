/*
** EPITECH PROJECT, 2025
** G-CPP-500
** File description:
** serializer.hpp
*/
#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <cstring>
#include <stdexcept>
#include <arpa/inet.h> // pour htons, ntohs, htonl, ntohl
#include "protocol_data.hpp"

namespace Protocol
{
    // --- Interface commune ---
    class IMessage
    {
    public:
        virtual ~IMessage() = default;
        virtual ProtocolData::MessageType getType() const = 0;
        virtual std::vector<uint8_t> serialize() const = 0;
        virtual size_t size() const = 0;
    };

    // --- CONNECT ---
    class ConnectMessage : public IMessage
    {
    private:
        ProtocolData::ConnectRequest m_data{}; // ✅ AJOUTER {} pour initialiser à zéro

    public:
        ConnectMessage(const std::string &nickname);
        ConnectMessage() = default;
        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;

        const ProtocolData::ConnectRequest &getData() const { return m_data; }
    };
    // --- INPUT ---
    class PlayerInputMessage : public IMessage
    {
    private:
        ProtocolData::PlayerInput data_;

    public:
        explicit PlayerInputMessage(const ProtocolData::PlayerInput &input);
        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;

        const ProtocolData::PlayerInput &getData() const { return data_; }
    };

    // --- WELCOME ---
    class WelcomeMessage : public IMessage
    {
    private:
        ProtocolData::Welcome data_;

    public:
        explicit WelcomeMessage(const ProtocolData::Welcome &welcome);

        // ✅ AJOUTÉ : Constructeurs helper
        WelcomeMessage(uint32_t playerId, const std::string &confirmedNickname);
        explicit WelcomeMessage(const std::string &reason); // Pour refus

        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;
        const ProtocolData::Welcome &getData() const;
    };

    // --- SNAPSHOT ---
    class SnapshotMessage : public IMessage
    {
    private:
        ProtocolData::Snapshot data_;

    public:
        explicit SnapshotMessage(const ProtocolData::Snapshot &snapshot);
        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;
        const ProtocolData::Snapshot &getData() const;
    };

    // --- PLAYER_EVENT ---
    class PlayerEventMessage : public IMessage
    {
    private:
        ProtocolData::PlayerEvent data_;

    public:
        explicit PlayerEventMessage(const ProtocolData::PlayerEvent &data);
        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;
        const ProtocolData::PlayerEvent &getData() const;
    };

    // --- ERROR ---
    class ErrorMessage : public IMessage
    {
    public:
        ErrorMessage() = default;
        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;
    };

    //---DISCONNECT---
    class DisconnectMessage : public IMessage
    {
    public:
        DisconnectMessage() = default;

        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;
    };

    // --- NOUVEAUX MESSAGES POUR LE LOBBY ---

    // --- LIST_ROOMS_REQUEST --- (Client -> Serveur)
    class ListRoomsRequestMessage : public IMessage
    {
    public:
        ListRoomsRequestMessage() = default;
        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;
    };

    // --- ROOM_LIST_RESPONSE --- (Serveur -> Client)
    class RoomListResponseMessage : public IMessage
    {
    private:
        ProtocolData::RoomList data_;

    public:
        explicit RoomListResponseMessage(const ProtocolData::RoomList &data);
        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;
        const ProtocolData::RoomList &getData() const { return data_; }
    };

    // --- CREATE_ROOM_REQUEST --- (Client -> Serveur)
  class CreateRoomRequestMessage : public IMessage
{
private:
    ProtocolData::CreateRoomRequest data_;

public:
    explicit CreateRoomRequestMessage(const ProtocolData::RoomConfig& config);
    CreateRoomRequestMessage() = default;
    ProtocolData::MessageType getType() const override;
    std::vector<uint8_t> serialize() const override;
    size_t size() const override;
    
    // ✅ AJOUTER CE GETTER
    const ProtocolData::CreateRoomRequest& getData() const { return data_; }
};

    // --- CREATE_ROOM_RESPONSE --- (Serveur -> Client)
    class CreateRoomResponseMessage : public IMessage
    {
    private:
        ProtocolData::RoomResponse data_;

    public:
        explicit CreateRoomResponseMessage(const ProtocolData::RoomResponse &data);
        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;
        const ProtocolData::RoomResponse &getData() const { return data_; }
    };

    // --- JOIN_ROOM_REQUEST --- (Client -> Serveur)
    class JoinRoomRequestMessage : public IMessage
    {
    private:
        ProtocolData::JoinRoomRequest data_;

    public:
        explicit JoinRoomRequestMessage(const ProtocolData::JoinRoomRequest &data);
        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;
        const ProtocolData::JoinRoomRequest &getData() const { return data_; }
    };

    // --- JOIN_ROOM_RESPONSE --- (Serveur -> Client)
    class JoinRoomResponseMessage : public IMessage
    {
    private:
        ProtocolData::RoomResponse data_;

    public:
        explicit JoinRoomResponseMessage(const ProtocolData::RoomResponse &data);
        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;
        const ProtocolData::RoomResponse &getData() const { return data_; }
    };

    // --- PLAYER_JOINED_ROOM --- (Serveur -> Clients de la room)
    class PlayerJoinedRoomMessage : public IMessage
    {
    private:
        ProtocolData::PlayerRoomNotification data_;

    public:
        explicit PlayerJoinedRoomMessage(const ProtocolData::PlayerRoomNotification &data);
        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;
        const ProtocolData::PlayerRoomNotification &getData() const { return data_; }
    };

    // --- PLAYER_LEFT_ROOM --- (Serveur -> Clients de la room)
    class PlayerLeftRoomMessage : public IMessage
    {
    private:
        ProtocolData::PlayerRoomNotification data_;

    public:
        explicit PlayerLeftRoomMessage(const ProtocolData::PlayerRoomNotification &data);
        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;
        const ProtocolData::PlayerRoomNotification &getData() const { return data_; }
    };

    class LeaveRoomRequestMessage : public IMessage
    {
    private:
        ProtocolData::LeaveRoomRequest data_;

    public:
        explicit LeaveRoomRequestMessage(const ProtocolData::LeaveRoomRequest &data);
        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;
        const ProtocolData::LeaveRoomRequest &getData() const { return data_; }
    };

    // --- GAME_STARTING --- (Serveur -> Clients de la room)
    class GameStartingMessage : public IMessage
    {
    public:
        GameStartingMessage() = default;
        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;
    };


class DeltaSnapshotMessage : public IMessage {
public:
    explicit DeltaSnapshotMessage(const ProtocolData::DeltaSnapshot& delta);
    ProtocolData::MessageType getType() const;
    std::vector<uint8_t> serialize() const;
    const ProtocolData::DeltaSnapshot& getData() const { return m_data; }
    size_t size() const override;

private:
    ProtocolData::DeltaSnapshot m_data;
};
// namespace Protocol
    // --- Factory ---
    class MessageFactory
    {
    public:
        static std::unique_ptr<IMessage> deserialize(const std::vector<uint8_t> &buffer);
    };
}