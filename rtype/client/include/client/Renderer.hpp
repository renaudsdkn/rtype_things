// Dans Renderer.hpp
#pragma once
#include "raylib.h"
#include "SpriteManager.hpp"
// #include "GameState.hpp" // SUPPRIMÉ
#include "../include/ecs/engine.hpp" // Pour ECS::registry
class Renderer {
public:
    Renderer(SpriteManager& sm, int screenWidth, int screenHeight);
    // renderBackground peut rester s'il fait plus que ClearBackground
    // void renderBackground(); // Probablement plus utile ici
    // ✅ MODIFIÉ : Prend la registry CONSTANTE en paramètre
    void renderEntities(const ECS::registry& registry);

private:
    SpriteManager& spriteManager;
    // Garde screenWidth/Height si nécessaire
    int screenWidth;
    int screenHeight;
};