#include "../include/server/room.hpp"
#include <iostream>

Room::Room(uint32_t roomId) : m_id(roomId)
{
    m_engine = std::make_unique<Engine>();
    std::cout << "[THREAD JEU] Room " << m_id << " créée avec son propre moteur Engine." << std::endl;
}

void Room::addPlayer(uint32_t playerId, const asio::ip::udp::endpoint &endpoint)
{
    m_players[playerId] = endpoint;
    m_engine->spawn_player(playerId, 100.f, 100.f + (m_players.size() * 50.f));
    std::cout << "[THREAD JEU] Joueur " << playerId << " ajouté à l'Engine de la Room " << m_id << std::endl;
}

void Room::removePlayer(uint32_t playerId)
{
    if (m_players.count(playerId))
    {
        m_engine->remove_player(playerId); // Demander à l'engine de le supprimer
        m_players.erase(playerId);
        std::cout << "[THREAD JEU] Joueur " << playerId << " retiré de la Room " << m_id << std::endl;
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

void Room::update(float deltaTime)
{
    // --- Logique de Spawn ---
    auto now = std::chrono::steady_clock::now();
    float timeSinceLastSpawn = std::chrono::duration_cast<std::chrono::duration<float>>(now - m_lastEnemySpawnTime).count();

    // Si plus de X secondes se sont écoulées
    if (timeSinceLastSpawn > m_spawnInterval)
    {
        std::cout << "[THREAD JEU] Room " << m_id << ": Spawning enemy..." << std::endl;
        // Fais apparaître un ennemi Grubs sur la droite, à une hauteur Y aléatoire
        float randomY = 100.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (600.0f))); // Entre 100 et 700
        m_engine->spawn_enemy(Components::Enemystype::Grubs, 1300.f, randomY);                           // x=1300 (hors écran droite)

        m_lastEnemySpawnTime = now; // Réinitialise le timer
    }
    // --- Fin Logique de Spawn ---
    // Appeler simplement l'update de ton moteur !
    m_engine->update(deltaTime, 30.0f); // 30.0f = collisionBound par défaut
}

ProtocolData::Snapshot Room::getSnapshot() const
{
    ProtocolData::Snapshot snap;
    const auto &reg = m_engine->get_registry();

    const auto &positions = reg.get_components<Components::Position>();
    const auto &netIds = reg.get_components<Components::NetworkId>();
    // Récupère les composants pour déterminer le type
    const auto &controllables = reg.get_components<Components::Controllable>();
    const auto &enemyTypes = reg.get_components<Components::Enemystype>();
    const auto &bullets = reg.get_components<Components::Bullet>();
    const auto &powerUps = reg.get_components<Components::PowerUp>();

    // Pré-allouer peut améliorer les performances
    snap.entities.reserve(positions.size());

    for (size_t i = 0; i < positions.size(); ++i)
    {
        // Inclure seulement les entités qui ont une position ET un ID réseau
        if (positions[i].has_value() && i < netIds.size() && netIds[i].has_value())
        {
            ProtocolData::entity_state state;

            // --- Remplir les données ---
            state.id = netIds[i]->id; // L'ID réseau (sera converti dans serialize)
            state.x = positions[i]->x;
            state.y = positions[i]->y;

            // --- Déterminer le type ---
            uint8_t entityType = 255; // 255 = Type inconnu/par défaut

            if (i < controllables.size() && controllables[i].has_value())
            {
                entityType = 0; // Convention: 0 = Joueur
            }
            else if (i < enemyTypes.size() && enemyTypes[i].has_value())
            {
                // Convention: 10 + type d'ennemi (pour éviter conflit avec 0)
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

            snap.entities.push_back(state);
        }
    }
    return snap;
}

bool Room::isFull() const { return m_players.size() >= 4; } // Limite de 4 joueurs
bool Room::isEmpty() const { return m_players.empty(); }
uint32_t Room::getId() const { return m_id; }

std::vector<asio::ip::udp::endpoint> Room::getPlayerEndpoints() const
{
    std::vector<asio::ip::udp::endpoint> endpoints;
    for (const auto &[id, ep] : m_players)
    {
        endpoints.push_back(ep);
    }
    return endpoints;
}