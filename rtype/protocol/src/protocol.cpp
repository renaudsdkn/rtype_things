#include "../include/protocol/protocol_data.hpp"
#include "../include/protocol/serializer.hpp"
using namespace Protocol;

// ---------------- CONNECT ----------------
ProtocolData::MessageType ConnectMessage::getType() const
{
    return ProtocolData::MessageType::CONNECT;
}

std::vector<uint8_t> ConnectMessage::serialize() const
{
    ProtocolData::PacketHeader header{
        htons(sizeof(ProtocolData::PacketHeader)),
        static_cast<uint8_t>(ProtocolData::MessageType::CONNECT)};
    std::vector<uint8_t> buffer(sizeof(header));
    std::memcpy(buffer.data(), &header, sizeof(header));
    return buffer;
}

size_t ConnectMessage::size() const
{
    return sizeof(ProtocolData::PacketHeader);
}
//-------------------DISCONNECT-----------------
ProtocolData::MessageType DisconnectMessage::getType() const
{
    return ProtocolData::MessageType::DISCONNECT;
}

std::vector<uint8_t> DisconnectMessage::serialize() const
{
    ProtocolData::PacketHeader header{
        htons(sizeof(ProtocolData::PacketHeader)),
        static_cast<uint8_t>(ProtocolData::MessageType::DISCONNECT)};

    std::vector<uint8_t> data(sizeof(header));
    std::memcpy(data.data(), &header, sizeof(header));
    return data;
}

size_t DisconnectMessage::size() const{ return sizeof(ProtocolData::PacketHeader); }


// ---------------- INPUT ----------------
PlayerInputMessage::PlayerInputMessage(const ProtocolData::PlayerInput &input)
    : data_(input) {}

ProtocolData::MessageType PlayerInputMessage::getType() const
{
    return ProtocolData::MessageType::INPUT;
}

std::vector<uint8_t> PlayerInputMessage::serialize() const
{
    ProtocolData::PacketHeader header{
        static_cast<uint16_t>(sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::PlayerInput)),
        static_cast<uint8_t>(ProtocolData::MessageType::INPUT)};
    header.size = htons(header.size);

    ProtocolData::PlayerInput input = data_;
    input.playerId = htonl(input.playerId);

    std::vector<uint8_t> buffer(sizeof(header) + sizeof(input));
    std::memcpy(buffer.data(), &header, sizeof(header));
    std::memcpy(buffer.data() + sizeof(header), &input, sizeof(input));
    return buffer;
}

size_t PlayerInputMessage::size() const
{
    return sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::PlayerInput);
}

// ---------------- WELCOME ----------------
WelcomeMessage::WelcomeMessage(const ProtocolData::Welcome &welcome)
    : data_(welcome) {}

ProtocolData::MessageType WelcomeMessage::getType() const
{
    return ProtocolData::MessageType::WELCOME;
}

std::vector<uint8_t> WelcomeMessage::serialize() const
{
    ProtocolData::PacketHeader header{
        static_cast<uint16_t>(sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::Welcome)),
        static_cast<uint8_t>(ProtocolData::MessageType::WELCOME)};
    header.size = htons(header.size);

    ProtocolData::Welcome data = data_;
    data.playerId = htonl(data.playerId);

    std::vector<uint8_t> buffer(sizeof(header) + sizeof(data));
    std::memcpy(buffer.data(), &header, sizeof(header));
    std::memcpy(buffer.data() + sizeof(header), &data, sizeof(data));
    return buffer;
}

size_t WelcomeMessage::size() const
{
    return sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::Welcome);
}

const ProtocolData::Welcome &WelcomeMessage::getData() const
{
    return data_;
}

// ---------------- SNAPSHOT ----------------
SnapshotMessage::SnapshotMessage(const ProtocolData::Snapshot &snapshot)
    : data_(snapshot) {}

ProtocolData::MessageType SnapshotMessage::getType() const
{
    return ProtocolData::MessageType::SNAPSHOT;
}

std::vector<uint8_t> SnapshotMessage::serialize() const
{
    ProtocolData::PacketHeader header{
        static_cast<uint16_t>(sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::Snapshot)),
        static_cast<uint8_t>(ProtocolData::MessageType::SNAPSHOT)};
    header.size = htons(header.size);

    std::vector<uint8_t> buffer(sizeof(header) + sizeof(data_));
    std::memcpy(buffer.data(), &header, sizeof(header));
    std::memcpy(buffer.data() + sizeof(header), &data_, sizeof(data_));
    return buffer;
}

size_t SnapshotMessage::size() const
{
    return sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::Snapshot);
}

const ProtocolData::Snapshot &SnapshotMessage::getData() const
{
    return data_;
}
