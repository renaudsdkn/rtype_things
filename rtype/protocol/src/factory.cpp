#include "../include/protocol/protocol_data.hpp"
#include "../include/protocol/serializer.hpp"

using namespace Protocol;
// ---------------- FACTORY ----------------
std::unique_ptr<IMessage> MessageFactory::deserialize(const std::vector<uint8_t> &buffer)
{
    if (buffer.size() < sizeof(ProtocolData::PacketHeader))
        throw std::runtime_error("Buffer trop petit");

    ProtocolData::PacketHeader header;
    std::memcpy(&header, buffer.data(), sizeof(header));
    header.size = ntohs(header.size);

    if (header.size != buffer.size())
        throw std::runtime_error("Taille du message incohérente");

    ProtocolData::MessageType type = static_cast<ProtocolData::MessageType>(header.type);

    switch (type)
    {
    case ProtocolData::MessageType::CONNECT:
        return std::make_unique<ConnectMessage>();
    case ProtocolData::MessageType::DISCONNECT:
        return std::make_unique<DisconnectMessage>();
    case ProtocolData::MessageType::INPUT:
    {
        ProtocolData::PlayerInput input;
        std::memcpy(&input, buffer.data() + sizeof(ProtocolData::PacketHeader), sizeof(input));
        input.playerId = ntohl(input.playerId);
        return std::make_unique<PlayerInputMessage>(input);
    }

    case ProtocolData::MessageType::WELCOME:
    {
        ProtocolData::Welcome welcome;
        std::memcpy(&welcome, buffer.data() + sizeof(ProtocolData::PacketHeader), sizeof(welcome));
        welcome.playerId = ntohl(welcome.playerId);
        return std::make_unique<WelcomeMessage>(welcome);
    }
    case ProtocolData::MessageType::SNAPSHOT:
    {
        // Vérifications de taille... (important pour la sécurité)

        const uint8_t *ptr = buffer.data() + sizeof(ProtocolData::PacketHeader);

        // 1. Lire le nombre d'entités ET LE CONVERTIR (ntohl)
        uint32_t count;
        uint32_t netCount;
        std::memcpy(&netCount, ptr, sizeof(netCount));
        count = ntohl(netCount); // ✅ Correction Endianness
        ptr += sizeof(uint32_t);

        // Vérification de taille totale attendue... (important)

        ProtocolData::Snapshot snap;
        snap.entities.reserve(count);

        // 2. BOUCLER pour lire CHAQUE entité INDIVIDUELLEMENT
        for (uint32_t i = 0; i < count; ++i)
        {
            ProtocolData::entity_state current_entity;

            // Lire l'ID (4 octets) ET LE CONVERTIR (ntohl)
            uint32_t netId;
            std::memcpy(&netId, ptr, sizeof(netId));
            current_entity.id = ntohl(netId); // ✅ Correction Endianness
            ptr += sizeof(uint32_t);

            // Lire le type (1 octet) - Copie simple
            std::memcpy(&current_entity.type, ptr, sizeof(uint8_t));
            ptr += sizeof(uint8_t);

            // Lire X (4 octets) - Copie simple
            std::memcpy(&current_entity.x, ptr, sizeof(float));
            ptr += sizeof(float);

            // Lire Y (4 octets) - Copie simple
            std::memcpy(&current_entity.y, ptr, sizeof(float));
            ptr += sizeof(float);
            // ✅ IMPORTANT : Si tu as ajouté vx, vy, damage, xp, level à entity_state,
            // IL FAUT AUSSI LES LIRE ICI !
            std::memcpy(&current_entity.vx, ptr, sizeof(float)); ptr += sizeof(float);
             std::memcpy(&current_entity.vy, ptr, sizeof(float)); ptr += sizeof(float);
             std::memcpy(&current_entity.damage, ptr, sizeof(uint8_t)); ptr += sizeof(uint8_t);
             std::memcpy(&current_entity.xp, ptr, sizeof(uint8_t)); ptr += sizeof(uint8_t);
             std::memcpy(&current_entity.level, ptr, sizeof(uint8_t)); ptr += sizeof(uint8_t);
            // Ajouter l'entité reconstruite au vecteur
            snap.entities.push_back(current_entity);
        }
        // Vérification finale du pointeur... (sécurité)

        return std::make_unique<SnapshotMessage>(snap);
    }
    case ProtocolData::MessageType::SPAWN_ENTITY:
    {
        ProtocolData::SpawnEntity spawn;
        std::memcpy(&spawn, buffer.data() + sizeof(ProtocolData::PacketHeader), sizeof(spawn));
        spawn.entity.id = ntohl(spawn.entity.id);
        return std::make_unique<SpawnEntityMessage>(spawn);
    }

    case ProtocolData::MessageType::MOVE_ENTITY:
    {
        ProtocolData::MoveEntity move;
        std::memcpy(&move, buffer.data() + sizeof(ProtocolData::PacketHeader), sizeof(move));
        move.id = ntohl(move.id);
        // Pas besoin de ntohf (float) — floats ne sont pas affectés par l’endian sur UDP ici
        return std::make_unique<MoveEntityMessage>(move);
    }

    case ProtocolData::MessageType::DESTROY_ENTITY:
    {
        ProtocolData::DestroyEntity destroy;
        std::memcpy(&destroy, buffer.data() + sizeof(ProtocolData::PacketHeader), sizeof(destroy));
        destroy.id = ntohl(destroy.id);
        return std::make_unique<DestroyEntityMessage>(destroy);
    }

    case ProtocolData::MessageType::PLAYER_EVENT:
    {
        ProtocolData::PlayerEvent ev;
        std::memcpy(&ev, buffer.data() + sizeof(ProtocolData::PacketHeader), sizeof(ev));
        ev.playerId = ntohl(ev.playerId);
        return std::make_unique<PlayerEventMessage>(ev);
    }

    case ProtocolData::MessageType::PING:
        return std::make_unique<PingMessage>();

    case ProtocolData::MessageType::PING_RESPONSE:
        return std::make_unique<PingResponseMessage>();

    case ProtocolData::MessageType::ERROR:
        return std::make_unique<ErrorMessage>();
    default:
        throw std::runtime_error("Type de message inconnu");
    }
}
