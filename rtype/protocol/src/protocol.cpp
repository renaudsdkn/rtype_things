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

// --- SNAPSHOT ---
SnapshotMessage::SnapshotMessage(const ProtocolData::Snapshot &snapshot)
    : data_(snapshot) {}

ProtocolData::MessageType SnapshotMessage::getType() const {
    return ProtocolData::MessageType::SNAPSHOT;
}

// MODIFIÉ : Sérialisation manuelle
std::vector<uint8_t> SnapshotMessage::serialize() const {
    // Calculer la taille totale : header + nombre d'entités + (taille de chaque entité)
    uint32_t numEntities = data_.entities.size();
    uint16_t totalSize = sizeof(ProtocolData::PacketHeader) + sizeof(uint32_t) /* pour le count */
                         + numEntities * sizeof(ProtocolData::entity_state); // Utilise la nouvelle taille

    ProtocolData::PacketHeader header;
    header.size = htons(totalSize); // Convertir la taille totale
    header.type = static_cast<uint8_t>(ProtocolData::MessageType::SNAPSHOT);

    std::vector<uint8_t> buffer(totalSize);
    uint8_t* ptr = buffer.data();

    // 1. Écrire le header
    std::memcpy(ptr, &header, sizeof(header));
    ptr += sizeof(header);

    // 2. Écrire le nombre d'entités (converti)
    uint32_t netNumEntities = htonl(numEntities);
    std::memcpy(ptr, &netNumEntities, sizeof(netNumEntities));
    ptr += sizeof(netNumEntities);

    // 3. Écrire chaque entité champ par champ
    for (const auto& entity : data_.entities) {
        uint32_t netId = htonl(entity.id); // Convertir l'ID
        std::memcpy(ptr, &netId, sizeof(netId));
        ptr += sizeof(netId);

        std::memcpy(ptr, &entity.type, sizeof(entity.type)); // uint8_t n'a pas besoin de conversion
        ptr += sizeof(entity.type);

        // Copier les floats octet par octet (généralement sûr avec IEEE 754)
        std::memcpy(ptr, &entity.x, sizeof(entity.x));
        ptr += sizeof(entity.x);
        std::memcpy(ptr, &entity.y, sizeof(entity.y));
        ptr += sizeof(entity.y);
        // ✅ IMPORTANT : Si tu as ajouté vx, vy, damage, xp, level à entity_state,
        // IL FAUT AUSSI LES ÉCRIRE ICI !
        std::memcpy(ptr, &entity.vx, sizeof(entity.vx)); ptr += sizeof(entity.vx);
         std::memcpy(ptr, &entity.vy, sizeof(entity.vy)); ptr += sizeof(entity.vy);
         std::memcpy(ptr, &entity.damage, sizeof(entity.damage)); ptr += sizeof(entity.damage);
         std::memcpy(ptr, &entity.xp, sizeof(entity.xp)); ptr += sizeof(entity.xp);
         std::memcpy(ptr, &entity.level, sizeof(entity.level)); ptr += sizeof(entity.level);
    }
    return buffer;
}

size_t SnapshotMessage::size() const {
    // Retourne la taille calculée comme dans serialize()
     return sizeof(ProtocolData::PacketHeader) + sizeof(uint32_t)
           + data_.entities.size() * sizeof(ProtocolData::entity_state);
}
const ProtocolData::Snapshot &SnapshotMessage::getData() const { return data_; }