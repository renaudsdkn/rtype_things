#include "../include/server/GameManager.hpp"
#include "../include/server/server.hpp" // Pour m_server->send()
#include "protocol/Factory.hpp"
#include "protocol/serializer.hpp"
#include <iostream>
#include <vector>
#include <memory>
extern std::atomic<bool> g_running;

// --- CONSTRUCTEUR ---
GameManager::GameManager(
    std::shared_ptr<ThreadSafeQueue<NetworkPacket>> incoming,
    std::shared_ptr<UdpServer> server)
    : m_incomingMessages(incoming),
      m_server(server), // ✅ Stocke le shared_ptr
      m_playerManager(),
      m_messageHandler(m_playerManager, *this) // ✅ Passe la référence *this
{
    m_lastCleanupTime = std::chrono::steady_clock::now();
    std::cout << "[GameManager] Initialisé." << std::endl;
}

void GameManager::stop() { m_running = false; }

// --- BOUCLE DE JEU ---
void GameManager::run()
{
    std::cout << "[THREAD JEU] Boucle de jeu démarrée." << std::endl;
    auto lastTime = std::chrono::steady_clock::now();
    const std::chrono::milliseconds tickRate(16); // ~60 FPS

    while (m_running && g_running)
    {
        auto now = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(now - lastTime).count();
        lastTime = now;

        processNetworkInputs();
        updateGame(deltaTime);
        broadcastSnapshots();
        broadcastGameEvents();
        cleanupPlayers();

        auto sleepTime = tickRate - (std::chrono::steady_clock::now() - now);
        if (sleepTime > std::chrono::milliseconds(0))
        {
            std::this_thread::sleep_for(sleepTime);
        }
    }
}

// --- TRAITEMENT DES PAQUETS ENTRANTS ---
void GameManager::processNetworkInputs()
{
    std::queue<NetworkPacket> packets = m_incomingMessages->popAll();
    while (!packets.empty())
    {
        auto packet = std::move(packets.front());
        packets.pop();

        // Met à jour l'activité du joueur
        m_playerManager.updatePlayerActivity(packet.sender);

        // Désérialise et transmet au MessageHandler
        try
        {
            auto message = Protocol::MessageFactory::deserialize(packet.data);
            m_messageHandler.handleMessage(message, packet.sender);
        }
        catch (const std::exception &e)
        {
            std::cerr << "[THREAD JEU] Erreur désérialisation: " << e.what() << std::endl;
        }
    }
}

// --- MISE À JOUR LOGIQUE ---
void GameManager::updateGame(float deltaTime)
{
    for (auto &room : m_rooms)
    {
        room->update(deltaTime);
    }
}

// --- ENVOI DES SNAPSHOTS ---
// --- ENVOI DES SNAPSHOTS ---
void GameManager::broadcastSnapshots()
{
    for (auto &room : m_rooms)
    {
        if (room->isEmpty() || room->getCurrentState() != Room::State::PLAYING)
            continue;

        ProtocolData::Snapshot snapData = room->getSnapshot();
        
        // ✅ AJOUT : Limiter nombre d'entités pour éviter overflow buffer
        const size_t MAX_ENTITIES = 40;  // 40 entités max dans un snapshot
        
        if (snapData.entities.size() > MAX_ENTITIES) {
            std::cerr << "[GameManager] ⚠️ Trop d'entités (" << snapData.entities.size() 
                      << "), on en envoie seulement " << MAX_ENTITIES << std::endl;
            
            // Garde les plus proches du joueur (ou les plus importantes)
            // Option simple : garde les N premières
            snapData.entities.resize(MAX_ENTITIES);
        }
        
        if (snapData.entities.empty())
            continue;

        Protocol::SnapshotMessage msg(snapData);
        std::vector<uint8_t> dataToSend = msg.serialize();
        
        // ✅ AJOUT : Log taille du paquet pour debug
        if (dataToSend.size() > 4096) {
            std::cerr << "[GameManager] ⚠️ Snapshot volumineux: " << dataToSend.size() 
                      << " octets (" << snapData.entities.size() << " entités)" << std::endl;
        }

        // Utilise la méthode broadcastToRoom (plus propre)
        broadcastToRoom(room->getId(), dataToSend);
    }
}

// --- ENVOI DES ÉVÉNEMENTS (COMME GAME_OVER) ---
void GameManager::broadcastGameEvents()
{
    for (auto &room : m_rooms)
    {
        // Si la room vient de passer en GAME_OVER...
        if (room->getCurrentState() == Room::State::GAME_OVER)
        {
            std::cout << "[GameManager] Détection de GAME_OVER dans Room " << room->getId() << ". Diffusion." << std::endl;

            // 1. Préparer le message
            ProtocolData::PlayerEvent eventData;
            eventData.playerId = 0; // 0 = Global
            eventData.type = ProtocolData::PlayerEventType::GAME_OVER;
            // eventData.playerId = htonl(0); // Pas besoin pour 0

            Protocol::PlayerEventMessage gameOverMsg(eventData);
            auto dataToSend = gameOverMsg.serialize();

            // 2. Envoyer aux joueurs de la room
            broadcastToRoom(room->getId(), dataToSend);

            // ✅ CRITIQUE : Change l'état pour ne PAS renvoyer en boucle
            room->setState(Room::State::FINISHED);
            std::cout << "[GameManager] Room " << room->getId() << " passée à FINISHED." << std::endl;
            // --- ✅ PERSISTENCE JSON UNIQUEMENT ---
            try
            {
                uint32_t matchId = static_cast<uint32_t>(std::time(nullptr));
                int roomId = static_cast<int>(room->getId());

                // Récupère les scores depuis la room (Engine authoritative)
                auto playerScores = room->getFinalPlayerScores();

                // Convertit vers ScoreEntry
                std::vector<ScoreEntry> entries;
                for (const auto &ps : playerScores)
                {
                    ScoreEntry e;
                    e.player_id = ps.playerId;
                    e.nickname = m_playerManager.getNickname(ps.playerId);
                    e.score = ps.score;
                    e.kills = ps.kills;
                    e.deaths = ps.deaths;
                    e.extras = nlohmann::json::object(); // ✅ Vide pour l'instant
                    entries.push_back(e);
                }

                // Écrire les fichiers JSON
                ScoreFileWriter writer("data/last_match_scores.json");
                writer.writeMatchScores(matchId, roomId, entries);

                std::string matchFile = "data/match_" + std::to_string(matchId) + ".json";
                ScoreFileWriter writer2(matchFile);
                writer2.writeMatchScores(matchId, roomId, entries);

                std::cout << "[GameManager] ✅ Scores écrits dans " << matchFile << std::endl;
            }
            catch (const std::exception &e)
            {
                std::cerr << "[GameManager] ❌ Erreur persistance JSON: " << e.what() << std::endl;
            }
        }
    }
}

// --- NETTOYAGE DES JOUEURS INACTIFS ---
void GameManager::cleanupPlayers()
{
    if (std::chrono::steady_clock::now() - m_lastCleanupTime > std::chrono::seconds(35))
    {
        // 1. Récupère les IDs des joueurs inactifs (grâce au PlayerManager modifié)
        std::vector<uint32_t> removedPlayerIDs = m_playerManager.removeInactivePlayers(std::chrono::seconds(130));

        // 2. Pour chaque ID...
        for (uint32_t playerId : removedPlayerIDs)
        {
            // ...traite leur déconnexion (les retire de la room, notifie les autres)
            handlePlayerDisconnect(playerId);
        }

        if (!removedPlayerIDs.empty())
        {
            std::cout << "[THREAD JEU] Nettoyage inactifs terminé. " << removedPlayerIDs.size() << " joueur(s) supprimé(s)." << std::endl;
        }
        m_lastCleanupTime = std::chrono::steady_clock::now();
    }
}

// --- NOUVELLES FONCTIONS (Logique Lobby) ---

// Cherche une room dispo ou en crée une
Room *GameManager::findAvailableRoomOrCreate()
{
    for (auto &room : m_rooms)
    {
        Room::State state = room->getCurrentState();
        if ((state == Room::State::WAITING_FOR_PLAYERS || state == Room::State::PLAYING) && !room->isFull())
        {
            return room.get();
        }
    }

    // ✅ NOUVEAU CODE avec config par défaut
    uint32_t newId = m_rooms.empty() ? 1 : m_rooms.back()->getId() + 1;

    // Créer une config par défaut
    ProtocolData::RoomConfig defaultConfig;
    std::memset(&defaultConfig, 0, sizeof(defaultConfig));
    strncpy(defaultConfig.roomName, "Room Auto", 31);
    defaultConfig.roomName[31] = '\0';
    defaultConfig.difficulty = 1; // Normal
    defaultConfig.maxPlayers = 0;
    defaultConfig.enemySpeedMultiplier = 100;
    defaultConfig.spawnRateMultiplier = 100;
    defaultConfig.friendlyFire = 0;
    defaultConfig.powerUpsEnabled = 1;
    defaultConfig.survivalMode = 0;

    m_rooms.push_back(std::make_unique<Room>(newId, *this, defaultConfig)); // ✅ CORRIGÉ
    Room *newRoom = m_rooms.back().get();
    m_idToRoomMap[newId] = newRoom;
    return newRoom;
}

// Trouve une room par son ID
Room *GameManager::getRoomById(uint32_t roomId)
{
    auto it = m_idToRoomMap.find(roomId);
    if (it != m_idToRoomMap.end())
    {
        return it->second;
    }
    // Fallback si la map n'est pas à jour (ne devrait pas arriver)
    for (auto &room : m_rooms)
    {
        if (room->getId() == roomId)
            return room.get();
    }
    return nullptr;
}

// Envoie des données à tous les joueurs d'une room, sauf un (optionnel)
void GameManager::broadcastToRoom(uint32_t roomId, const std::vector<uint8_t> &data, std::optional<uint32_t> excludePlayerId)
{
    Room *room = getRoomById(roomId);
    if (!room)
        return;

    std::vector<asio::ip::udp::endpoint> endpoints = room->getPlayerEndpoints();

    for (const auto &ep : endpoints)
    {
        if (excludePlayerId)
        {
            auto idOpt = m_playerManager.getPlayerIdByEndpoint(ep);
            if (idOpt && idOpt.value() == excludePlayerId.value())
            {
                continue; // N'envoie pas au joueur exclu
            }
        }
        m_server->send(data, ep);
    }
}

// --- GESTION DES REQUÊTES LOBBY (Implémentation) ---

ProtocolData::RoomList GameManager::getRoomListData() const
{
    ProtocolData::RoomList list;
    list.rooms.reserve(m_rooms.size());
    for (const auto &room : m_rooms)
    {
        // N'affiche pas les rooms finies
        if (room->getCurrentState() == Room::State::FINISHED)
            continue;

        ProtocolData::RoomInfo info;
        info.roomId = room->getId();
        info.playerCount = room->getPlayerCount();
        info.maxPlayers = 4; // TODO: Mettre en constante
        info.roomState = static_cast<uint8_t>(room->getCurrentState());
        list.rooms.push_back(info);
    }
    return list;
}

void GameManager::handleListRoomsRequest(uint32_t playerId)
{
    std::cout << "[GameManager] Joueur " << playerId << " demande la liste des rooms." << std::endl;
    auto endpointOpt = m_playerManager.getEndpointById(playerId);
    if (!endpointOpt)
        return;

    ProtocolData::RoomList roomListData = getRoomListData();
    Protocol::RoomListResponseMessage responseMsg(roomListData);
    auto dataToSend = responseMsg.serialize();
    m_server->send(dataToSend, *endpointOpt);
}

void GameManager::handleCreateRoomRequest(uint32_t playerId, const ProtocolData::RoomConfig &config)
{
    std::cout << "[GameManager] Joueur " << playerId << " demande à créer une room avec config." << std::endl;
    auto endpointOpt = m_playerManager.getEndpointById(playerId);
    if (!endpointOpt)
        return;

    handlePlayerLeft(playerId);

    // Crée la room avec la config
    uint32_t newId = m_rooms.empty() ? 1 : m_rooms.back()->getId() + 10;
    m_rooms.push_back(std::make_unique<Room>(newId, *this, config));
    Room *newRoom = m_rooms.back().get();
    m_idToRoomMap[newId] = newRoom;

    newRoom->addPlayer(playerId, *endpointOpt);
    m_playerToRoomMap[playerId] = newRoom;

    ProtocolData::RoomResponse responseData;
    responseData.success = 1;
    responseData.roomId = newId;
    Protocol::CreateRoomResponseMessage responseMsg(responseData);
    auto dataToSend = responseMsg.serialize();

    m_server->send(dataToSend, *endpointOpt);

    std::cout << "[GameManager] Room " << newId << " créée avec nom: '"
              << config.roomName << "'" << std::endl;
}
void GameManager::handleJoinRoomRequest(uint32_t playerId, uint32_t roomId)
{
    std::cout << "[GameManager] Joueur " << playerId << " demande à rejoindre la room " << roomId << "." << std::endl;
    auto endpointOpt = m_playerManager.getEndpointById(playerId);
    if (!endpointOpt)
        return;

    handlePlayerLeft(playerId);

    Room *room = getRoomById(roomId);
    ProtocolData::RoomResponse responseData;
    bool success = false;

    if (room && !room->isFull())
    {
        Room::State state = room->getCurrentState();
        if (state == Room::State::WAITING_FOR_PLAYERS ||
            state == Room::State::STARTING ||
            state == Room::State::PLAYING)
        {

            responseData.success = 1;
            responseData.roomId = roomId;
            Protocol::JoinRoomResponseMessage responseMsg(responseData);
            auto dataToSend = responseMsg.serialize();
            m_server->send(dataToSend, *endpointOpt);

            std::cout << "[GameManager] ✅ JOIN_ROOM_RESPONSE envoyé au joueur " << playerId << std::endl;

            room->addPlayer(playerId, *endpointOpt);
            m_playerToRoomMap[playerId] = room;
            success = true;

            std::cout << "[GameManager] ✅ Joueur " << playerId << " a rejoint la room " << roomId
                      << " (état: " << static_cast<int>(state) << ")" << std::endl;
        }
        else
        {
            std::cout << "[GameManager] ❌ Room " << roomId << " n'est pas joignable (état: "
                      << static_cast<int>(state) << ")" << std::endl;
            responseData.success = 0;
            responseData.roomId = roomId;
            Protocol::JoinRoomResponseMessage responseMsg(responseData);
            m_server->send(responseMsg.serialize(), *endpointOpt);
        }
    }
    else
    {
        responseData.success = 0;
        responseData.roomId = roomId;
        if (!room)
        {
            std::cout << "[GameManager] ❌ Room " << roomId << " introuvable." << std::endl;
        }
        else if (room->isFull())
        {
            std::cout << "[GameManager] ❌ Room " << roomId << " est pleine." << std::endl;
        }
        Protocol::JoinRoomResponseMessage responseMsg(responseData);
        m_server->send(responseMsg.serialize(), *endpointOpt);
    }

    // ✅ MODIFIÉ : Inclut le pseudo dans la notification
    if (success)
    {
        ProtocolData::PlayerRoomNotification notifData;
        notifData.roomId = roomId;
        notifData.playerId = playerId;

        // ✅ Récupère le pseudo
        std::string nickname = m_playerManager.getNickname(playerId);
        strncpy(notifData.nickname, nickname.c_str(), 20);
        notifData.nickname[20] = '\0';

        Protocol::PlayerJoinedRoomMessage notificationMsg(notifData);
        auto dataNotif = notificationMsg.serialize();
        broadcastToRoom(roomId, dataNotif, playerId);
    }
}
// --- MODIFICATION DES ANCIENNES FONCTIONS ---
// ✅ NOUVELLE FONCTION
void GameManager::sendToEndpoint(const asio::ip::udp::endpoint &endpoint, const std::vector<uint8_t> &data)
{
    m_server->send(data, endpoint);
}
void GameManager::handleNewPlayer(uint32_t playerId, const asio::ip::udp::endpoint &endpoint)
{
    m_playerToRoomMap[playerId] = nullptr; // nullptr = dans le lobby
    std::cout << "[GameManager] Joueur " << playerId << " ajouté au lobby." << std::endl;

    // ❌ NE PLUS ENVOYER WELCOME ICI (déjà envoyé dans message_handler)
    // Le message_handler s'occupe maintenant d'envoyer WELCOME avec le pseudo
}

// Gère un joueur qui part (soit quit la room, soit se déconnecte)
void GameManager::handlePlayerLeft(uint32_t playerId)
{
    // Vérifie si le joueur était bien dans une room
    if (m_playerToRoomMap.count(playerId) && m_playerToRoomMap[playerId] != nullptr)
    {
        Room *room = m_playerToRoomMap[playerId];
        uint32_t roomId = room->getId();
        std::cout << "[GameManager] Joueur " << playerId << " quitte la room " << roomId << "." << std::endl;

        room->removePlayer(playerId);

        // Notifie les autres joueurs de la room
        ProtocolData::PlayerRoomNotification notifData;
        notifData.roomId = roomId;
        notifData.playerId = playerId;
        Protocol::PlayerLeftRoomMessage notificationMsg(notifData);
        auto dataNotif = notificationMsg.serialize();

        broadcastToRoom(roomId, dataNotif); // Envoie à tous ceux qui restent
    }
    // Remet le joueur dans le "lobby" (il est toujours connecté au serveur)
    m_playerToRoomMap[playerId] = nullptr;
}

// Gère une déconnexion complète du serveur
void GameManager::handlePlayerDisconnect(uint32_t playerId)
{
    std::cout << "[GameManager] Déconnexion complète du joueur " << playerId << "." << std::endl;
    // 1. Notifie les autres si le joueur était dans une room
    handlePlayerLeft(playerId);

    // 2. Supprime totalement le joueur des maps du GameManager
    if (m_playerToRoomMap.count(playerId))
    {
        m_playerToRoomMap.erase(playerId);
    }
    // (PlayerManager::removeInactivePlayers s'occupe de la map players_)
}

void GameManager::sendToPlayer(uint32_t playerId, const std::vector<uint8_t> &data)
{
    auto endpointOpt = m_playerManager.getEndpointById(playerId);
    if (endpointOpt)
    {
        m_server->send(data, *endpointOpt);
        std::cout << "[GameManager] Message envoyé au joueur " << playerId << std::endl;
    }
    else
    {
        std::cerr << "[GameManager ERREUR] Impossible d'envoyer au joueur " << playerId
                  << " : endpoint introuvable." << std::endl;
    }
}
// ✅ CORRIGÉ : Gère les inputs (évite le segfault)
void GameManager::handlePlayerInput(uint32_t playerId, const ProtocolData::PlayerInput &input)
{
    // Vérifie si le joueur existe ET s'il est dans une room (non-nul)
    if (m_playerToRoomMap.count(playerId) && m_playerToRoomMap[playerId] != nullptr)
    {
        m_playerToRoomMap[playerId]->handleInput(playerId, input);
    }
    // else: Le joueur est dans le lobby, on ignore ses inputs de jeu
    // std::cout << "[GameManager] Input du joueur " << playerId << " (dans le lobby) ignoré." << std::endl;
}

std::optional<uint32_t> GameManager::getRoomIdForPlayer(uint32_t playerId) const
{
    auto it = m_playerToRoomMap.find(playerId);
    if (it != m_playerToRoomMap.end() && it->second != nullptr)
    {
        return it->second->getId();
    }
    return std::nullopt;
}

// ───────────────────────────────────────────────────────────────

std::vector<uint32_t> GameManager::getPlayersInRoom(uint32_t roomId) const
{
    std::vector<uint32_t> result;

    auto roomIt = m_idToRoomMap.find(roomId);
    if (roomIt == m_idToRoomMap.end() || roomIt->second == nullptr)
    {
        return result; // Room inexistante
    }

    Room *room = roomIt->second;

    // Parcourt m_playerToRoomMap pour trouver tous les joueurs de cette room
    for (const auto &[playerId, playerRoom] : m_playerToRoomMap)
    {
        if (playerRoom == room)
        {
            result.push_back(playerId);
        }
    }

    return result;
}

void GameManager::requestShutdown()
{
    std::cout << "[GameManager] 🛑 Shutdown requested by admin" << std::endl;

    // ✅ Arrêter game loop
    extern std::atomic<bool> g_running;
    g_running = false;

    // ✅ Arrêter ASIO (pour débloquer networkThread)
    extern asio::io_context *g_io_context_ptr;
    if (g_io_context_ptr)
    {
        std::cout << "[GameManager] 🛑 Stopping ASIO io_context..." << std::endl;
        g_io_context_ptr->stop();
    }
}

bool GameManager::forceEndRoom(uint32_t roomId)
{
    std::lock_guard<std::mutex> lock(m_roomsMutex); // ✅ LOCK

    for (auto &room : m_rooms)
    {
        if (room->getId() == roomId)
        {
            std::cout << "[GameManager] Admin forcing end of Room " << roomId << std::endl;
            room->setState(Room::State::GAME_OVER);
            return true;
        }
    }
    std::cerr << "[GameManager] Room " << roomId << " not found" << std::endl;
    return false;
}

bool GameManager::kickPlayer(uint32_t playerId)
{
    // handlePlayerDisconnect devrait déjà gérer ses propres locks
    std::cout << "[GameManager] Admin kicking player " << playerId << std::endl;
    handlePlayerDisconnect(playerId);
    return true;
}