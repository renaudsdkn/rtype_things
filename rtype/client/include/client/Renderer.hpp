// Dans Renderer.hpp
#pragma once

#include "SpriteManager.hpp"
// #include "GameState.hpp" // SUPPRIMÉ
#include "../include/ecs/engine.hpp" // Pour ECS::registry
#include "protocol/protocol_data.hpp" // Pour RoomInfo
#include "../include/server/room.hpp" // SUPPRIMÉ
#include "GameClient.hpp" // Pour GameClient::PlayerStats
class Renderer {
public:
    Renderer(SpriteManager& sm, int screenWidth, int screenHeight);
    // ✅ MODIFIÉ : Prend la registry CONSTANTE en paramètre
    void renderEntities(const RenderData& data);
    // ✅ NOUVEAU
    void renderLobby(const std::vector<ProtocolData::RoomInfo>& roomList, int selectedIndex);
    void renderWaitingRoom(std::optional<uint32_t> roomId, const std::vector<std::string>& playerNames);
    void renderGameOverScreen();
    void renderPlayerHUD(const GameClient::PlayerLocalStats& stats);
     // ✅ NOUVEAU
    void renderNicknameEntry(const std::string& currentNickname,const std::string& errorMessage = "");
    void renderRoomCreationScreen(const ProtocolData::RoomConfig& config, int selectedOption);
    void renderPlayerName(const std::string& nickname);
    void renderNetworkStats(const GameClient::NetworkStats& stats, float fps);

private:
    SpriteManager& spriteManager;
    int screenWidth; // Garde les dimensions
    int screenHeight;
};