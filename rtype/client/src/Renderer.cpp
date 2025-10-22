// Dans Renderer.cpp
#include "../include/client/Renderer.hpp"
#include <algorithm> // Pour std::max
#include <string>    // Pour std::string
#include "../include/ecs"
Renderer::Renderer(SpriteManager& sm, int screenWidth, int screenHeight)
    : spriteManager(sm), screenWidth(screenWidth), screenHeight(screenHeight) {}

// void Renderer::renderBackground() { /* Plus utile si fait dans Game::render */ }

// ✅ MODIFIÉ : Lit depuis la registry
void Renderer::renderEntities(const ECS::registry& registry) {
    // Récupère les composants nécessaires DEPUIS la registry (en const)
    const auto& positions = registry.get_components<Components::Position>();
    const auto& drawables = registry.get_components<Components::Drawable>(); // Le composant graphique

    size_t max_entities = std::max(positions.size(), drawables.size());

    for (size_t i = 0; i < max_entities; ++i) {
        // Si l'entité a une Position ET un Drawable
        if (i < positions.size() && positions[i].has_value() &&
            i < drawables.size() && drawables[i].has_value())
        {
            const auto& pos = positions[i].value();
            const auto& drawInfo = drawables[i].value();

            // Détermine quel sprite utiliser basé sur le type serveur stocké dans Drawable
            std::string spriteName = "unknown"; // Nom du sprite dans SpriteManager
            float scale = 0.15f; // Échelle par défaut

            // TODO: Adapter cette logique à tes conventions de type serveur
            if (drawInfo.serverEntityType == 0) { // Joueur
                spriteName = "player";
                scale = 0.2f;
            } else if (drawInfo.serverEntityType >= 10 && drawInfo.serverEntityType < 100) { // Ennemi
                // Idéalement, différencier les types d'ennemis
                if (drawInfo.serverEntityType == 10 + Components::Grubs) spriteName = "enemy"; // Exemple
                // else if (drawInfo.serverEntityType == 10 + Components::Flyers) spriteName = "enemy_flyer";
                else spriteName = "enemy"; // Fallback ennemi générique
            } else if (drawInfo.serverEntityType >= 100 && drawInfo.serverEntityType < 200) { // Balle
                spriteName = "missile";
                scale = 0.1f;
            } else if (drawInfo.serverEntityType >= 200) { // PowerUp
                spriteName = "powerup"; // Nom de sprite à définir/charger
                 scale = 0.1f;
             }

            // Récupère la texture depuis SpriteManager
            Texture2D sprite = spriteManager.getSprite(spriteName);

            // Dessine si la texture est valide
            if (sprite.id != 0) {
                 // Dessine la texture centrée sur la position (x, y)
                 DrawTextureEx(sprite,
                               {pos.x - (sprite.width * scale / 2.0f), pos.y - (sprite.height * scale / 2.0f)},
                               0.0f, // Rotation
                               scale, // Échelle
                               WHITE); // Teinte
            } else {
                 // Optionnel : Dessine un rectangle si sprite inconnu (pour débug)
                 Color color = GRAY;
                 if(spriteName == "player") color = GREEN; else if(spriteName == "enemy") color = MAGENTA; else if(spriteName == "missile") color = YELLOW;
                 DrawRectangleRec({pos.x - drawInfo.width / 2, pos.y - drawInfo.height / 2, drawInfo.width, drawInfo.height}, color);
            }
        }
    }
}