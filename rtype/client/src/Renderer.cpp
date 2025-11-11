// Dans Renderer.cpp
#include "../include/client/Renderer.hpp"
#include <algorithm> // Pour std::max
#include <string>    // Pour std::string

Renderer::Renderer(SpriteManager& sm, int screenWidth, int screenHeight)
    : spriteManager(sm), screenWidth(screenWidth), screenHeight(screenHeight) {} // ✅ Stocke les dimensions

// ✅ Implémentation renderEntities (modifiée pour ECS)

void Renderer::renderEntities(const RenderData& data) {
    // --- Rendu des Joueurs ---
    for (const auto& player : data.players) {
        Texture2D sprite = spriteManager.getSprite("player");
        float scale = 0.15f;
        
        if (sprite.id != 0) {
            DrawTextureEx(sprite, 
			  {player.pos.x - (sprite.width * scale / 2.0f), 
			   player.pos.y - (sprite.height * scale / 2.0f)}, 
			  0.0f, scale, WHITE);
        } else {
            DrawRectangle(player.pos.x - 15, player.pos.y - 15, 30, 30, GREEN);
        }
    }
    
    // --- Rendu des Ennemis ---
    for (const auto& enemy : data.enemies) {
        Texture2D sprite = spriteManager.getSprite("enemy");
        float scale = 0.15f;
        
        if (sprite.id != 0) {
            DrawTextureEx(sprite, 
			  {enemy.pos.x - (sprite.width * scale / 2.0f), 
			   enemy.pos.y - (sprite.height * scale / 2.0f)}, 
			  0.0f, scale, WHITE);
        } else {
            DrawRectangle(enemy.pos.x - 15, enemy.pos.y - 15, 30, 30, MAGENTA);
        }
    }
    
    // --- Rendu des Balles ---
    for (const auto& bullet : data.bullets) {
        Texture2D sprite = spriteManager.getSprite("missile");
        float scale = 0.5f;
        
        if (sprite.id != 0) {
            DrawTextureEx(sprite, 
			  {bullet.pos.x - (sprite.width * scale / 2.0f), 
			   bullet.pos.y - (sprite.height * scale / 2.0f)}, 
			  0.0f, scale, WHITE);
        } else {
            DrawRectangle(bullet.pos.x - 5, bullet.pos.y - 5, 10, 10, YELLOW);
        }
    }
    
    // --- Rendu des Power-Ups ---
    for (const auto& orb : data.orbs) {
	Texture2D sprite = spriteManager.getSprite("powerup");
	float scale = 0.005f; // ← très petit

	if (sprite.id != 0) {
	    Vector2 position = {
		orb.pos.x - (sprite.width * scale / 2.0f),
		orb.pos.y - (sprite.height * scale / 2.0f)
	    };

	    DrawTextureEx(sprite, position, 0.0f, scale, WHITE);
	} else {
	    DrawCircle(orb.pos.x, orb.pos.y, 5, GOLD); // cercle plus petit aussi
	}
    }

}

void Renderer::renderLobby(const std::vector<ProtocolData::RoomInfo>& roomList, int selectedIndex) {
    DrawText("LOBBY - Bienvenue !", 100, 50, 40, WHITE);

    // ✅ Boutons d'action
    struct Button {
        const char* label;
        Vector2 position;
    };

    std::vector<Button> buttons = {
        {"Appuyez sur [C] pour Créer une room", {100, 120}},
        {"Appuyez sur [R] pour Rafraîchir la liste", {100, 190}},
        {"Utilisez HAUT/BAS et ENTRÉE pour Rejoindre", {100, 260}}
    };

    for (const auto& btn : buttons) {
        int fontSize = 20;
        int paddingY = 20;
        int paddingX = 80; // ← longueur augmentée

        Vector2 textSize = MeasureTextEx(GetFontDefault(), btn.label, fontSize, 1);
        Rectangle rect = {
            btn.position.x - paddingX,
            btn.position.y - paddingY,
            textSize.x + 2 * paddingX,
            textSize.y + 2 * paddingY
        };

        DrawRectangleRec(rect, Fade(BLUE, 0.3f)); // ← fond moins foncé
        DrawText(btn.label, btn.position.x, btn.position.y, fontSize, WHITE);
    }

    // ✅ Liste des rooms
    int yPos = 340;
    for (size_t i = 0; i < roomList.size(); ++i) {
        const auto& room = roomList[i];

        std::string stateStr;
        Color color;

        switch ((Room::State)room.roomState) {
            case Room::State::WAITING_FOR_PLAYERS: stateStr = "En attente"; color = SKYBLUE; break;
            case Room::State::STARTING: stateStr = "Démarre..."; color = ORANGE; break;
            case Room::State::PLAYING: stateStr = "En cours (joignable)"; color = GREEN; break;
            case Room::State::GAME_OVER:
            case Room::State::FINISHED: stateStr = "Terminée"; color = GRAY; break;
            default: stateStr = "Inconnue"; color = RED;
        }

        std::string text = "Room #" + std::to_string(room.roomId) +
                           " (" + std::to_string(room.playerCount) +
                           "/" + std::to_string(room.maxPlayers) +
                           ") - " + stateStr;

        if (i == selectedIndex) color = YELLOW;

        int fontSize = 20;
        int paddingY = 16;
        int paddingX = 60; // ← longueur augmentée
        Vector2 textSize = MeasureTextEx(GetFontDefault(), text.c_str(), fontSize, 1);
        Rectangle rect = {
            115.0f - paddingX, (float)yPos - paddingY,
            textSize.x + 2 * paddingX,
            textSize.y + 2 * paddingY
        };

        DrawRectangleRec(rect, Fade(DARKGRAY, 0.35f)); // ← fond moins foncé
        DrawText(text.c_str(), 120, yPos, fontSize, color);
        yPos += 50;
    }
}




void Renderer::renderWaitingRoom(std::optional<uint32_t> roomId, const std::vector<std::string>& playerNames) {
    std::string text = "Room #" + (roomId.has_value() ? std::to_string(*roomId) : "...");
    DrawText(text.c_str(), 100, 200, 30, WHITE);
    DrawText("En attente d'autres joueurs...", 100, 240, 20, GRAY);
    DrawText("Appuyez sur [Q] pour Quitter", 100, 270, 20, GRAY);
    // Optionnel : DrawText("Appuyez sur [ESPACE] pour démarrer (Host)", 100, 300, 20, GRAY);

    int yPos = 350;
    DrawText("Joueurs présents:", 100, yPos, 20, WHITE);
    yPos += 30;
    for (const auto& name : playerNames) {
        DrawText(name.c_str(), 120, yPos, 20, LIGHTGRAY);
        yPos += 25;
    }
}
void Renderer::renderPlayerHUD(const GameClient::PlayerLocalStats& stats)
{
    // Barre de santé
    float healthBarWidth = 200.0f;
    float healthBarHeight = 20.0f;
    float healthRatio = static_cast<float>(stats.health) / static_cast<float>(stats.maxHealth);
    DrawRectangle(10, screenHeight - 40, healthBarWidth, healthBarHeight, DARKGRAY);
    DrawRectangle(10, screenHeight - 40, healthBarWidth * healthRatio, healthBarHeight, RED);
    DrawText(("HP: " + std::to_string(stats.health) + "/" + std::to_string(stats.maxHealth)).c_str(), 15, screenHeight - 38, 14, WHITE);

    // Niveau et XP
    DrawText(("Niveau: " + std::to_string(stats.level)).c_str(), 10, screenHeight - 70, 14, WHITE);
    DrawText(("XP: " + std::to_string(stats.xp) + "/" + std::to_string(stats.xpForNextLevel)).c_str(), 100, screenHeight - 70, 14, WHITE);

    // Arme équipée
    DrawText(("Arme: " + stats.weaponName).c_str(), 10, screenHeight - 100, 14, WHITE);
}
void Renderer::renderGameOverScreen() {
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f)); // Assombrir
    DrawText("GAME OVER", screenWidth / 2 - MeasureText("GAME OVER", 60) / 2, screenHeight / 2 - 30, 60, RED);
    DrawText("Appuyez sur [R] pour retourner au Lobby", screenWidth / 2 - MeasureText("Appuyez sur [R] pour retourner au Lobby", 20) / 2, screenHeight / 2 + 40, 20, GRAY);
}

void Renderer::renderNicknameEntry(const std::string& currentNickname, 
                                    const std::string& errorMessage) {
    // Titre
    const char* title = "R-TYPE MULTIPLAYER";
    int titleWidth = MeasureText(title, 40);
    DrawText(title, (screenWidth - titleWidth) / 2, 150, 40, WHITE);
    
    // Instructions
    const char* instruction = "Entrez votre pseudo (3-20 caractères):";
    int instrWidth = MeasureText(instruction, 20);
    DrawText(instruction, (screenWidth - instrWidth) / 2, 220, 20, LIGHTGRAY);
    
    // Champ de saisie
    int boxWidth = 400;
    int boxHeight = 50;
    int boxX = (screenWidth - boxWidth) / 2;
    int boxY = 270;
    
    // Fond du champ
    DrawRectangle(boxX, boxY, boxWidth, boxHeight, Fade(WHITE, 0.1f));
    DrawRectangleLines(boxX, boxY, boxWidth, boxHeight, WHITE);
    
    // Texte saisi
    std::string displayText = currentNickname + "_";  // Curseur clignotant
    DrawText(displayText.c_str(), boxX + 10, boxY + 12, 24, WHITE);
    
    // Aide
    const char* help = "Appuyez sur ENTRÉE pour valider";
    int helpWidth = MeasureText(help, 16);
    DrawText(help, (screenWidth - helpWidth) / 2, boxY + 70, 16, GRAY);
    
    // Message d'erreur (si présent)
    if (!errorMessage.empty()) {
        int errorWidth = MeasureText(errorMessage.c_str(), 18);
        DrawText(errorMessage.c_str(), (screenWidth - errorWidth) / 2, boxY + 110, 18, RED);
    }
}
void Renderer::renderPlayerName(const std::string& nickname) {
    if (nickname.empty()) return;  // N'affiche rien si pas de pseudo
    
    // Position en haut à gauche
    int x = 10;
    int y = 10;
    
    // Texte avec ombre pour meilleure lisibilité
    DrawText(nickname.c_str(), x + 2, y + 2, 24, BLACK);  // Ombre
    DrawText(nickname.c_str(), x, y, 24, YELLOW);         // Texte
}

void Renderer::renderRoomCreationScreen(const ProtocolData::RoomConfig& config, int selectedOption) {
    // Fond semi-transparent
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.3f));
    
    // Panneau central
    int panelX = screenWidth / 2 - 300;
    int panelY = 50;
    int panelWidth = 600;
    int panelHeight = 650;
    
    DrawRectangle(panelX, panelY, panelWidth, panelHeight, Fade(DARKGRAY, 0.9f));
    DrawRectangleLines(panelX, panelY, panelWidth, panelHeight, YELLOW);
    
    // Titre
    const char* title = "CRÉER UNE PARTIE";
    int titleWidth = MeasureText(title, 30);
    DrawText(title, screenWidth/2 - titleWidth/2, panelY + 20, 30, YELLOW);
    
    int yPos = panelY + 80;
    int leftMargin = panelX + 30;
    
    // ═══════════════════════════════════════════════════════
    // Option 0 : NOM DE LA ROOM
    // ═══════════════════════════════════════════════════════
    if (selectedOption == 0) DrawText(">", leftMargin - 20, yPos, 20, YELLOW);
    DrawText("Nom de la room:", leftMargin, yPos, 20, WHITE);
    yPos += 25;
    
    DrawRectangleLines(leftMargin, yPos, 540, 30, selectedOption == 0 ? YELLOW : GRAY);
    DrawText(config.roomName, leftMargin + 5, yPos + 5, 20, LIGHTGRAY);
    if (selectedOption == 0 && ((int)(GetTime() * 2) % 2 == 0)) {
        DrawText("_", leftMargin + 5 + MeasureText(config.roomName, 20), yPos + 5, 20, YELLOW);
    }
    yPos += 50;
    
    // ═══════════════════════════════════════════════════════
    // Option 1 : DIFFICULTÉ
    // ═══════════════════════════════════════════════════════
    if (selectedOption == 1) DrawText(">", leftMargin - 20, yPos, 20, YELLOW);
    DrawText("Difficulté:", leftMargin, yPos, 20, WHITE);
    yPos += 25;
    
    const char* difficulties[] = {"Facile", "Normal", "Difficile"};
    for (int i = 0; i < 3; i++) {
        Color color = (config.difficulty == i) ? YELLOW : GRAY;
        if (selectedOption == 1 && config.difficulty == i) color = ORANGE;
        DrawText(difficulties[i], leftMargin + i*180, yPos, 20, color);
        if (config.difficulty == i) {
            DrawText("●", leftMargin + i*180 - 20, yPos, 20, YELLOW);
        }
    }   
    yPos += 50;
    
    // ═══════════════════════════════════════════════════════
    // Option 2 : MAX JOUEURS
    // ═══════════════════════════════════════════════════════
    if (selectedOption == 2) DrawText(">", leftMargin - 20, yPos, 20, YELLOW);
    DrawText("Joueurs max:", leftMargin, yPos, 20, WHITE);
    yPos += 25;
    
    for (int i = 2; i <= 6; i++) {
        Color color = (config.maxPlayers == i) ? YELLOW : GRAY;
        if (selectedOption == 2 && config.maxPlayers == i) color = ORANGE;
        
        DrawRectangle(leftMargin + (i-2)*90 + 10, yPos, 50, 30, 
                      config.maxPlayers == i ? Fade(YELLOW, 0.3f) : Fade(GRAY, 0.1f));
        DrawRectangleLines(leftMargin + (i-2)*90 + 10, yPos, 50, 30, color);
        DrawText(TextFormat("%d", i), leftMargin + (i-2)*90 + 28, yPos + 5, 20, color);
    }
    yPos += 50;
    
    // ═══════════════════════════════════════════════════════
    // Option 3 : VITESSE ENNEMIS
    // ═══════════════════════════════════════════════════════
    if (selectedOption == 3) DrawText(">", leftMargin - 20, yPos, 20, YELLOW);
    DrawText("Vitesse des ennemis:", leftMargin, yPos, 20, WHITE);
    DrawText(TextFormat("%d%%", config.enemySpeedMultiplier), leftMargin + 450, yPos, 20, YELLOW);
    yPos += 25;
    
    int barWidth = 400;
    int barFilled = (config.enemySpeedMultiplier - 50) * barWidth / 100;  // 50-150 → 0-400px
    DrawRectangle(leftMargin, yPos, barFilled, 20, YELLOW);
    DrawRectangleLines(leftMargin, yPos, barWidth, 20, selectedOption == 3 ? YELLOW : GRAY);
    yPos += 40;
    
    // ═══════════════════════════════════════════════════════
    // Option 4 : SPAWN RATE
    // ═══════════════════════════════════════════════════════
    if (selectedOption == 4) DrawText(">", leftMargin - 20, yPos, 20, YELLOW);
    DrawText("Spawn rate:", leftMargin, yPos, 20, WHITE);
    DrawText(TextFormat("%d%%", config.spawnRateMultiplier), leftMargin + 450, yPos, 20, YELLOW);
    yPos += 25;
    
    barFilled = (config.spawnRateMultiplier - 50) * barWidth / 150;  // 50-200 → 0-400px
    DrawRectangle(leftMargin, yPos, barFilled, 20, YELLOW);
    DrawRectangleLines(leftMargin, yPos, barWidth, 20, selectedOption == 4 ? YELLOW : GRAY);
    yPos += 40;
    
    // ═══════════════════════════════════════════════════════
    // Option 5 : TIR AMI
    // ═══════════════════════════════════════════════════════
    if (selectedOption == 5) DrawText(">", leftMargin - 20, yPos, 20, YELLOW);
    DrawText(config.friendlyFire ? "☑" : "☐", leftMargin, yPos, 24, 
             selectedOption == 5 ? YELLOW : WHITE);
    DrawText("Tir ami activé", leftMargin + 35, yPos, 20, WHITE);
    yPos += 35;
    
    // ═══════════════════════════════════════════════════════
    // Option 6 : POWER-UPS
    // ═══════════════════════════════════════════════════════
    if (selectedOption == 6) DrawText(">", leftMargin - 20, yPos, 20, YELLOW);
    DrawText(config.powerUpsEnabled ? "☑" : "☐", leftMargin, yPos, 24, 
             selectedOption == 6 ? YELLOW : WHITE);
    DrawText("Power-ups activés", leftMargin + 35, yPos, 20, WHITE);
    yPos += 50;
    
    // ═══════════════════════════════════════════════════════
    // Options 7-8 : BOUTONS
    // ═══════════════════════════════════════════════════════
    int buttonY = panelY + panelHeight - 70;
    
    // Annuler
    Color cancelColor = (selectedOption == 7) ? YELLOW : GRAY;
    DrawRectangleLines(leftMargin + 50, buttonY, 180, 40, cancelColor);
    DrawText("ANNULER", leftMargin + 90, buttonY + 10, 20, cancelColor);
    
    // Créer
    Color createColor = (selectedOption == 8) ? YELLOW : WHITE;
    DrawRectangle(leftMargin + 300, buttonY, 180, 40, 
                  selectedOption == 8 ? Fade(YELLOW, 0.3f) : Fade(DARKGRAY, 0.5f));
    DrawRectangleLines(leftMargin + 300, buttonY, 180, 40, createColor);
    DrawText("CRÉER", leftMargin + 350, buttonY + 10, 20, createColor);
    
    // Aide en bas
    DrawText("↑↓ Naviguer  ←→ Modifier  ENTRÉE Valider  ÉCHAP Annuler", 
             screenWidth/2 - 250, screenHeight - 30, 16, GRAY);
}



void Renderer::renderNetworkStats(const GameClient::NetworkStats& stats, float fps) {
    int x = 10;  // ✅ Gauche (pas droite)
    int y = 10;  // ✅ Haut
    int padding = 10;
    int lineHeight = 20;
    int boxWidth = 240;
    int boxHeight = 120;
    
    // ═══════════════════════════════════════════════════
    // Fond semi-transparent avec bordure
    // ═══════════════════════════════════════════════════
    DrawRectangle(x, y, boxWidth, boxHeight, Fade(BLACK, 0.75f));
    DrawRectangleLines(x, y, boxWidth, boxHeight, Fade(WHITE, 0.5f));
    
    int textX = x + padding;
    int textY = y + padding;
    
    // ═══════════════════════════════════════════════════
    // Titre
    // ═══════════════════════════════════════════════════
    DrawText("NETWORK DEBUG", textX, textY, 14, YELLOW);
    textY += lineHeight + 5;
    
    // ═══════════════════════════════════════════════════
    // FPS (avec couleur conditionnelle)
    // ═══════════════════════════════════════════════════
    Color fpsColor = GREEN;
    if (fps < 60) fpsColor = ORANGE;
    if (fps < 30) fpsColor = RED;
    
    DrawText(TextFormat("FPS: %d", (int)fps), textX, textY, 12, fpsColor);
    textY += lineHeight;
    
    // ═══════════════════════════════════════════════════
    // Entités affichées
    // ═══════════════════════════════════════════════════
    Color entColor = WHITE;
    if (stats.entitiesCount >= 35) entColor = ORANGE;
    if (stats.entitiesCount >= 40) entColor = RED;
    
    DrawText(TextFormat("Entities: %d", stats.entitiesCount), textX, textY, 12, entColor);
    textY += lineHeight;
    
    // ═══════════════════════════════════════════════════
    // Bande passante instantanée
    // ═══════════════════════════════════════════════════
    Color bwColor = GREEN;
    if (stats.currentKbps > 50.0f) bwColor = ORANGE;
    if (stats.currentKbps > 100.0f) bwColor = RED;
    
    DrawText(TextFormat("BW: %.1f Ko/s", stats.currentKbps), textX, textY, 12, bwColor);
    textY += lineHeight;
    
    // ═══════════════════════════════════════════════════
    // Paquets reçus (total)
    // ═══════════════════════════════════════════════════
    DrawText(TextFormat("Packets: %zu", stats.packetsReceived), textX, textY, 12, DARKGRAY);
}