#pragma once

#include "GraphicsEngine.hpp"
#include "InputManager.hpp"
#include "client.hpp" // ✅ Renommé depuis UdpClient.hpp ? Contient RTypeClient
#include "../include/ecs/engine.hpp" 
#include "protocol/protocol_data.hpp" // Pour Snapshot
#include <unordered_map>              // Pour la map
#include <set>                        // Pour le set d'IDs
#include <optional>                   // Pour l'ID joueur optionnel
// Dans rtype/client/include/client/GameClient.hpp
#pragma once
// ... (includes: InputManager, ecs.hpp, protocol_data.hpp, etc.)

// Déclaration anticipée
class RTypeClient;

class GameClient {
public:
    // ✅ Le constructeur prend InputManager ET RTypeClient
    GameClient(InputManager& input, RTypeClient& networkClient);

    // Méthodes appelées par main
    void processInput();
    void updatePrediction();
    bool isGameOver() const { return m_gameOver; }
    // Méthodes appelées VIA CALLBACKS depuis RTypeClient
    void updateFromServer(const ProtocolData::Snapshot& snapshot);
    void setLocalPlayerId(uint32_t networkId);
    void setPlayerEventHandler(const ProtocolData::PlayerEvent& playerEvent); // ✅ Gérer les événements joueur

    // Accesseur pour le rendu
    const ECS::registry& getRegistry() const { return m_registry; }
 
private:
    void initECS();

    InputManager& m_input;
    // ✅ GARDE la référence à RTypeClient pour pouvoir envoyer
    RTypeClient& m_client;
    bool m_gameOver = false;
    ECS::registry m_registry;
    std::unordered_map<uint32_t, ECS::entity_t> networkIdToEntityMap;
    std::optional<uint32_t> m_localPlayerNetworkId;
    std::optional<ECS::entity_t> m_player_entity;
};