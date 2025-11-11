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

class RTypeClient; // Déclaration anticipée

// Structure consolidée qui regroupe TOUTES les données à afficher
struct RenderData
{
    std::vector<playerInfo> players;
    std::vector<EnemyInfo> enemies;
    std::vector<BulletInfo> bullets;
    std::vector<OrbsInfo> orbs;

    // Méthode helper pour vider toutes les listes
    void clear()
    {
        players.clear();
        enemies.clear();
        bullets.clear();
        orbs.clear();
    }

    // Méthode helper pour vérifier si vide
    bool isEmpty() const
    {
        return players.empty() && enemies.empty() &&
               bullets.empty() && orbs.empty();
    }
};

class GameClient
{
public:
    // ✅ NOUVEAU : États du client
    enum class State
    {
        CONNECTING,
        ENTERING_NICKNAME,
        LOBBY,
        CREATING_ROOM,
        WAITING_IN_ROOM,
        PLAYING,
        GAME_OVER
    };

    GameClient(InputManager &input, RTypeClient &networkClient);
    // ✅ NOUVELLE MÉTHODE : Retourne les données de rendu depuis l'ECS client
    RenderData getRenderData() const;

    // ✅ Stats du joueur local
    struct PlayerLocalStats
    {
        int health = 0;
        int maxHealth = 100;
        int xp = 0;
        int level = 1;
        int xpForNextLevel = 100;
        std::string weaponName = "Standard Shot";
    };

    // ✅ NOUVEAU : Structure stats réseau
    struct NetworkStats {
        size_t bytesReceived = 0;
        size_t packetsReceived = 0;
        int entitiesCount = 0;
        float lastUpdateTime = 0.0f;
        
        // Pour calcul bande passante instantanée
        size_t lastBytesSnapshot = 0;
        float lastCalcTime = 0.0f;
        float currentKbps = 0.0f;
    };
       const NetworkStats& getNetworkStats() const { return m_networkStats; }
    void updateNetworkStats(); // Appelée chaque frame

    PlayerLocalStats getLocalPlayerStats() const;

    void setTemporaryNickname(const std::string &nickname) { m_tempNickname = nickname; }
    std::string getTemporaryNickname() const { return m_tempNickname; }
    std::string getNicknameRejectionReason() { return m_nicknameRejectionReason; }
    void clearNicknameRejectionReason() { m_nicknameRejectionReason = ""; }

    // --- Méthodes pour la boucle principale ---
    void processInput(); // ✅ Prend l'index de sélection du lobby
    void updatePrediction();

    std::string getConfirmedNickname() const { return m_confirmedNickname; }
    // --- Accesseurs pour le Rendu et la Logique ---
    const ECS::registry &getRegistry() const { return m_registry; }
    State getState() const { return m_state.load(); }
    const std::vector<ProtocolData::RoomInfo> &getRoomList() const { return m_roomList; }
    std::optional<uint32_t> getCurrentRoomId() const { return m_currentRoomId; }
    std::vector<std::string> getPlayerNicknamesInRoom() const; // Pour afficher les joueurs en attente
    int getPlayerScore() const;                                // Pour le HUD

    // --- Méthodes appelées par RTypeClient (callbacks) ---
    void updateFromServer(const ProtocolData::Snapshot &snapshot);
    void setLocalPlayerId(uint32_t networkId);
    void handleRoomList(const ProtocolData::RoomList &list);
    void handleRoomResponse(ProtocolData::MessageType type, const ProtocolData::RoomResponse &response);
    void handlePlayerNotification(ProtocolData::MessageType type, const ProtocolData::PlayerRoomNotification &notif);
    void handlePlayerEvent(const ProtocolData::PlayerEvent &event); // Pour GAME_OVER
    void handleGameStarting();
    void handleWelcome(uint32_t playerId, const ProtocolData::Welcome &welcomeData);

    // --- Actions demandées par Game::handleInput ---
    void requestListRooms();
    void requestCreateRoom();
    void requestJoinRoom(uint32_t roomId);
    void requestLeaveRoom();       // ✅ NOUVEAU
    void requestStartGame();       // ✅ NOUVEAU (si le joueur 1 démarre)
    void setState(State newState); // ✅ Pour revenir au lobby après GAME_OVER
    void cleanupGameEntities();    // Pour vider l'ECS après une partie

    std::string m_nicknameRejectionReason;

    // ✅ NOUVEAU : Gestion de la config de room
    void setRoomConfig(const ProtocolData::RoomConfig &config) { m_roomConfig = config; }
    ProtocolData::RoomConfig &getRoomConfig() { return m_roomConfig; }
    const ProtocolData::RoomConfig &getRoomConfig() const { return m_roomConfig; }

    // ✅ NOUVEAU : Initialise une config par défaut
    void initDefaultRoomConfig();
    void applyDeltaSnapshot(const ProtocolData::DeltaSnapshot& delta);
private:
    void initECS();

    InputManager &m_input;
    RTypeClient &m_client;
    ECS::registry m_registry; // ECS Locale

    // Outils de synchronisation
    std::unordered_map<uint32_t, ECS::entity_t> networkIdToEntityMap;
    std::optional<uint32_t> m_localPlayerNetworkId;
    std::optional<ECS::entity_t> m_player_entity;

    // État du Lobby
    std::atomic<State> m_state{State::CONNECTING};
    std::vector<ProtocolData::RoomInfo> m_roomList;
    std::optional<uint32_t> m_currentRoomId;
    std::map<uint32_t, std::string> m_playersInRoom; // Stocke les joueurs dans la room (ID -> Pseudo)
    int m_playerScore = 0;
    std::string m_tempNickname; // ✅ NOUVEAU
    std::string m_confirmedNickname;
    NetworkStats m_networkStats;
    ProtocolData::RoomConfig m_roomConfig;  // ✅ NOUVEAU
        // ✅ NOUVEAU : Map pour retrouver entités locales depuis networkId serveur
    
    // Helpers privés
    void createEntityFromState(const ProtocolData::entity_state& state);
    void updateEntityFromState(uint32_t networkId, const ProtocolData::entity_state& state);
    void destroyEntityByNetworkId(uint32_t networkId);

};