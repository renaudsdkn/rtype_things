#include "../include/server/room.hpp"
#include <algorithm>
// ✅ Constructeur modifié
Room::Room(uint32_t roomId, GameManager &gameManager, const ProtocolData::RoomConfig &config)
    : m_id(roomId),
      m_gameManagerRef(gameManager),
      m_currentState(State::WAITING_FOR_PLAYERS),
      m_config(config)
{
    m_engine = std::make_unique<Engine>();

    // ✅ Calcule l'intervalle de spawn selon la config
    float baseInterval = 5.0f; // 5 secondes par défaut
    m_spawnInterval = baseInterval * (100.0f / m_config.spawnRateMultiplier);

    std::cout << "[THREAD JEU] Room " << m_id << " créée avec config:" << std::endl;
    std::cout << "  - Nom: " << m_config.roomName << std::endl;
    std::cout << "  - Difficulté: " << (int)m_config.difficulty << std::endl;
    std::cout << "  - Max joueurs: " << (int)m_config.maxPlayers << std::endl;
    std::cout << "  - Spawn interval: " << m_spawnInterval << "s" << std::endl;
}

void Room::addPlayer(uint32_t playerId, const asio::ip::udp::endpoint &endpoint)
{
    if (isFull())
    {
        std::cout << "[Room " << m_id << "] Room pleine!" << std::endl;
        return;
    }
    if (m_players.count(playerId))
    {
        std::cerr << "[Room " << m_id << "] ⚠️ Joueur " << playerId << " déjà dans la room !" << std::endl;
        return;
    }
    m_players[playerId] = endpoint;
    m_engine->spawn_player(playerId, 100.f, 100.f + (m_players.size() * 50.f));

    const auto &playerEntities = m_engine->get_player_entities();
    bool found = false;

    for (auto ent : playerEntities)
    {
        auto &networkIds = m_engine->get_registry().get_components<Components::NetworkId>();
        if (ent < networkIds.size() && networkIds[ent].has_value())
        {
            if (networkIds[ent]->id == playerId)
            {
                found = true;
                std::cout << "[Room " << m_id << "] ✅ Joueur " << playerId
                          << " CONFIRMÉ dans Engine (entité " << (size_t)ent << ")" << std::endl;
                break;
            }
        }
    }

    if (!found)
    {
        std::cerr << "[Room " << m_id << "] ❌❌❌ ERREUR CRITIQUE : Joueur " << playerId
                  << " NON TROUVÉ dans Engine après spawn !" << std::endl;
        std::cerr << "[Room " << m_id << "] player_entity_set size: " << playerEntities.size() << std::endl;
    }

    std::cout << "[Room " << m_id << "] ✅ Joueur " << playerId
              << " ajouté. Total: " << m_players.size() << std::endl;
    State currentState = m_currentState.load();

    // ✅ CAS 1 : La room était en attente, on démarre bientôt
    if (currentState == State::WAITING_FOR_PLAYERS && m_players.size() >= 2)
    {
        m_currentState.store(State::STARTING);
        m_gameStartTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
        std::cout << "[Room " << m_id << "] Démarrage programmé dans 500ms..." << std::endl;
    }
    // ✅ CAS 2 : La partie est DÉJÀ en cours (STARTING ou PLAYING)
    else if (currentState == State::STARTING || currentState == State::PLAYING)
    {
        std::cout << "[Room " << m_id << "] ⚡ Joueur " << playerId
                  << " rejoint une partie en cours (état: " << static_cast<int>(currentState)
                  << "). Envoi immédiat de GAME_STARTING..." << std::endl;

        // Prépare le message GAME_STARTING
        ProtocolData::PacketHeader header{
            htons(sizeof(ProtocolData::PacketHeader)),
            static_cast<uint8_t>(ProtocolData::MessageType::GAME_STARTING)};
        std::vector<uint8_t> data(sizeof(header));
        std::memcpy(data.data(), &header, sizeof(header));

        // ✅ Envoie UNIQUEMENT au nouveau joueur
        m_gameManagerRef.sendToPlayer(playerId, data);
        std::cout << "[Room " << m_id << "] 📡 GAME_STARTING envoyé au joueur " << playerId << std::endl;
    }
}
void Room::removePlayer(uint32_t playerId)
{
    // ✅ CORRECTION : D'abord retirer le joueur
    if (m_players.count(playerId))
    {
        m_engine->remove_player(playerId);
        m_players.erase(playerId);
        std::cout << "[THREAD JEU] Joueur " << playerId << " retiré de la Room " << m_id << std::endl;
    }

    // ✅ PUIS vérifier si la room est vide APRÈS le retrait
    if (m_players.empty())
    {
        State currentState = m_currentState.load();

        // Si la room était en jeu et devient vide → GAME_OVER
        if (currentState == State::PLAYING || currentState == State::STARTING)
        {
            m_currentState.store(State::GAME_OVER);
            std::cout << "[THREAD JEU] Room " << m_id << ": Tous les joueurs sont partis, GAME OVER." << std::endl;
        }
        // Si elle était en attente → reste en attente (réutilisable)
    }
}

void Room::update(float deltaTime)
{
    State currentStateSnapshot = m_currentState.load(); // Lit l'état atomique une fois

    // ✅ NOUVEAU : Si on est en STARTING, vérifier si c'est le moment de démarrer
    if (currentStateSnapshot == State::STARTING)
    {
        auto now = std::chrono::steady_clock::now();
        if (now >= m_gameStartTime)
        {
            // C'est l'heure de démarrer !
            m_currentState.store(State::PLAYING);
            m_lastEnemySpawnTime = now;

            std::cout << "[Room " << m_id << "] 🎮 La partie commence MAINTENANT avec "
                      << m_players.size() << " joueurs!" << std::endl;

            // Prépare et envoie GAME_STARTING
            ProtocolData::PacketHeader header{
                htons(sizeof(ProtocolData::PacketHeader)),
                static_cast<uint8_t>(ProtocolData::MessageType::GAME_STARTING)};
            std::vector<uint8_t> data(sizeof(header));
            std::memcpy(data.data(), &header, sizeof(header));

            m_gameManagerRef.broadcastToRoom(m_id, data);
            std::cout << "[Room " << m_id << "] 📡 GAME_STARTING diffusé" << std::endl;
        }
        return; // Ne fait rien d'autre tant qu'on n'a pas démarré
    }

    if (currentStateSnapshot != State::PLAYING)
    {
        return; // Ne fait rien si pas en jeu
    }

    // --- Logique de Spawn ---
    auto now = std::chrono::steady_clock::now();
    float timeSinceLastSpawn = std::chrono::duration_cast<std::chrono::duration<float>>(now - m_lastEnemySpawnTime).count();

    if (timeSinceLastSpawn > m_spawnInterval)
    {
        std::cout << "[THREAD JEU] Room " << m_id << ": Spawning enemy..." << std::endl;
        float randomY = 100.0f + static_cast<float>(rand() % 601); // Entre 100 et 700
        Components::Enemystype enemyType;
        switch (m_config.difficulty)
        {
        case 0:
            enemyType = Components::Enemystype::Grubs;
            break; // Facile
        case 1:
            enemyType = Components::Enemystype::Flyers;
            break; // Normal
        case 2:
            enemyType = Components::Enemystype::Dobkeratops;
            break; // Difficile
        default:
            enemyType = Components::Enemystype::Grubs;
        }

        m_engine->spawn_enemy(enemyType, 1300.f, randomY);
        m_lastEnemySpawnTime = now;
    }

    // --- Update Engine avec vitesse modifiée ---
    float baseSpeed = 30.0f;
    float modifiedSpeed = baseSpeed * (m_config.enemySpeedMultiplier / 100.0f);
    m_engine->update(deltaTime, modifiedSpeed);

    // --- Vérification de Fin de Partie ---
    if (m_engine->hasLost())
    {
        // Tente de changer l'état de PLAYING à GAME_OVER
        State expectedState = State::PLAYING;
        if (m_currentState.compare_exchange_strong(expectedState, State::GAME_OVER))
        {
            // On vient juste de passer à GAME_OVER
            std::cout << "[THREAD JEU] Room " << m_id << ": GAME OVER!" << std::endl;

            // Créer le message PlayerEvent GAME_OVER
            ProtocolData::PlayerEvent eventData;
            eventData.playerId = 0; // 0 pour global
            eventData.type = ProtocolData::PlayerEventType::GAME_OVER;

            Protocol::PlayerEventMessage gameOverMsg(eventData);
            auto dataToSend = gameOverMsg.serialize();

            // ✅ APPEL FINAL : Utilise la référence au GameManager pour envoyer
            m_gameManagerRef.broadcastToRoom(m_id, dataToSend);
            std::cout << "[THREAD JEU] Room " << m_id << ": Envoi du message GAME_OVER aux joueurs." << std::endl;
        }
    }
}

void Room::handleInput(uint32_t playerId, const ProtocolData::PlayerInput &input)
{
    // Traduire le paquet réseau en structure de données pour l'Engine
    InputData engineInputs;
    engineInputs.up = input.up;
    engineInputs.down = input.down;
    engineInputs.left = input.left;
    engineInputs.right = input.right;
    engineInputs.shoot = input.shoot;

    // Transmettre à l'engine
    m_engine->handle_input(playerId, engineInputs);
}

ProtocolData::Snapshot Room::getSnapshot() const
{
    ProtocolData::Snapshot snap;
    const auto &reg = m_engine->get_registry(); // Récupère la registry de l'Engine

    // Récupère TOUTES les sparse_array nécessaires
    const auto &positions = reg.get_components<Components::Position>();
    const auto &velocities = reg.get_components<Components::Velocity>(); // ✅ Pour vx, vy
    const auto &netIds = reg.get_components<Components::NetworkId>();
    const auto &controllables = reg.get_components<Components::Controllable>();
    const auto &enemyTypes = reg.get_components<Components::Enemystype>();
    const auto &bullets = reg.get_components<Components::Bullet>();
    const auto &powerUps = reg.get_components<Components::PowerUp>();
    const auto &damages = reg.get_components<Components::Damage_t>();         // ✅ Pour damage
    const auto &player_stats = reg.get_components<Components::PlayerStats>(); // ✅ Pour xp, level
    auto &health_ps = reg.get_components<Components::Healthpoints_t>();       // ✅ Pour health
    snap.entities.reserve(positions.size());                                  // Pré-allocation

    for (size_t i = 0; i < positions.size(); ++i)
    {
        // Condition : l'entité doit avoir AU MINIMUM une position et un ID réseau
        if (positions[i].has_value() && i < netIds.size() && netIds[i].has_value())
        {
            ProtocolData::entity_state state; // Crée la structure à envoyer

            // --- Remplir les données ---
            state.id = netIds[i]->id; // Sera converti dans serialize()
            state.x = positions[i]->x;
            state.y = positions[i]->y;

            // --- Remplir la vélocité (si l'entité en a une) ---
            if (i < velocities.size() && velocities[i].has_value())
            {
                state.vx = velocities[i]->x;
                state.vy = velocities[i]->y;
            }
            else
            {
                state.vx = 0.f; // Valeur par défaut si pas de vélocité
                state.vy = 0.f;
            }

            // --- Remplir les dégâts (si l'entité en a) ---
            if (i < damages.size() && damages[i].has_value())
            {
                // Attention à la conversion potentielle si Damage_t::_damage est > 255
                state.damage = static_cast<uint8_t>(damages[i]->_damage);
            }
            else
            {
                state.damage = 0; // Valeur par défaut
            }

            // --- Remplir XP et Level (si c'est un joueur) ---
            state.xp = 0;    // Valeur par défaut
            state.level = 0; // Valeur par défaut
            if (i < player_stats.size() && player_stats[i].has_value())
            {
                // Attention aux conversions si xp/level peuvent dépasser 255
                state.xp = static_cast<uint8_t>(player_stats[i]->xp);
                state.level = static_cast<uint8_t>(player_stats[i]->level);
            }
            // Après avoir rempli damage/xp/level
            if (i < health_ps.size() && health_ps[i].has_value())
            {
                int hp = static_cast<int>(health_ps[i].value());
                state.health = static_cast<uint8_t>(std::min(255, hp));
            }
            else
            {
                state.health = 100; // Valeur par défaut
            }

            state.score = 0;
            if (i < netIds.size() && netIds[i].has_value())
            {
                uint32_t netId = netIds[i]->id;
                // m_engine expose get_player_score(netId)
                try
                {
                    state.score = static_cast<uint32_t>(m_engine->get_player_score(netId));
                }
                catch (...)
                {
                    state.score = 0;
                }
            }

            // --- Déterminer le type ---
            uint8_t entityType = 255; // 255 = Type inconnu/par défaut
            if (i < controllables.size() && controllables[i].has_value())
            {
                entityType = 0; // Convention: 0 = Joueur
            }
            else if (i < enemyTypes.size() && enemyTypes[i].has_value())
            {
                entityType = 10 + static_cast<uint8_t>(enemyTypes[i].value());
            }
            else if (i < bullets.size() && bullets[i].has_value() && bullets[i]->active)
            {
                entityType = 100; // Convention: 100 = Balle
            }
            else if (i < powerUps.size() && powerUps[i].has_value())
            {
                entityType = 200; // Convention: 200 = PowerUp
            }
            state.type = entityType;

            // Ajouter l'état complet de l'entité au snapshot
            snap.entities.push_back(state);
        }
    }
    return snap;
}

bool Room::isFull() const
{
    return m_players.size() >= m_config.maxPlayers;
}
bool Room::isEmpty() const { return m_players.empty(); }
uint32_t Room::getId() const { return m_id; }

Room::State Room::getCurrentState() const
{
    return m_currentState.load(std::memory_order_relaxed);
}

void Room::setState(State newState)
{
    m_currentState.store(newState, std::memory_order_relaxed);
}

size_t Room::getPlayerCount() const
{
    return m_players.size();
}

std::vector<asio::ip::udp::endpoint> Room::getPlayerEndpoints() const
{
    std::vector<asio::ip::udp::endpoint> endpoints;
    endpoints.reserve(m_players.size());
    for (const auto &[id, ep] : m_players)
    {
        endpoints.push_back(ep);
    }
    return endpoints;
}

// --- NOUVEAU : collecte les scores finaux depuis l'Engine
std::vector<Engine::PlayerScoreInfo> Room::getFinalPlayerScores() const
{
    if (!m_engine)
        return {};
    return m_engine->collect_all_player_scores();
}

#include <algorithm>
std::vector<ProtocolData::EntityChange> Room::computeDiff(
    const ProtocolData::Snapshot &oldSnap,
    const ProtocolData::Snapshot &newSnap)
{
    std::vector<ProtocolData::EntityChange> changes;

    // ✅ CORRIGÉ : Utilise entity_state
    std::unordered_map<uint32_t, ProtocolData::entity_state> oldEntitiesMap;
    std::unordered_map<uint32_t, ProtocolData::entity_state> newEntitiesMap;

    for (const auto &entity : oldSnap.entities)
    {
        oldEntitiesMap[entity.id] = entity; // ✅ Utilise .id (pas .networkId)
    }

    for (const auto &entity : newSnap.entities)
    {
        newEntitiesMap[entity.id] = entity; // ✅ Utilise .id
    }

    // Détecter CREATED et UPDATED
    for (const auto &[entityId, newData] : newEntitiesMap)
    {
        auto oldIt = oldEntitiesMap.find(entityId);

        if (oldIt == oldEntitiesMap.end())
        {
            // ✅ CREATED
            ProtocolData::EntityChange change;
            change.entityId = entityId;
            change.changeType = ProtocolData::EntityChangeType::CREATED;
            change.data = newData;
            changes.push_back(change);
        }
        else
        {
            // ✅ UPDATED
            const auto &oldData = oldIt->second;

            bool hasChanged = (oldData.x != newData.x ||
                               oldData.y != newData.y ||
                               oldData.vx != newData.vx ||
                               oldData.vy != newData.vy ||
                               oldData.health != newData.health ||
                               oldData.score != newData.score);

            if (hasChanged)
            {
                ProtocolData::EntityChange change;
                change.entityId = entityId;
                change.changeType = ProtocolData::EntityChangeType::UPDATED;
                change.data = newData;
                change.changedFields = 0;

                if (oldData.x != newData.x || oldData.y != newData.y)
                {
                    change.changedFields |= 0x01; // Position changed
                }
                if (oldData.health != newData.health)
                {
                    change.changedFields |= 0x02; // Health changed
                }

                changes.push_back(change);
            }
        }
    }

    // Détecter DESTROYED
    for (const auto &[entityId, oldData] : oldEntitiesMap)
    {
        if (newEntitiesMap.find(entityId) == newEntitiesMap.end())
        {
            ProtocolData::EntityChange change;
            change.entityId = entityId;
            change.changeType = ProtocolData::EntityChangeType::DESTROYED;
            changes.push_back(change);
        }
    }

    return changes;
}

void Room::broadcastDeltaSnapshot()
{
    ProtocolData::Snapshot currentSnapshot = getSnapshot();
    m_snapshotSequence++;

    // ✅ CORRIGÉ : Itère sur la map (paire key/value)
    for (const auto &[playerId, endpoint] : m_players)
    { // ← CORRIGÉ
        ProtocolData::Snapshot &lastSnapshot = m_playerLastSnapshots[playerId];

        // Premier snapshot → envoie complet
        if (lastSnapshot.entities.empty())
        {
            std::cout << "[Room] Premier snapshot pour joueur " << playerId
                      << " (snapshot complet)" << std::endl;

            Protocol::SnapshotMessage fullMsg(currentSnapshot);
            auto dataToSend = fullMsg.serialize();
            m_gameManagerRef.sendToEndpoint(endpoint, dataToSend); // ✅ Utilise endpoint direct

            lastSnapshot = currentSnapshot;
            continue;
        }

        // Calcule delta
        auto changes = computeDiff(lastSnapshot, currentSnapshot);

        // Si aucun changement, skip
        if (changes.empty())
        {
            continue;
        }

        // Construire DeltaSnapshot
        ProtocolData::DeltaSnapshot deltaSnap;
        deltaSnap.snapshotId = m_snapshotSequence;
        deltaSnap.changeCount = static_cast<uint16_t>(changes.size());
        deltaSnap.changes = std::move(changes);
        deltaSnap.timestamp = static_cast<uint32_t>(std::time(nullptr));

        // Limite sécurité
        const size_t MAX_CHANGES = 50;
        if (deltaSnap.changeCount > MAX_CHANGES)
        {
            std::cerr << "[Room] ⚠️ Trop de changements (" << deltaSnap.changeCount
                      << "), limite à " << MAX_CHANGES << std::endl;
            deltaSnap.changes.resize(MAX_CHANGES);
            deltaSnap.changeCount = MAX_CHANGES;
        }

        // Envoyer delta
        sendDeltaSnapshot(playerId, deltaSnap);

        // Sauvegarder snapshot actuel
        lastSnapshot = currentSnapshot;
    }
}
void Room::sendDeltaSnapshot(uint32_t playerId, const ProtocolData::DeltaSnapshot &delta)
{
    // ✅ Utilise DeltaSnapshotMessage (comme les autres messages)
    Protocol::DeltaSnapshotMessage msg(delta);
    auto buffer = msg.serialize();

    // Envoyer
    auto endpointOpt = m_gameManagerRef.getPlayerManager().getEndpointById(playerId);
    if (endpointOpt)
    {
        m_gameManagerRef.sendToEndpoint(*endpointOpt, buffer);
    }
    else
    {
        std::cerr << "[Room] Impossible d'envoyer delta au joueur " << playerId
                  << " (endpoint introuvable)" << std::endl;
    }
}