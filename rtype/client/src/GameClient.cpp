#include "../include/client/GameClient.hpp"
#include <set> // Pour std::set
#include <vector> // Pour std::vector (si besoin)
#include <iostream> // Pour les logs
#include "raylib.h" // Pour DrawRectangleRec et les couleurs

// ✅ Constructeur prend RTypeClient&
GameClient::GameClient(InputManager& input, RTypeClient& networkClient)
    : m_input(input),
      m_client(networkClient) // ✅ Initialise la référence m_client
{
    initECS();
    std::cout << "[GameClient] Initialisé." << std::endl;
}

// --- Stockage de l'ID Joueur ---
void GameClient::setLocalPlayerId(uint32_t networkId) {
    m_localPlayerNetworkId = networkId;
    std::cout << "[GameClient] ID joueur local défini sur : " << networkId << std::endl;
}

// --- Implémentation de la Synchronisation ---
void GameClient::updateFromServer(const ProtocolData::Snapshot& snapshot) {
    // 1. Noter les IDs reçus dans ce snapshot
    std::set<uint32_t> receivedNetworkIds;
    // receivedNetworkIds.reserve(snapshot.entities.size()); // reserve n'existe pas pour set

    // 2. Traiter chaque entité du snapshot (Mise à jour ou Création)
    for (const auto& entityState : snapshot.entities) {
        // Utilise l'ID réseau SANS CONVERSION (il est déjà dans l'ordre de l'hôte après désérialisation)
        uint32_t netId = entityState.id;
        receivedNetworkIds.insert(netId);

        auto it = networkIdToEntityMap.find(netId);

        if (it != networkIdToEntityMap.end()) {
            // --- CAS 1: Entité déjà connue -> MISE À JOUR ---
            ECS::entity_t localEntity = it->second;

            // Met à jour la position
            auto& positions = m_registry.get_components<Components::Position>();
            if (localEntity < positions.size() && positions[localEntity].has_value()) {
                // TODO: Interpolation/Extrapolation pour lisser le mouvement
                positions[localEntity]->x = entityState.x;
                positions[localEntity]->y = entityState.y;
            }

            // Optionnel : Met à jour la vélocité
            auto& velocities = m_registry.get_components<Components::Velocity>();
             if (localEntity < velocities.size() && velocities[localEntity].has_value()) {
                 velocities[localEntity]->x = entityState.vx;
                 velocities[localEntity]->y = entityState.vy;
             }
            // Optionnel : Mettre à jour XP, Level, Damage si pertinent côté client
            // ...

        } else {
            // --- CAS 2: Nouvelle entité -> CRÉATION ---
            ECS::entity_t newEntity = m_registry.spawn_entity();

            m_registry.emplace_component<Components::NetworkId>(newEntity, netId);
            m_registry.emplace_component<Components::Position>(newEntity, entityState.x, entityState.y);
            m_registry.emplace_component<Components::Velocity>(newEntity, entityState.vx, entityState.vy);
            // Optionnel: emplace_component pour Damage, XP, Level si besoin client

            // Ajoute le composant graphique basé sur le type reçu
            Components::Drawable drawInfo;
            drawInfo.serverEntityType = entityState.type;
            // Définir la taille par défaut ou en fonction du type
            if (entityState.type == 0) { // Joueur
                 drawInfo.width = 50.f; drawInfo.height = 30.f;
             } else { // Ennemi/Balle/PowerUp...
                 drawInfo.width = 30.f; drawInfo.height = 30.f;
             }
            m_registry.emplace_component<Components::Drawable>(newEntity, drawInfo);

   

            // Enregistre le lien
            networkIdToEntityMap[netId] = newEntity;
            // std::cout << "[CLIENT SYNC] Nouvelle entité créée (NetID: " << netId << ", LocalID: " << (size_t)newEntity << ", Type: " << (int)entityState.type << ")" << std::endl;
        }
    }

    // 3. Supprimer les entités qui n'existent plus côté serveur
    for (auto it = networkIdToEntityMap.begin(); it != networkIdToEntityMap.end(); /* no increment */) {
        uint32_t netId = it->first;
        ECS::entity_t localEntity = it->second;

        if (receivedNetworkIds.find(netId) == receivedNetworkIds.end()) {
            // std::cout << "[CLIENT SYNC] Entité détruite (NetID: " << netId << ", LocalID: " << (size_t)localEntity << ")" << std::endl;
            m_registry.kill_entity(localEntity);
            it = networkIdToEntityMap.erase(it); // Supprime de la map et avance
        } else {
            ++it; // Avance seulement si on n'a pas supprimé
        }
    }
    // entitiesPresentInLastSnapshot = receivedNetworkIds; // Met à jour pour la prochaine frame
}

void GameClient::setPlayerEventHandler(const ProtocolData::PlayerEvent& playerEvent) {
    // Gérer les différents types d'événements
    if (playerEvent.type == ProtocolData::PlayerEventType::GAME_OVER) {
        m_gameOver = true;
        std::cout << "[GameClient] Événement GAME_OVER reçu pour le joueur ID: " << playerEvent.playerId << std::endl;
    }
}

// --- initECS Modifié ---
void GameClient::initECS() {
    m_registry.register_component<Position>();
    m_registry.register_component<Velocity>();
    m_registry.register_component<NetworkId>(); // ✅ Important pour la synchro
    m_registry.register_component<Drawable>();  // ✅ Composant graphique

    // NE PLUS CRÉER LE JOUEUR ICI. Il sera créé quand le snapshot arrivera.
    std::cout << "ECS Client initialised." << std::endl;
}

// --- Gestion des Inputs Modifiée ---
void GameClient::processInput() {
    CommandList commands = m_input.getCommands(); // Récupère les commandes
    bool up = false, down = false, left = false, right = false, shoot = false;
    bool hasInput = false; // Pour savoir s'il faut envoyer un paquet

    for (PlayerAction cmd : commands) {
        hasInput = true; // Une touche a été pressée/relâchée
        switch (cmd) {
            case PlayerAction::MOVE_UP:    up = true; break;
            case PlayerAction::MOVE_DOWN:  down = true; break;
            case PlayerAction::MOVE_LEFT:  left = true; break;
            case PlayerAction::MOVE_RIGHT: right = true; break;
            case PlayerAction::SHOOT:      shoot = true; break;
            default: break;
        }
    }

    // Envoyer seulement si une touche pertinente a été actionnée
    // ou peut-être envoyer périodiquement même si rien ne change ?
    ///if (hasInput) { // Ou une condition plus complexe pour éviter le spam
        ProtocolData::PlayerInput inputData{};
        inputData.up = up;
        inputData.down = down;
        inputData.left = left;
        inputData.right = right;
        inputData.shoot = shoot;
        // L'ID joueur n'est pas nécessaire ici, le serveur le connaît via l'endpoint
        m_client.send_input(inputData);
     //}
}

// --- Mise à jour Logique Client (Prédiction/Interpolation) ---
void GameClient::updatePrediction() {
    // C'est ici que tu pourrais implémenter des techniques pour lisser
    // les mouvements entre les snapshots (interpolation) ou pour
    // rendre le joueur local plus réactif (prédiction).
    // Pour l'instant, on peut laisser vide. L'affichage se base
    // directement sur les positions reçues dans updateFromServer.

    // Exemple simple d'interpolation (à améliorer):
    // auto& positions = m_registry.get_components<Position>();
    // auto& velocities = m_registry.get_components<Velocity>();
    // float frameTime = GetFrameTime(); // Temps depuis la dernière frame (Raylib)
    // for (size_t i=0; i < positions.size(); ++i) {
    //      if (positions[i].has_value() && i < velocities.size() && velocities[i].has_value()) {
    //          positions[i]->x += velocities[i]->x * frameTime;
    //          positions[i]->y += velocities[i]->y * frameTime;
    //      }
    // }
}
