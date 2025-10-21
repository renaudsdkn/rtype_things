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
    public:
        ConnectMessage() = default;
        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;
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

        const ProtocolData::PlayerInput& getData() const { return data_; }
    };

    // --- WELCOME ---
    class WelcomeMessage : public IMessage
    {
    private:
        ProtocolData::Welcome data_;

    public:
        explicit WelcomeMessage(const ProtocolData::Welcome &welcome);
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
    // --- SPAWN_ENTITY ---
    class SpawnEntityMessage : public IMessage
    {
    private:
        ProtocolData::SpawnEntity data_;

    public:
        explicit SpawnEntityMessage(const ProtocolData::SpawnEntity &data);
        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;
        const ProtocolData::SpawnEntity &getData() const;
    };

    // --- MOVE_ENTITY ---
    class MoveEntityMessage : public IMessage
    {
    private:
        ProtocolData::MoveEntity data_;

    public:
        explicit MoveEntityMessage(const ProtocolData::MoveEntity &data);
        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;
        const ProtocolData::MoveEntity &getData() const;
    };

    // --- DESTROY_ENTITY ---
    class DestroyEntityMessage : public IMessage
    {
    private:
        ProtocolData::DestroyEntity data_;

    public:
        explicit DestroyEntityMessage(const ProtocolData::DestroyEntity &data);
        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;
        const ProtocolData::DestroyEntity &getData() const;
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

    // --- PING ---
    class PingMessage : public IMessage
    {
    public:
        PingMessage() = default;
        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;
    };

    // --- PING_RESPONSE ---
    class PingResponseMessage : public IMessage
    {
    public:
        PingResponseMessage() = default;
        ProtocolData::MessageType getType() const override;
        std::vector<uint8_t> serialize() const override;
        size_t size() const override;
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

    // --- Factory ---
    class MessageFactory
    {
    public:
        static std::unique_ptr<IMessage> deserialize(const std::vector<uint8_t> &buffer);
    };
}
