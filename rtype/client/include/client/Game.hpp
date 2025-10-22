// Dans Game.hpp
#pragma once

#include "raylib.h"
#include "SpriteManager.hpp"
#include "Renderer.hpp"
// #include "GameState.hpp" // SUPPRIMÉ
#include "Starfield.hpp"
// ✅ NOUVEAUX INCLUDES
#include "client.hpp"       // Pour RTypeClient (réseau)
#include "InputManager.hpp" // Pour InputManager
#include "GameClient.hpp"   // Pour GameClient (logique + état ECS)

class Game {
public:
    Game(int screenWidth, int screenHeight);
    ~Game();
    void run(); // Boucle principale

private:
    // Garde les méthodes de la boucle, le contenu changera
    void handleInput();
    void updateGameLogic(); // Renommé depuis update
    void render();

    int screenWidth;
    int screenHeight;

    // --- Composants Graphiques (conservés) ---
    SpriteManager spriteManager;
    Renderer renderer;
    Starfield starfield;

    // --- ✅ NOUVEAUX Composants Logique & Réseau ---
    InputManager inputManager;
    RTypeClient networkClient;
    GameClient gameClient; // Gère l'ECS et la synchro

    // ❌ SUPPRIMÉ : GameState game;
    // ❌ SUPPRIMÉ : Texture2D missileSprite, enemySprite (sera géré par Renderer/SpriteManager)
};