#include "../include/protocol/protocol_data.hpp"
#include "../include/protocol/serializer.hpp"
using namespace Protocol;

// --- LIST_ROOMS_REQUEST --- (Client -> Serveur, pas de données)
ProtocolData::MessageType ListRoomsRequestMessage::getType() const {
    return ProtocolData::MessageType::LIST_ROOMS_REQUEST;
}
size_t ListRoomsRequestMessage::size() const {
    return sizeof(ProtocolData::PacketHeader);
}
std::vector<uint8_t> ListRoomsRequestMessage::serialize() const {
    ProtocolData::PacketHeader header{
        htons(sizeof(ProtocolData::PacketHeader)),
        static_cast<uint8_t>(ProtocolData::MessageType::LIST_ROOMS_REQUEST)
    };
    std::vector<uint8_t> buffer(sizeof(header));
    std::memcpy(buffer.data(), &header, sizeof(header));
    return buffer;
}


// --- ROOM_LIST_RESPONSE --- (Serveur -> Client, liste de rooms)
RoomListResponseMessage::RoomListResponseMessage(const ProtocolData::RoomList &data)
    : data_(data) {}

ProtocolData::MessageType RoomListResponseMessage::getType() const {
    return ProtocolData::MessageType::ROOM_LIST_RESPONSE;
}

// Gère la sérialisation du vecteur
size_t RoomListResponseMessage::size() const {
    // Taille = header + 1 octet pour le count + N * taille d'une RoomInfo
    return sizeof(ProtocolData::PacketHeader) + sizeof(uint8_t) + data_.rooms.size() * sizeof(ProtocolData::RoomInfo);
}

std::vector<uint8_t> RoomListResponseMessage::serialize() const {
    // Limite à 255 rooms max à cause du uint8_t pour le count
    uint8_t numEntities = static_cast<uint8_t>(data_.rooms.size());
    uint16_t totalSize = sizeof(ProtocolData::PacketHeader) + sizeof(uint8_t) + numEntities * sizeof(ProtocolData::RoomInfo);

    ProtocolData::PacketHeader header;
    header.size = htons(totalSize);
    header.type = static_cast<uint8_t>(ProtocolData::MessageType::ROOM_LIST_RESPONSE);

    std::vector<uint8_t> buffer(totalSize);
    uint8_t* ptr = buffer.data();

    // 1. Écrire le header
    std::memcpy(ptr, &header, sizeof(header));
    ptr += sizeof(header);

    // 2. Écrire le nombre de rooms (sur 1 octet)
    std::memcpy(ptr, &numEntities, sizeof(uint8_t));
    ptr += sizeof(uint8_t);

    // 3. Écrire chaque RoomInfo en convertissant le roomId
    for (const auto& roomInfo : data_.rooms) {
        ProtocolData::RoomInfo tmp = roomInfo;
        tmp.roomId = htonl(tmp.roomId); // ✅ Conversion Endianness
        // Les autres champs (playerCount, maxPlayers, roomState) sont des uint8_t, pas besoin de convertir

        std::memcpy(ptr, &tmp, sizeof(tmp));
        ptr += sizeof(ProtocolData::RoomInfo);
    }
    return buffer;
}

// --- CREATE_ROOM_REQUEST --- (Client -> Serveur, pas de données)
// Constructeur MODIFIÉ
// ✅ Constructeur avec config
CreateRoomRequestMessage::CreateRoomRequestMessage(const ProtocolData::RoomConfig& config) {
    std::memset(&data_, 0, sizeof(data_));
    data_.config = config;
}

// Méthode serialize() existante (à vérifier)
std::vector<uint8_t> CreateRoomRequestMessage::serialize() const {
    size_t totalSize = sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::CreateRoomRequest);
    ProtocolData::PacketHeader header{
        htons(totalSize),
        static_cast<uint8_t>(ProtocolData::MessageType::CREATE_ROOM_REQUEST)
    };
    
    std::vector<uint8_t> buffer(totalSize);
    std::memcpy(buffer.data(), &header, sizeof(header));
    std::memcpy(buffer.data() + sizeof(header), &data_, sizeof(data_));
    return buffer;
}

size_t CreateRoomRequestMessage::size() const {
    return sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::CreateRoomRequest);
}
ProtocolData::MessageType CreateRoomRequestMessage::getType() const {
    return ProtocolData::MessageType::CREATE_ROOM_REQUEST;
}



// --- CREATE_ROOM_RESPONSE --- (Serveur -> Client)
CreateRoomResponseMessage::CreateRoomResponseMessage(const ProtocolData::RoomResponse &data)
    : data_(data) {}

ProtocolData::MessageType CreateRoomResponseMessage::getType() const {
    return ProtocolData::MessageType::CREATE_ROOM_RESPONSE;
}
size_t CreateRoomResponseMessage::size() const {
    return sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::RoomResponse);
}
std::vector<uint8_t> CreateRoomResponseMessage::serialize() const {
    ProtocolData::PacketHeader header{
        static_cast<uint16_t>(sizeof(header) + sizeof(data_)),
        static_cast<uint8_t>(ProtocolData::MessageType::CREATE_ROOM_RESPONSE)
    };
    header.size = htons(header.size);

    ProtocolData::RoomResponse tmp = data_;
    tmp.roomId = htonl(tmp.roomId); // ✅ Conversion Endianness

    std::vector<uint8_t> buffer(sizeof(header) + sizeof(tmp));
    std::memcpy(buffer.data(), &header, sizeof(header));
    std::memcpy(buffer.data() + sizeof(header), &tmp, sizeof(tmp));
    return buffer;
}

// --- JOIN_ROOM_REQUEST --- (Client -> Serveur)
JoinRoomRequestMessage::JoinRoomRequestMessage(const ProtocolData::JoinRoomRequest &data)
    : data_(data) {}

ProtocolData::MessageType JoinRoomRequestMessage::getType() const {
    return ProtocolData::MessageType::JOIN_ROOM_REQUEST;
}
size_t JoinRoomRequestMessage::size() const {
    return sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::JoinRoomRequest);
}

// ❌ TON ANCIENNE VERSION ÉTAIT INCORRECTE (manquait htonl)
std::vector<uint8_t> JoinRoomRequestMessage::serialize() const {
    ProtocolData::PacketHeader header{
        static_cast<uint16_t>(sizeof(header) + sizeof(data_)),
        static_cast<uint8_t>(ProtocolData::MessageType::JOIN_ROOM_REQUEST)
    };
    header.size = htons(header.size);

    // ✅ Correction
    ProtocolData::JoinRoomRequest tmp = data_;
    tmp.roomId = htonl(tmp.roomId); // Conversion Endianness

    std::vector<uint8_t> buffer(sizeof(header) + sizeof(tmp));
    std::memcpy(buffer.data(), &header, sizeof(header));
    std::memcpy(buffer.data() + sizeof(header), &tmp, sizeof(tmp)); // Utilise tmp
    return buffer;
}

// --- JOIN_ROOM_RESPONSE --- (Serveur -> Client)
JoinRoomResponseMessage::JoinRoomResponseMessage(const ProtocolData::RoomResponse &data)
    : data_(data) {}

ProtocolData::MessageType JoinRoomResponseMessage::getType() const {
    return ProtocolData::MessageType::JOIN_ROOM_RESPONSE;
}
size_t JoinRoomResponseMessage::size() const {
    return sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::RoomResponse);
}

// ❌ TON ANCIENNE VERSION ÉTAIT INCORRECTE (manquait htonl)
std::vector<uint8_t> JoinRoomResponseMessage::serialize() const {
    ProtocolData::PacketHeader header{
        static_cast<uint16_t>(sizeof(header) + sizeof(data_)),
        static_cast<uint8_t>(ProtocolData::MessageType::JOIN_ROOM_RESPONSE)
    };
    header.size = htons(header.size);

    // ✅ Correction
    ProtocolData::RoomResponse tmp = data_;
    tmp.roomId = htonl(tmp.roomId); // Conversion Endianness

    std::vector<uint8_t> buffer(sizeof(header) + sizeof(tmp));
    std::memcpy(buffer.data(), &header, sizeof(header));
    std::memcpy(buffer.data() + sizeof(header), &tmp, sizeof(tmp)); // Utilise tmp
    return buffer;
}

// --- PLAYER_JOINED_ROOM --- (Notification Serveur -> Clients)
PlayerJoinedRoomMessage::PlayerJoinedRoomMessage(const ProtocolData::PlayerRoomNotification &data)
    : data_(data) {}

ProtocolData::MessageType PlayerJoinedRoomMessage::getType() const {
    return ProtocolData::MessageType::PLAYER_JOINED_ROOM;
}
size_t PlayerJoinedRoomMessage::size() const {
    return sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::PlayerRoomNotification);
}

// ❌ TON ANCIENNE VERSION ÉTAIT INCORRECTE (manquait htonl)
std::vector<uint8_t> PlayerJoinedRoomMessage::serialize() const {
    ProtocolData::PacketHeader header{
        htons(sizeof(ProtocolData::PacketHeader) + sizeof(data_)),
        static_cast<uint8_t>(ProtocolData::MessageType::PLAYER_JOINED_ROOM)
    };

    // ✅ Correction
    ProtocolData::PlayerRoomNotification tmp = data_;
    tmp.roomId = htonl(tmp.roomId);
    tmp.playerId = htonl(tmp.playerId);

    std::vector<uint8_t> buffer(sizeof(header) + sizeof(tmp));
    std::memcpy(buffer.data(), &header, sizeof(header));
    std::memcpy(buffer.data() + sizeof(header), &tmp, sizeof(tmp)); // Utilise tmp
    return buffer;
}

// --- PLAYER_LEFT_ROOM --- (Notification Serveur -> Clients)
PlayerLeftRoomMessage::PlayerLeftRoomMessage(const ProtocolData::PlayerRoomNotification &data)
    : data_(data) {}

ProtocolData::MessageType PlayerLeftRoomMessage::getType() const {
    return ProtocolData::MessageType::PLAYER_LEFT_ROOM;
}
size_t PlayerLeftRoomMessage::size() const {
    return sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::PlayerRoomNotification);
}

// ❌ TON ANCIENNE VERSION ÉTAIT INCORRECTE (manquait htonl)
std::vector<uint8_t> PlayerLeftRoomMessage::serialize() const {
    ProtocolData::PacketHeader header{
        htons(sizeof(ProtocolData::PacketHeader) + sizeof(data_)),
        static_cast<uint8_t>(ProtocolData::MessageType::PLAYER_LEFT_ROOM)
    };
    
    // ✅ Correction
    ProtocolData::PlayerRoomNotification tmp = data_;
    tmp.roomId = htonl(tmp.roomId);
    tmp.playerId = htonl(tmp.playerId);

    std::vector<uint8_t> buffer(sizeof(header) + sizeof(tmp));
    std::memcpy(buffer.data(), &header, sizeof(header));
    std::memcpy(buffer.data() + sizeof(header), &tmp, sizeof(tmp)); // Utilise tmp
    return buffer;
}

// --- GAME_STARTING --- (Notification Serveur -> Clients)
ProtocolData::MessageType GameStartingMessage::getType() const {
    return ProtocolData::MessageType::GAME_STARTING;
}
size_t GameStartingMessage::size() const {
    return sizeof(ProtocolData::PacketHeader);
}
std::vector<uint8_t> GameStartingMessage::serialize() const {
    ProtocolData::PacketHeader header{
        htons(sizeof(ProtocolData::PacketHeader)),
        static_cast<uint8_t>(ProtocolData::MessageType::GAME_STARTING)
    };
    std::vector<uint8_t> buffer(sizeof(header));
    std::memcpy(buffer.data(), &header, sizeof(header));
    return buffer;
}