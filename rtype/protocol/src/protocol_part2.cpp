#include "../include/protocol/protocol_data.hpp"
#include "../include/protocol/serializer.hpp"
using namespace Protocol;

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
        static_cast<uint8_t>(ProtocolData::MessageType::PLAYER_EVENT)};
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

// ---------------- ERROR ----------------
ProtocolData::MessageType ErrorMessage::getType() const
{
    return ProtocolData::MessageType::ERROR;
}

std::vector<uint8_t> ErrorMessage::serialize() const
{
    ProtocolData::PacketHeader header{
        htons(sizeof(ProtocolData::PacketHeader)),
        static_cast<uint8_t>(ProtocolData::MessageType::ERROR)};
    std::vector<uint8_t> buffer(sizeof(header));
    std::memcpy(buffer.data(), &header, sizeof(header));
    return buffer;
}

size_t ErrorMessage::size() const { return sizeof(ProtocolData::PacketHeader); }

DeltaSnapshotMessage::DeltaSnapshotMessage(const ProtocolData::DeltaSnapshot &delta)
    : m_data(delta) {}

ProtocolData::MessageType DeltaSnapshotMessage::getType() const
{
    return ProtocolData::MessageType::DELTA_SNAPSHOT;
}

std::vector<uint8_t> DeltaSnapshotMessage::serialize() const
{
    std::vector<uint8_t> buffer;

    // ─────────────────────────────────────────────────────
    // 1. Calculer taille totale
    // ─────────────────────────────────────────────────────
    size_t totalSize = sizeof(ProtocolData::PacketHeader) + sizeof(uint32_t) // snapshotId
                       + sizeof(uint16_t)                                    // changeCount
                       + sizeof(uint32_t);                                   // timestamp

    // Ajouter taille de chaque EntityChange
    for (const auto &change : m_data.changes)
    {
        totalSize += sizeof(uint32_t);                       // entityId
        totalSize += sizeof(ProtocolData::EntityChangeType); // changeType
        totalSize += sizeof(uint8_t);                        // changedFields

        // Si CREATED ou UPDATED, inclure entity_state
        if (change.changeType != ProtocolData::EntityChangeType::DESTROYED)
        {
            totalSize += sizeof(ProtocolData::entity_state);
        }
    }

    buffer.resize(totalSize);
    size_t offset = 0;

    // ─────────────────────────────────────────────────────
    // 2. Écrire Header
    // ─────────────────────────────────────────────────────
    ProtocolData::PacketHeader header;
    header.size = htons(static_cast<uint16_t>(totalSize));
    header.type = static_cast<uint8_t>(ProtocolData::MessageType::DELTA_SNAPSHOT);

    std::memcpy(buffer.data() + offset, &header, sizeof(header));
    offset += sizeof(header);

    // ─────────────────────────────────────────────────────
    // 3. Écrire snapshotId (big endian)
    // ─────────────────────────────────────────────────────
    uint32_t snapshotIdNet = htonl(m_data.snapshotId);
    std::memcpy(buffer.data() + offset, &snapshotIdNet, sizeof(snapshotIdNet));
    offset += sizeof(snapshotIdNet);

    // ─────────────────────────────────────────────────────
    // 4. Écrire changeCount (big endian)
    // ─────────────────────────────────────────────────────
    uint16_t changeCountNet = htons(m_data.changeCount);
    std::memcpy(buffer.data() + offset, &changeCountNet, sizeof(changeCountNet));
    offset += sizeof(changeCountNet);

    // ─────────────────────────────────────────────────────
    // 5. Écrire timestamp (big endian)
    // ─────────────────────────────────────────────────────
    uint32_t timestampNet = htonl(m_data.timestamp);
    std::memcpy(buffer.data() + offset, &timestampNet, sizeof(timestampNet));
    offset += sizeof(timestampNet);

    // ─────────────────────────────────────────────────────
    // 6. Écrire chaque EntityChange
    // ─────────────────────────────────────────────────────
    for (const auto &change : m_data.changes)
    {
        // 6a. entityId (big endian)
        uint32_t entityIdNet = htonl(change.entityId);
        std::memcpy(buffer.data() + offset, &entityIdNet, sizeof(entityIdNet));
        offset += sizeof(entityIdNet);

        // 6b. changeType (1 byte, pas de conversion)
        uint8_t changeTypeByte = static_cast<uint8_t>(change.changeType);
        std::memcpy(buffer.data() + offset, &changeTypeByte, sizeof(changeTypeByte));
        offset += sizeof(changeTypeByte);

        // 6c. changedFields (1 byte)
        std::memcpy(buffer.data() + offset, &change.changedFields, sizeof(change.changedFields));
        offset += sizeof(change.changedFields);

        // 6d. Si CREATED ou UPDATED → écrire entity_state complet
        if (change.changeType != ProtocolData::EntityChangeType::DESTROYED)
        {
            // Conversion big endian pour entity_state
            ProtocolData::entity_state stateNet = change.data;
            stateNet.id = htonl(change.data.id);
            stateNet.score = htonl(change.data.score);
            // Note: floats ne nécessitent pas htonl (IEEE 754 standard)
            // Si besoin : convertir avec htonf (custom function)

            std::memcpy(buffer.data() + offset, &stateNet, sizeof(stateNet));
            offset += sizeof(stateNet);
        }
    }

    return buffer;
}

size_t DeltaSnapshotMessage::size() const { return sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::DeltaSnapshot); }

// namespace Protocol