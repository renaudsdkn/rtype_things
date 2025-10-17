#include "../include/protocol/protocol_data.hpp"
#include "../include/protocol/serializer.hpp"
using namespace Protocol;

// ---------------- SPAWN_ENTITY ----------------
SpawnEntityMessage::SpawnEntityMessage(const ProtocolData::SpawnEntity &data)
    : data_(data) {}

ProtocolData::MessageType SpawnEntityMessage::getType() const
{
    return ProtocolData::MessageType::SPAWN_ENTITY;
}

std::vector<uint8_t> SpawnEntityMessage::serialize() const
{
    ProtocolData::PacketHeader header{
        static_cast<uint16_t>(sizeof(header) + sizeof(data_)),
        static_cast<uint8_t>(ProtocolData::MessageType::SPAWN_ENTITY)
    };
    header.size = htons(header.size);

    ProtocolData::SpawnEntity tmp = data_;
    tmp.entity.id = htonl(tmp.entity.id);

    std::vector<uint8_t> buffer(sizeof(header) + sizeof(tmp));
    std::memcpy(buffer.data(), &header, sizeof(header));
    std::memcpy(buffer.data() + sizeof(header), &tmp, sizeof(tmp));
    return buffer;
}

size_t SpawnEntityMessage::size() const { return sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::SpawnEntity); }
const ProtocolData::SpawnEntity &SpawnEntityMessage::getData() const { return data_; }


// ---------------- MOVE_ENTITY ----------------
MoveEntityMessage::MoveEntityMessage(const ProtocolData::MoveEntity &data)
    : data_(data) {}

ProtocolData::MessageType MoveEntityMessage::getType() const
{
    return ProtocolData::MessageType::MOVE_ENTITY;
}

std::vector<uint8_t> MoveEntityMessage::serialize() const
{
    ProtocolData::PacketHeader header{
        static_cast<uint16_t>(sizeof(header) + sizeof(data_)),
        static_cast<uint8_t>(ProtocolData::MessageType::MOVE_ENTITY)
    };
    header.size = htons(header.size);

    ProtocolData::MoveEntity tmp = data_;
    tmp.id = htonl(tmp.id);

    std::vector<uint8_t> buffer(sizeof(header) + sizeof(tmp));
    std::memcpy(buffer.data(), &header, sizeof(header));
    std::memcpy(buffer.data() + sizeof(header), &tmp, sizeof(tmp));
    return buffer;
}

size_t MoveEntityMessage::size() const { return sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::MoveEntity); }
const ProtocolData::MoveEntity &MoveEntityMessage::getData() const { return data_; }


// ---------------- DESTROY_ENTITY ----------------
DestroyEntityMessage::DestroyEntityMessage(const ProtocolData::DestroyEntity &data)
    : data_(data) {}

ProtocolData::MessageType DestroyEntityMessage::getType() const
{
    return ProtocolData::MessageType::DESTROY_ENTITY;
}

std::vector<uint8_t> DestroyEntityMessage::serialize() const
{
    ProtocolData::PacketHeader header{
        static_cast<uint16_t>(sizeof(header) + sizeof(data_)),
        static_cast<uint8_t>(ProtocolData::MessageType::DESTROY_ENTITY)
    };
    header.size = htons(header.size);

    ProtocolData::DestroyEntity tmp = data_;
    tmp.id = htonl(tmp.id);

    std::vector<uint8_t> buffer(sizeof(header) + sizeof(tmp));
    std::memcpy(buffer.data(), &header, sizeof(header));
    std::memcpy(buffer.data() + sizeof(header), &tmp, sizeof(tmp));
    return buffer;
}

size_t DestroyEntityMessage::size() const { return sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::DestroyEntity); }
const ProtocolData::DestroyEntity &DestroyEntityMessage::getData() const { return data_; }


// ---------------- PLAYER_EVENT ----------------
PlayerEventMessage::PlayerEventMessage(const ProtocolData::PlayerEvent &data)
    : data_(data) {}

ProtocolData::MessageType PlayerEventMessage::getType() const
{
    return ProtocolData::MessageType::PLAYER_EVENT;
}

std::vector<uint8_t> PlayerEventMessage::serialize() const
{
    ProtocolData::PacketHeader header{
        static_cast<uint16_t>(sizeof(header) + sizeof(data_)),
        static_cast<uint8_t>(ProtocolData::MessageType::PLAYER_EVENT)
    };
    header.size = htons(header.size);

    ProtocolData::PlayerEvent tmp = data_;
    tmp.playerId = htonl(tmp.playerId);

    std::vector<uint8_t> buffer(sizeof(header) + sizeof(tmp));
    std::memcpy(buffer.data(), &header, sizeof(header));
    std::memcpy(buffer.data() + sizeof(header), &tmp, sizeof(tmp));
    return buffer;
}

size_t PlayerEventMessage::size() const { return sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::PlayerEvent); }
const ProtocolData::PlayerEvent &PlayerEventMessage::getData() const { return data_; }


// ---------------- PING ----------------
ProtocolData::MessageType PingMessage::getType() const
{
    return ProtocolData::MessageType::PING;
}

std::vector<uint8_t> PingMessage::serialize() const
{
    ProtocolData::PacketHeader header{
        htons(sizeof(ProtocolData::PacketHeader)),
        static_cast<uint8_t>(ProtocolData::MessageType::PING)
    };
    std::vector<uint8_t> buffer(sizeof(header));
    std::memcpy(buffer.data(), &header, sizeof(header));
    return buffer;
}

size_t PingMessage::size() const { return sizeof(ProtocolData::PacketHeader); }


// ---------------- PING_RESPONSE ----------------
ProtocolData::MessageType PingResponseMessage::getType() const
{
    return ProtocolData::MessageType::PING_RESPONSE;
}

std::vector<uint8_t> PingResponseMessage::serialize() const
{
    ProtocolData::PacketHeader header{
        htons(sizeof(ProtocolData::PacketHeader)),
        static_cast<uint8_t>(ProtocolData::MessageType::PING_RESPONSE)
    };
    std::vector<uint8_t> buffer(sizeof(header));
    std::memcpy(buffer.data(), &header, sizeof(header));
    return buffer;
}

size_t PingResponseMessage::size() const { return sizeof(ProtocolData::PacketHeader); }


// ---------------- ERROR ----------------
ProtocolData::MessageType ErrorMessage::getType() const
{
    return ProtocolData::MessageType::ERROR;
}

std::vector<uint8_t> ErrorMessage::serialize() const
{
    ProtocolData::PacketHeader header{
        htons(sizeof(ProtocolData::PacketHeader)),
        static_cast<uint8_t>(ProtocolData::MessageType::ERROR)
    };
    std::vector<uint8_t> buffer(sizeof(header));
    std::memcpy(buffer.data(), &header, sizeof(header));
    return buffer;
}

size_t ErrorMessage::size() const { return sizeof(ProtocolData::PacketHeader); }
