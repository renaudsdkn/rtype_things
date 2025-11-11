#include "../include/client/GameClient.hpp"
#include <set>      // Pour std::set
#include <vector>   // Pour std::vector (si besoin)
#include <iostream> // Pour les logs
#include "raylib.h" // Pour DrawRectangleRec et les couleurs

// Dans rtype/client/src/GameClient.cpp

#include "../include/ecs/engine.hpp" // Pour les composants

// --- Constructeur ---
GameClient::GameClient(InputManager &input, RTypeClient &networkClient)
    : m_input(input),
      m_client(networkClient),
      m_tempNickname(""),
      m_confirmedNickname("")
{
    initECS();
    initDefaultRoomConfig(); // ✅ NOUVEAU
    m_state.store(State::ENTERING_NICKNAME);
    std::cout << "[GameClient] Initialisé. État: ENTERING_NICKNAME" << std::endl;
}

// --- initECS (Modifié) ---

// ✅ CORRECTION : Enregistrer TOUS les composants
void GameClient::initECS()
{
    // Composants de base
    m_registry.register_component<Components::Position>();
    m_registry.register_component<Components::Velocity>();
    m_registry.register_component<Components::NetworkId>();
    m_registry.register_component<Components::Drawable>();

    // Composants pour les statistiques
    m_registry.register_component<Components::PlayerStats>();
    m_registry.register_component<Components::Healthpoints_t>();
    m_registry.register_component<Components::Damage_t>();
    m_registry.register_component<Components::AttackType>();
    m_registry.register_component<Components::Controllable>();

    // Composants pour identifier les types d'entités
    m_registry.register_component<Components::Enemystype>();
    m_registry.register_component<Components::Bullet>();
    m_registry.register_component<Components::PowerUp>();

    std::cout << "[GameClient] ECS Client initialisé avec tous les composants." << std::endl;
}

// --- Vide l'ECS (quand on quitte une partie) ---
void GameClient::cleanupGameEntities()
{
    // Recrée une registry vide pour tout effacer
    m_registry = ECS::registry();
    initECS(); // Ré-enregistre les composants
    networkIdToEntityMap.clear();
    m_player_entity.reset();
    m_playerScore = 0;
    std::cout << "[GameClient] ECS nettoyée." << std::endl;
}

void GameClient::setLocalPlayerId(uint32_t id)
{
    m_localPlayerNetworkId = id;
    std::cout << "[GameClient] ID joueur local défini: " << id << std::endl;

    // ❌ NE PLUS passer automatiquement au LOBBY
    // Le client doit d'abord recevoir la confirmation du pseudo (WELCOME avec accepted=1)
}

// --- Synchro Snapshot (Callback de SNAPSHOT) ---
void GameClient::updateFromServer(const ProtocolData::Snapshot &snapshot)
{
    if (m_state != State::PLAYING && m_state != State::GAME_OVER)
        return;
    m_networkStats.packetsReceived++;
    m_networkStats.entitiesCount = snapshot.entities.size();
    m_networkStats.lastUpdateTime = GetTime();

    // ✅ Estime taille paquet (header + entités)
    size_t estimatedSize = sizeof(ProtocolData::PacketHeader);
    estimatedSize += snapshot.entities.size() * sizeof(ProtocolData::entity_state);
    m_networkStats.bytesReceived += estimatedSize;

    std::set<uint32_t> receivedNetworkIds;

    for (const auto &entityState : snapshot.entities)
    {
        uint32_t netId = entityState.id;
        receivedNetworkIds.insert(netId);
        auto it = networkIdToEntityMap.find(netId);

        if (it != networkIdToEntityMap.end())
        {
            // === MISE À JOUR d'une entité existante ===
            ECS::entity_t localEntity = it->second;

            // Position
            auto &pos = m_registry.get_components<Components::Position>()[localEntity];
            if (pos)
            {
                pos->x = entityState.x;
                pos->y = entityState.y;
            }

            // Velocity
            auto &vel = m_registry.get_components<Components::Velocity>()[localEntity];
            if (vel)
            {
                vel->x = entityState.vx;
                vel->y = entityState.vy;
            }

            // ✅ NOUVEAU : Dégâts
            auto &damages = m_registry.get_components<Components::Damage_t>()[localEntity];
            if (damages)
            {
                damages->_damage = entityState.damage;
            }

            // ✅ NOUVEAU : XP et Level (si c'est un joueur)
            if (entityState.type == 0)
            { // Type 0 = Joueur
                auto &stats = m_registry.get_components<Components::PlayerStats>()[localEntity];
                if (stats)
                {
                    stats->xp = entityState.xp;
                    stats->level = entityState.level;
                }

                // ✅ NOUVEAU : Santé (approximation, le snapshot ne l'envoie pas actuellement)
                // TODO: Ajouter 'health' dans entity_state si nécessaire
                auto &health = m_registry.get_components<Components::Healthpoints_t>()[localEntity];
                if (health)
                {
                    // Si le snapshot n'envoie pas la santé, on la garde telle quelle
                    // Ou on pourrait utiliser 'damage' comme indicateur ?
                }
            }
        }
        else
        {
            // === CRÉATION d'une nouvelle entité ===
            ECS::entity_t newEntity = m_registry.spawn_entity();

            // Composants de base
            m_registry.emplace_component<Components::NetworkId>(newEntity, netId);
            m_registry.emplace_component<Components::Position>(newEntity, entityState.x, entityState.y);
            m_registry.emplace_component<Components::Velocity>(newEntity, entityState.vx, entityState.vy);
            m_registry.emplace_component<Components::Drawable>(newEntity, entityState.type);

            // ✅ NOUVEAU : Identifie le TYPE et ajoute les composants appropriés
            if (entityState.type == 0)
            {
                // === JOUEUR ===
                m_registry.emplace_component<Components::PlayerStats>(newEntity, entityState.xp, entityState.level);
                m_registry.emplace_component<Components::Healthpoints_t>(newEntity, 100); // Santé par défaut
                m_registry.emplace_component<Components::Damage_t>(newEntity, entityState.damage);
                m_registry.emplace_component<Components::Controllable>(newEntity);
                m_registry.emplace_component<Components::AttackType>(newEntity, Components::AttackType::StdShot); // Arme par défaut

                // Marque comme joueur local si c'est notre ID
                if (m_localPlayerNetworkId.has_value() && netId == m_localPlayerNetworkId.value())
                {
                    m_player_entity = newEntity;
                    std::cout << "[GameClient] Joueur local créé (entité " << (size_t)newEntity << ")" << std::endl;
                }
            }
            else if (entityState.type >= 10 && entityState.type < 100)
            {
                // === ENNEMI ===
                // anim.add_l()
                Components::Enemystype enemyType = static_cast<Components::Enemystype>(entityState.type - 10);
                m_registry.emplace_component<Components::Enemystype>(newEntity, enemyType);
                m_registry.emplace_component<Components::Healthpoints_t>(newEntity, 100); // Santé par défaut
                m_registry.emplace_component<Components::Damage_t>(newEntity, entityState.damage);
            }
            else if (entityState.type == 100)
            {
                // === BALLE ===
                m_registry.emplace_component<Components::Bullet>(newEntity);
                m_registry.emplace_component<Components::Damage_t>(newEntity, entityState.damage);
                m_registry.emplace_component<Components::AttackType>(newEntity, Components::AttackType::StdShot); // Type par défaut
            }
            else if (entityState.type >= 200)
            {
                // === POWER-UP ===
                m_registry.emplace_component<Components::PowerUp>(newEntity);
            }

            networkIdToEntityMap[netId] = newEntity;
            std::cout << "[GameClient] Entité créée (type " << (int)entityState.type
                      << ", netId " << netId << ", entité " << (size_t)newEntity << ")" << std::endl;
        }
    }

    // === SUPPRESSION des entités disparues ===
    for (auto it = networkIdToEntityMap.begin(); it != networkIdToEntityMap.end();)
    {
        if (receivedNetworkIds.find(it->first) == receivedNetworkIds.end())
        {
            std::cout << "[GameClient] Entité " << it->first << " supprimée (disparue du snapshot)" << std::endl;
            m_registry.kill_entity(it->second);
            it = networkIdToEntityMap.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
// --- Callback Événements (GAME_OVER) ---
void GameClient::handlePlayerEvent(const ProtocolData::PlayerEvent &event)
{
    std::cout << "[GameClient] PlayerEvent reçu : type="
              << static_cast<int>(event.type) << std::endl;
    if (event.type == ProtocolData::PlayerEventType::GAME_OVER)
    {
        std::cout << "[GameClient] Événement GAME_OVER reçu !" << std::endl;
        m_state = State::GAME_OVER;
    }
}

// --- Callbacks Lobby ---
void GameClient::handleRoomList(const ProtocolData::RoomList &list)
{
    if (m_state == State::LOBBY)
    { // Ne met à jour que si on est dans le lobby
        m_roomList = list.rooms;
        std::cout << "[GameClient] Liste des rooms mise à jour (" << m_roomList.size() << " rooms)." << std::endl;
    }
}

void GameClient::handleRoomResponse(ProtocolData::MessageType type, const ProtocolData::RoomResponse &response)
{
    if (response.success == 1)
    {
        std::cout << "[GameClient] Succès " << (type == ProtocolData::MessageType::CREATE_ROOM_RESPONSE ? "Création" : "Join") << ". Room ID: " << response.roomId << std::endl;
        m_currentRoomId = response.roomId;
        m_state = State::WAITING_IN_ROOM;                                                                                              // On attend dans la room
        m_roomList.clear();                                                                                                            // On n'est plus dans le lobby
        m_playersInRoom.clear();                                                                                                       // Vide la liste des joueurs
        m_playersInRoom[m_localPlayerNetworkId.value_or(0)] = "Vous (ID: " + std::to_string(m_localPlayerNetworkId.value_or(0)) + ")"; // Ajoute soi-même
    }
    else
    {
        std::cerr << "[GameClient] Échec " << (type == ProtocolData::MessageType::CREATE_ROOM_RESPONSE ? "Création" : "Join") << "." << std::endl;
        m_state = State::LOBBY; // Reste dans le lobby
        requestListRooms();     // Rafraîchit la liste pour voir ce qui n'allait pas
    }
}

void GameClient::handlePlayerNotification(ProtocolData::MessageType type, const ProtocolData::PlayerRoomNotification &notif)
{
    // Vérifie si on est bien dans la room concernée
    if (m_state != State::WAITING_IN_ROOM || !m_currentRoomId.has_value() || notif.roomId != m_currentRoomId.value())
        return;

    if (type == ProtocolData::MessageType::PLAYER_JOINED_ROOM)
    {
        std::cout << "[GameClient] Joueur " << notif.playerId << " a rejoint la room." << std::endl;
        m_playersInRoom[notif.playerId] = "Joueur " + std::to_string(notif.playerId); // Ajoute à la liste
    }
    else if (type == ProtocolData::MessageType::PLAYER_LEFT_ROOM)
    {
        std::cout << "[GameClient] Joueur " << notif.playerId << " a quitté la room." << std::endl;
        m_playersInRoom.erase(notif.playerId); // Retire de la liste
    }
}

void GameClient::handleWelcome(uint32_t playerId, const ProtocolData::Welcome &welcomeData)
{
    if (welcomeData.accepted == 1)
    {
        // ✅ Pseudo accepté !
        m_localPlayerNetworkId = playerId;
        m_confirmedNickname = std::string(welcomeData.confirmedNickname);

        std::cout << "[GameClient] Pseudo accepté: '" << m_confirmedNickname
                  << "'. Passage au LOBBY." << std::endl;

        setState(State::LOBBY);

        // ✅ NOUVEAU : Demande la liste des rooms
        // (À implémenter dans GameClient si pas déjà fait)
    }
    else
    {
        // ❌ Pseudo refusé
        std::string reason = std::string(welcomeData.reason);
        std::cout << "[GameClient] Pseudo refusé: " << reason << std::endl;

        // ✅ Retourne à l'écran de saisie
        setState(State::ENTERING_NICKNAME);

        // ✅ Stocke la raison pour l'afficher
        m_nicknameRejectionReason = reason;
    }
}

void GameClient::handleGameStarting()
{
    if (m_state == State::WAITING_IN_ROOM)
    {
        std::cout << "[GameClient] La partie commence !" << std::endl;
        m_state = State::PLAYING;
        m_playersInRoom.clear(); // Vide la liste d'attente (inutile en jeu)
    }
    else
    {
        std::cout << "[GameClient] ❌ État incorrect: " << static_cast<int>(m_state.load()) << std::endl;
    }
}

// --- Accesseurs (Implémentation) ---
std::vector<std::string> GameClient::getPlayerNicknamesInRoom() const
{
    std::vector<std::string> nicknames;
    for (auto const &[id, name] : m_playersInRoom)
        nicknames.push_back(name);
    return nicknames;
}
int GameClient::getPlayerScore() const
{
    // TODO: Récupérer le score depuis l'ECS (PlayerStats)

    return m_playerScore;
}
void GameClient::setState(State newState)
{
    m_state.store(newState);
}

// --- Actions (appelées par Game::handleInput) ---
void GameClient::requestListRooms()
{
    m_client.sendListRoomsRequest();
}
void GameClient::requestCreateRoom()
{
    m_client.sendCreateRoomRequest();
}
void GameClient::requestJoinRoom(uint32_t roomId)
{
    m_client.sendJoinRoomRequest(roomId);
}
void GameClient::requestLeaveRoom()
{
    // TODO: Ajouter un message LEAVE_ROOM au protocole
    // m_client.sendLeaveRoomRequest(m_currentRoomId.value_or(0));
    if (m_currentRoomId.has_value())
    {
        m_client.sendLeaveRoomRequest(*m_currentRoomId);
    }
    m_state = State::LOBBY; // Retour lobby
    cleanupGameEntities();  // Nettoie l'ECS
    requestListRooms();     // Demande la liste
}
void GameClient::requestStartGame()
{
    // TODO: Si le "host" doit démarrer la partie
    // m_client.sendStartGameRequest(m_currentRoomId.value_or(0));
}

// --- Gestion des Inputs ---
void GameClient::processInput()
{
    CommandList commands = m_input.getCommands();
    // Envoie les inputs de jeu (mouvement, tir) SEULEMENT si on est en jeu
    if (m_state == State::PLAYING)
    {
        bool up = false, down = false, left = false, right = false, shoot = false;
        for (PlayerAction cmd : commands)
        {
            switch (cmd)
            {
            case PlayerAction::MOVE_UP:
                up = true;
                break;
            case PlayerAction::MOVE_DOWN:
                down = true;
                break;
            case PlayerAction::MOVE_LEFT:
                left = true;
                break;
            case PlayerAction::MOVE_RIGHT:
                right = true;
                break;
            case PlayerAction::SHOOT:
                shoot = true;
                break;
            default:
                break;
            }
        }
        ProtocolData::PlayerInput inputData{};
        inputData.up = up;
        inputData.down = down;
        inputData.left = left;
        inputData.right = right;
        inputData.shoot = shoot;
        m_client.send_input(inputData);
    }
}
void GameClient::initDefaultRoomConfig()
{
    std::memset(&m_roomConfig, 0, sizeof(m_roomConfig));

    // Valeurs par défaut
    strncpy(m_roomConfig.roomName, "Ma Partie", 31);
    m_roomConfig.roomName[31] = '\0';

    m_roomConfig.difficulty = 1;             // Normal
    m_roomConfig.maxPlayers = 0;             // 4 joueurs
    m_roomConfig.enemySpeedMultiplier = 100; // 100% (vitesse normale)
    m_roomConfig.spawnRateMultiplier = 100;  // 100% (spawn normal)
    m_roomConfig.friendlyFire = 0;           // Désactivé
    m_roomConfig.powerUpsEnabled = 1;        // Activé
    m_roomConfig.survivalMode = 0;           // Normal

    std::cout << "[GameClient] Config par défaut initialisée." << std::endl;
}
// --- Logique Client (Prédiction/Interpolation) ---
void GameClient::updatePrediction()
{
    if (m_state != State::PLAYING)
        return;

    // Logique d'interpolation/prédiction (comme avant)
    auto &positions = m_registry.get_components<Position>();
    auto &velocities = m_registry.get_components<Velocity>();
    float frameTime = GetFrameTime(); // Nécessite Raylib (devrait être passé en paramètre)

    for (size_t i = 0; i < positions.size(); ++i)
    {
        if (positions[i].has_value() && i < velocities.size() && velocities[i].has_value())
        {
            // Ne pas interpoler notre propre joueur (on pourrait prédire à la place)
            if (m_player_entity.has_value() && i == (size_t)m_player_entity.value())
            {
                // Prédiction simple : applique la vélocité
                positions[i]->x += velocities[i]->x * frameTime;
                positions[i]->y += velocities[i]->y * frameTime;
            }
            else
            {
                // Interpolation simple pour les autres
                positions[i]->x += velocities[i]->x * frameTime;
                positions[i]->y += velocities[i]->y * frameTime;
            }
        }
    }
}

void GameClient::applyDeltaSnapshot(const ProtocolData::DeltaSnapshot &delta)
{
    std::cout << "[GameClient] 📦 Delta snapshot #" << delta.snapshotId
              << " (" << delta.changeCount << " changes)" << std::endl;

    for (const auto &change : delta.changes)
    {
        switch (change.changeType)
        {
        case ProtocolData::EntityChangeType::CREATED:
        {
            std::cout << "[GameClient] ➕ CREATE entity " << change.entityId
                      << " (type=" << (int)change.data.type << ")" << std::endl;
            createEntityFromState(change.data);
            break;
        }

        case ProtocolData::EntityChangeType::UPDATED:
        {
            // std::cout << "[GameClient] ♻️ UPDATE entity " << change.entityId << std::endl;
            updateEntityFromState(change.entityId, change.data);
            break;
        }

        case ProtocolData::EntityChangeType::DESTROYED:
        {
            std::cout << "[GameClient] ❌ DESTROY entity " << change.entityId << std::endl;
            destroyEntityByNetworkId(change.entityId);
            break;
        }

        default:
            std::cerr << "[GameClient] ⚠️ Type de changement inconnu: "
                      << (int)change.changeType << std::endl;
            break;
        }
    }
}

// ─────────────────────────────────────────────────────────────
// Helpers (adaptés à TON ECS)
// ─────────────────────────────────────────────────────────────

void GameClient::createEntityFromState(const ProtocolData::entity_state &state)
{
    // ✅ Utilise TON registry (pas m_engine.createEntity)
    ECS::entity_t localEntity = m_registry.spawn_entity();

    // ✅ Composants de base (TOUJOURS présents)
    m_registry.emplace_component<Components::NetworkId>(localEntity, state.id);
    m_registry.emplace_component<Components::Position>(localEntity, state.x, state.y);
    m_registry.emplace_component<Components::Velocity>(localEntity, state.vx, state.vy);
    m_registry.emplace_component<Components::Drawable>(localEntity, state.type);

    // ✅ Type spécifique
    if (state.type == 0)
    {
        // === JOUEUR ===
        m_registry.emplace_component<Components::PlayerStats>(localEntity, state.xp, state.level, state.score);
        m_registry.emplace_component<Components::Healthpoints_t>(localEntity, state.health);
        m_registry.emplace_component<Components::Damage_t>(localEntity, state.damage);
        m_registry.emplace_component<Components::Controllable>(localEntity);
        m_registry.emplace_component<Components::AttackType>(localEntity, Components::AttackType::StdShot);

        // Si c'est NOTRE joueur local
        if (m_localPlayerNetworkId.has_value() && state.id == m_localPlayerNetworkId.value())
        {
            m_player_entity = localEntity;
            std::cout << "[GameClient] 🎮 Joueur local créé (entité " << (size_t)localEntity << ")" << std::endl;
        }
    }
    else if (state.type >= 10 && state.type < 100)
    {
        // === ENNEMI ===
        Components::Enemystype enemyType = static_cast<Components::Enemystype>(state.type - 10);
        m_registry.emplace_component<Components::Enemystype>(localEntity, enemyType);
        m_registry.emplace_component<Components::Healthpoints_t>(localEntity, state.health);
        m_registry.emplace_component<Components::Damage_t>(localEntity, state.damage);
    }
    else if (state.type == 100)
    {
        // === BALLE ===
        m_registry.emplace_component<Components::Bullet>(localEntity);
        m_registry.emplace_component<Components::Damage_t>(localEntity, state.damage);
        m_registry.emplace_component<Components::AttackType>(localEntity, Components::AttackType::StdShot);
    }
    else if (state.type >= 200)
    {
        // === POWER-UP ===
        m_registry.emplace_component<Components::PowerUp>(localEntity);
    }

    // ✅ Map networkId → localEntity
    networkIdToEntityMap[state.id] = localEntity;

    std::cout << "[GameClient] Entité créée (netId=" << state.id
              << ", localEntity=" << (size_t)localEntity << ")" << std::endl;
}

void GameClient::updateEntityFromState(uint32_t networkId, const ProtocolData::entity_state &state)
{
    // ✅ Trouve l'entité locale via TA map
    auto it = networkIdToEntityMap.find(networkId);

    if (it == networkIdToEntityMap.end())
    {
        std::cerr << "[GameClient] ⚠️ UPDATE entité " << networkId
                  << " introuvable, création..." << std::endl;
        createEntityFromState(state);
        return;
    }

    ECS::entity_t localEntity = it->second;

    // ✅ Met à jour Position
    auto &positions = m_registry.get_components<Components::Position>();
    if (localEntity < positions.size() && positions[localEntity].has_value())
    {
        positions[localEntity]->x = state.x;
        positions[localEntity]->y = state.y;
    }

    // ✅ Met à jour Velocity
    auto &velocities = m_registry.get_components<Components::Velocity>();
    if (localEntity < velocities.size() && velocities[localEntity].has_value())
    {
        velocities[localEntity]->x = state.vx;
        velocities[localEntity]->y = state.vy;
    }

    // ✅ Met à jour Santé (si présent)
    auto &healths = m_registry.get_components<Components::Healthpoints_t>();
    if (localEntity < healths.size() && healths[localEntity].has_value())
    {
        healths[localEntity]->_hp = state.health;
    }

    // ✅ Met à jour Stats (si joueur)
    if (state.type == 0)
    {
        auto &stats = m_registry.get_components<Components::PlayerStats>();
        if (localEntity < stats.size() && stats[localEntity].has_value())
        {
            stats[localEntity]->xp = state.xp;
            stats[localEntity]->level = state.level;
            stats[localEntity]->score = state.score;
        }
    }
}

void GameClient::destroyEntityByNetworkId(uint32_t networkId)
{
    auto it = networkIdToEntityMap.find(networkId);

    if (it == networkIdToEntityMap.end())
    {
        std::cerr << "[GameClient] ⚠️ DESTROY entité " << networkId
                  << " introuvable (déjà détruite ?)" << std::endl;
        return;
    }

    ECS::entity_t localEntity = it->second;

    // ✅ Détruit dans TON registry
    m_registry.kill_entity(localEntity);

    // ✅ Retire de la map
    networkIdToEntityMap.erase(it);

    std::cout << "[GameClient] Entité détruite (netId=" << networkId
              << ", localEntity=" << (size_t)localEntity << ")" << std::endl;
}

void GameClient::updateNetworkStats() {
    float now = GetTime();
    float elapsed = now - m_networkStats.lastCalcTime;
    
    // Recalcule toutes les secondes
    if (elapsed >= 1.0f) {
        size_t bytesThisSecond = m_networkStats.bytesReceived - m_networkStats.lastBytesSnapshot;
        m_networkStats.currentKbps = (bytesThisSecond / 1024.0f) / elapsed;
        
        m_networkStats.lastBytesSnapshot = m_networkStats.bytesReceived;
        m_networkStats.lastCalcTime = now;
    }
}