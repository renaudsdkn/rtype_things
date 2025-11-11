#include "../include/protocol/protocol_data.hpp"
#include "../include/protocol/serializer.hpp"

using namespace Protocol;
// ---------------- FACTORY ----------------
std::unique_ptr<IMessage> MessageFactory::deserialize(const std::vector<uint8_t> &buffer)
{

    if (buffer.size() < sizeof(ProtocolData::PacketHeader))
        throw std::runtime_error("Buffer trop petit pour le Header");

    ProtocolData::PacketHeader header;
    std::memcpy(&header, buffer.data(), sizeof(header));
    header.size = ntohs(header.size);

    if (header.size != buffer.size())
        throw std::runtime_error("Taille de message incohérente (Header: " + std::to_string(header.size) + ", Reçu: " + std::to_string(buffer.size()) + ")");

    ProtocolData::MessageType type = static_cast<ProtocolData::MessageType>(header.type);
    const uint8_t *data_ptr = buffer.data() + sizeof(ProtocolData::PacketHeader);
    size_t data_len = buffer.size() - sizeof(ProtocolData::PacketHeader);

    switch (type)
    {
    case ProtocolData::MessageType::CONNECT:
    {
        if (data_len < sizeof(ProtocolData::ConnectRequest))
            throw std::runtime_error("Buffer trop court pour CONNECT");

        ProtocolData::ConnectRequest reqData;
        std::memcpy(&reqData, data_ptr, sizeof(reqData));
        reqData.nickname[20] = '\0';

        std::cout << "[FACTORY DEBUG] CONNECT désérialisé avec pseudo: '" << reqData.nickname << "'" << std::endl;

        return std::make_unique<ConnectMessage>(std::string(reqData.nickname));
    }
    case ProtocolData::MessageType::DISCONNECT:
        return std::make_unique<DisconnectMessage>();
    case ProtocolData::MessageType::INPUT:
    {
        ProtocolData::PlayerInput input;
        std::memcpy(&input, buffer.data() + sizeof(ProtocolData::PacketHeader), sizeof(input));
        input.playerId = ntohl(input.playerId);
        return std::make_unique<PlayerInputMessage>(input);
    }
    case ProtocolData::MessageType::LEAVE_ROOM_REQUEST:
    {
        ProtocolData::LeaveRoomRequest request;
        std::memcpy(&request, buffer.data() + sizeof(ProtocolData::PacketHeader), sizeof(request));
        request.roomId = ntohl(request.roomId);
        return std::make_unique<LeaveRoomRequestMessage>(request);
    }
    case ProtocolData::MessageType::WELCOME:
    {
        if (data_len < sizeof(ProtocolData::Welcome))
            throw std::runtime_error("Buffer trop petit pour WELCOME");
        ProtocolData::Welcome welcome;
        std::memcpy(&welcome, data_ptr, sizeof(welcome));
        welcome.playerId = ntohl(welcome.playerId); // Convertit l'ID
        return std::make_unique<WelcomeMessage>(welcome);
    }
    case ProtocolData::MessageType::SNAPSHOT:
    {
        // Vérifications de taille... (important pour la sécurité)
        if (data_len < sizeof(ProtocolData::Snapshot))
            throw std::runtime_error("Buffer trop petit pour SNAPSHOT");
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
            std::memcpy(&current_entity.vx, ptr, sizeof(float));
            ptr += sizeof(float);
            std::memcpy(&current_entity.vy, ptr, sizeof(float));
            ptr += sizeof(float);
            std::memcpy(&current_entity.damage, ptr, sizeof(uint8_t));
            ptr += sizeof(uint8_t);
            std::memcpy(&current_entity.xp, ptr, sizeof(uint8_t));
            ptr += sizeof(uint8_t);
            std::memcpy(&current_entity.level, ptr, sizeof(uint8_t));
            ptr += sizeof(uint8_t);
            std::memcpy(&current_entity.health, ptr, sizeof(uint8_t));
            ptr += sizeof(uint8_t);
            // Ajouter l'entité reconstruite au vecteur
            snap.entities.push_back(current_entity);
        }
        // Vérification finale du pointeur... (sécurité)

        return std::make_unique<SnapshotMessage>(snap);
    }
    case ProtocolData::MessageType::PLAYER_EVENT:
    {
        ProtocolData::PlayerEvent ev;
        std::memcpy(&ev, buffer.data() + sizeof(ProtocolData::PacketHeader), sizeof(ev));
        ev.playerId = ntohl(ev.playerId);
        return std::make_unique<PlayerEventMessage>(ev);
    }
    case ProtocolData::MessageType::LIST_ROOMS_REQUEST:
    {
        // Pas de données supplémentaires, juste le header
        return std::make_unique<ListRoomsRequestMessage>();
    }

    case ProtocolData::MessageType::ROOM_LIST_RESPONSE:
    {
        // 1. Lire le nombre de rooms (uint8_t)
        if (data_len < sizeof(uint8_t))
            throw std::runtime_error("Buffer trop petit pour ROOM_LIST count");
        uint8_t count = *data_ptr;
        data_ptr += sizeof(uint8_t);
        data_len -= sizeof(uint8_t);

        // 2. Vérifier la taille restante
        if (data_len != count * sizeof(ProtocolData::RoomInfo))
        {
            throw std::runtime_error("Taille incohérente pour ROOM_LIST data");
        }

        ProtocolData::RoomList roomList;
        roomList.rooms.reserve(count);

        // 3. Lire chaque RoomInfo
        for (int i = 0; i < count; ++i)
        {
            ProtocolData::RoomInfo roomInfo;
            std::memcpy(&roomInfo, data_ptr, sizeof(ProtocolData::RoomInfo));
            roomInfo.roomId = ntohl(roomInfo.roomId); // ✅ Conversion Endianness
            // (les autres champs sont uint8_t, pas de conversion)
            roomList.rooms.push_back(roomInfo);
            data_ptr += sizeof(ProtocolData::RoomInfo);
        }
        return std::make_unique<RoomListResponseMessage>(roomList);
    }

    case ProtocolData::MessageType::CREATE_ROOM_REQUEST:
    {
        // ✅ ANCIEN CODE (vide) :
        // return std::make_unique<CreateRoomRequestMessage>();

        // ✅ NOUVEAU CODE (avec config) :
        if (data_len < sizeof(ProtocolData::CreateRoomRequest))
            throw std::runtime_error("Buffer trop court pour CREATE_ROOM_REQUEST");

        ProtocolData::CreateRoomRequest requestData;
        std::memcpy(&requestData, data_ptr, sizeof(requestData));

        // Assurer null terminator pour le nom
        requestData.config.roomName[31] = '\0';

        std::cout << "[FACTORY DEBUG] CREATE_ROOM désérialisé avec config:" << std::endl;
        std::cout << "  - Nom: '" << requestData.config.roomName << "'" << std::endl;
        std::cout << "  - Difficulté: " << (int)requestData.config.difficulty << std::endl;
        std::cout << "  - Max joueurs: " << (int)requestData.config.maxPlayers << std::endl;

        return std::make_unique<CreateRoomRequestMessage>(requestData.config);
    }
    case ProtocolData::MessageType::CREATE_ROOM_RESPONSE:
    {
        if (data_len < sizeof(ProtocolData::RoomResponse))
            throw std::runtime_error("Buffer trop petit pour CREATE_ROOM_RESPONSE");
        ProtocolData::RoomResponse responseData;
        std::memcpy(&responseData, data_ptr, sizeof(responseData));
        responseData.roomId = ntohl(responseData.roomId); // ✅ Conversion Endianness
        return std::make_unique<CreateRoomResponseMessage>(responseData);
    }

    case ProtocolData::MessageType::JOIN_ROOM_REQUEST:
    {
        if (data_len < sizeof(ProtocolData::JoinRoomRequest))
            throw std::runtime_error("Buffer trop petit pour JOIN_ROOM_REQUEST");
        ProtocolData::JoinRoomRequest requestData;
        std::memcpy(&requestData, data_ptr, sizeof(requestData));
        requestData.roomId = ntohl(requestData.roomId); // ✅ Conversion Endianness
        return std::make_unique<JoinRoomRequestMessage>(requestData);
    }

    case ProtocolData::MessageType::JOIN_ROOM_RESPONSE:
    {
        if (data_len < sizeof(ProtocolData::RoomResponse))
            throw std::runtime_error("Buffer trop petit pour JOIN_ROOM_RESPONSE");
        ProtocolData::RoomResponse responseData;
        std::memcpy(&responseData, data_ptr, sizeof(responseData));
        responseData.roomId = ntohl(responseData.roomId); // ✅ Conversion Endianness
        return std::make_unique<JoinRoomResponseMessage>(responseData);
    }

    case ProtocolData::MessageType::PLAYER_JOINED_ROOM:
    {
        if (data_len < sizeof(ProtocolData::PlayerRoomNotification))
            throw std::runtime_error("Buffer trop petit pour PLAYER_JOINED_ROOM");
        ProtocolData::PlayerRoomNotification notifData;
        std::memcpy(&notifData, data_ptr, sizeof(notifData));
        notifData.roomId = ntohl(notifData.roomId);     // ✅ Conversion Endianness
        notifData.playerId = ntohl(notifData.playerId); // ✅ Conversion Endianness
        return std::make_unique<PlayerJoinedRoomMessage>(notifData);
    }

    case ProtocolData::MessageType::PLAYER_LEFT_ROOM:
    {
        if (data_len < sizeof(ProtocolData::PlayerRoomNotification))
            throw std::runtime_error("Buffer trop petit pour PLAYER_LEFT_ROOM");
        ProtocolData::PlayerRoomNotification notifData;
        std::memcpy(&notifData, data_ptr, sizeof(notifData));
        notifData.roomId = ntohl(notifData.roomId);     // ✅ Conversion Endianness
        notifData.playerId = ntohl(notifData.playerId); // ✅ Conversion Endianness
        return std::make_unique<PlayerLeftRoomMessage>(notifData);
    }

    case ProtocolData::MessageType::GAME_STARTING:
    {
        // Pas de données supplémentaires
        return std::make_unique<GameStartingMessage>();
    }

    case ProtocolData::MessageType::ERROR:
        return std::make_unique<ErrorMessage>();
        // ═══════════════════════════════════════════════════════════
    case ProtocolData::MessageType::DELTA_SNAPSHOT:
    {
        ProtocolData::DeltaSnapshot delta;
        size_t offset = sizeof(ProtocolData::PacketHeader);

        // 1. Lire snapshotId
        uint32_t snapshotIdNet;
        std::memcpy(&snapshotIdNet, buffer.data() + offset, sizeof(snapshotIdNet));
        delta.snapshotId = ntohl(snapshotIdNet);
        offset += sizeof(snapshotIdNet);

        // 2. Lire changeCount
        uint16_t changeCountNet;
        std::memcpy(&changeCountNet, buffer.data() + offset, sizeof(changeCountNet));
        delta.changeCount = ntohs(changeCountNet);
        offset += sizeof(changeCountNet);

        // 3. Lire timestamp
        uint32_t timestampNet;
        std::memcpy(&timestampNet, buffer.data() + offset, sizeof(timestampNet));
        delta.timestamp = ntohl(timestampNet);
        offset += sizeof(timestampNet);

        // 4. Lire chaque EntityChange
        delta.changes.reserve(delta.changeCount);

        for (uint16_t i = 0; i < delta.changeCount; ++i)
        {
            ProtocolData::EntityChange change;

            // 4a. entityId
            uint32_t entityIdNet;
            std::memcpy(&entityIdNet, buffer.data() + offset, sizeof(entityIdNet));
            change.entityId = ntohl(entityIdNet);
            offset += sizeof(entityIdNet);

            // 4b. changeType
            uint8_t changeTypeByte;
            std::memcpy(&changeTypeByte, buffer.data() + offset, sizeof(changeTypeByte));
            change.changeType = static_cast<ProtocolData::EntityChangeType>(changeTypeByte);
            offset += sizeof(changeTypeByte);

            // 4c. changedFields
            std::memcpy(&change.changedFields, buffer.data() + offset, sizeof(change.changedFields));
            offset += sizeof(change.changedFields);

            // 4d. Si CREATED ou UPDATED → lire entity_state
            if (change.changeType != ProtocolData::EntityChangeType::DESTROYED)
            {
                ProtocolData::entity_state stateNet;
                std::memcpy(&stateNet, buffer.data() + offset, sizeof(stateNet));

                // Conversion ntohl pour champs uint32_t
                change.data.id = ntohl(stateNet.id);
                change.data.score = ntohl(stateNet.score);
                change.data.type = stateNet.type;
                change.data.x = stateNet.x; // float (pas de conversion)
                change.data.y = stateNet.y;
                change.data.vx = stateNet.vx;
                change.data.vy = stateNet.vy;
                change.data.damage = stateNet.damage;
                change.data.xp = stateNet.xp;
                change.data.level = stateNet.level;
                change.data.health = stateNet.health;

                offset += sizeof(stateNet);
            }

            delta.changes.push_back(change);
        }

        std::cout << "[FACTORY DEBUG] DELTA_SNAPSHOT désérialisé:" << std::endl;
        std::cout << "  - Sequence: " << delta.snapshotId << std::endl;
        std::cout << "  - Changes: " << delta.changeCount << std::endl;

        return std::make_unique<DeltaSnapshotMessage>(delta);
    }
    default:
        throw std::runtime_error("Type de message inconnu");
    }
}
