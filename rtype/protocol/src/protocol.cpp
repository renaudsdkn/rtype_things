#include "../include/protocol/protocol_data.hpp"
#include "../include/protocol/serializer.hpp"
using namespace Protocol;

// ---------------- CONNECT ----------------
// ✅ NOUVEAU : Constructeur avec pseudo
ConnectMessage::ConnectMessage(const std::string& nickname) {
    std::memset(&m_data, 0, sizeof(m_data));
    strncpy(m_data.nickname, nickname.c_str(), 20);
    m_data.nickname[20] = '\0';
}

ProtocolData::MessageType ConnectMessage::getType() const
{
    return ProtocolData::MessageType::CONNECT;
}

// ✅ MODIFIÉ : Sérialise le pseudo
std::vector<uint8_t> ConnectMessage::serialize() const
{
    size_t totalSize = sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::ConnectRequest);
    ProtocolData::PacketHeader header{
        htons(totalSize),
        static_cast<uint8_t>(ProtocolData::MessageType::CONNECT)
    };
    
    std::vector<uint8_t> buffer(totalSize);
    std::memcpy(buffer.data(), &header, sizeof(header));
    std::memcpy(buffer.data() + sizeof(header), &m_data, sizeof(m_data));
    return buffer;
}

size_t ConnectMessage::size() const
{
    return sizeof(ProtocolData::PacketHeader) + sizeof(m_data);  // Plus sûr
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

size_t DisconnectMessage::size() const { return sizeof(ProtocolData::PacketHeader); }

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

// ✅ NOUVEAU : Constructeur pour ACCEPTATION
WelcomeMessage::WelcomeMessage(uint32_t playerId, const std::string& confirmedNickname) {
    std::memset(&data_, 0, sizeof(data_));
    data_.playerId = playerId;
    data_.accepted = 1;
    strncpy(data_.confirmedNickname, confirmedNickname.c_str(), 20);
    data_.confirmedNickname[20] = '\0';
    data_.reason[0] = '\0';
}

// ✅ NOUVEAU : Constructeur pour REFUS
WelcomeMessage::WelcomeMessage(const std::string& reason) {
    std::memset(&data_, 0, sizeof(data_));
    data_.playerId = 0;
    data_.accepted = 0;
    strncpy(data_.reason, reason.c_str(), 63);
    data_.reason[63] = '\0';
    data_.confirmedNickname[0] = '\0';
}

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

// --- SNAPSHOT ---
SnapshotMessage::SnapshotMessage(const ProtocolData::Snapshot &snapshot)
    : data_(snapshot) {}

ProtocolData::MessageType SnapshotMessage::getType() const
{
    return ProtocolData::MessageType::SNAPSHOT;
}

std::vector<uint8_t> SnapshotMessage::serialize() const
{
    uint32_t numEntities = data_.entities.size();
    uint16_t totalSize = sizeof(ProtocolData::PacketHeader) + sizeof(uint32_t) 
                         + numEntities * sizeof(ProtocolData::entity_state);

    ProtocolData::PacketHeader header;
    header.size = htons(totalSize);
    header.type = static_cast<uint8_t>(ProtocolData::MessageType::SNAPSHOT);

    std::vector<uint8_t> buffer(totalSize);
    uint8_t *ptr = buffer.data();

    std::memcpy(ptr, &header, sizeof(header));
    ptr += sizeof(header);

    uint32_t netNumEntities = htonl(numEntities);
    std::memcpy(ptr, &netNumEntities, sizeof(netNumEntities));
    ptr += sizeof(netNumEntities);

    for (const auto &entity : data_.entities)
    {
        uint32_t netId = htonl(entity.id);
        std::memcpy(ptr, &netId, sizeof(netId));
        ptr += sizeof(netId);

        std::memcpy(ptr, &entity.type, sizeof(entity.type));
        ptr += sizeof(entity.type);

        std::memcpy(ptr, &entity.x, sizeof(entity.x));
        ptr += sizeof(entity.x);
        std::memcpy(ptr, &entity.y, sizeof(entity.y));
        ptr += sizeof(entity.y);
        std::memcpy(ptr, &entity.vx, sizeof(entity.vx));
        ptr += sizeof(entity.vx);
        std::memcpy(ptr, &entity.vy, sizeof(entity.vy));
        ptr += sizeof(entity.vy);
        std::memcpy(ptr, &entity.damage, sizeof(entity.damage));
        ptr += sizeof(entity.damage);
        std::memcpy(ptr, &entity.xp, sizeof(entity.xp));
        ptr += sizeof(entity.xp);
        std::memcpy(ptr, &entity.level, sizeof(entity.level));
        ptr += sizeof(entity.level);
        std::memcpy(ptr, &entity.health, sizeof(entity.health));
        ptr += sizeof(entity.health);
    }
    return buffer;
}

size_t SnapshotMessage::size() const
{
    return sizeof(ProtocolData::PacketHeader) + sizeof(uint32_t) + data_.entities.size() * sizeof(ProtocolData::entity_state);
}
const ProtocolData::Snapshot &SnapshotMessage::getData() const { return data_; }

//------ LEAVEROOM ---
LeaveRoomRequestMessage::LeaveRoomRequestMessage(const ProtocolData::LeaveRoomRequest &data)
    : data_(data) {}
ProtocolData::MessageType LeaveRoomRequestMessage ::getType() const
{
    return ProtocolData::MessageType::LEAVE_ROOM_REQUEST;
}
std::vector<uint8_t> LeaveRoomRequestMessage ::serialize() const
{

    ProtocolData::PacketHeader header{
        static_cast<uint16_t>(sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::PlayerInput)),
        static_cast<uint8_t>(ProtocolData::MessageType::LEAVE_ROOM_REQUEST)};
    header.size = htons(header.size);
    
    ProtocolData::LeaveRoomRequest request = data_;
    request.roomId = htonl(request.roomId);

    std::vector<uint8_t> buffer(sizeof(header) + sizeof(request));
    std::memcpy(buffer.data(), &header, sizeof(header));
    std::memcpy(buffer.data() + sizeof(header), &request, sizeof(request));
    return buffer;
}

size_t LeaveRoomRequestMessage::size() const
{
    return sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::LeaveRoomRequest);
};