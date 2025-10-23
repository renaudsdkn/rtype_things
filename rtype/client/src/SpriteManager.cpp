#include "../include/client/SpriteManager.hpp"
// Dans rtype/client/src/SpriteManager.cpp

#include <iostream> // Pour les erreurs

void SpriteManager::loadAssets() {
    // Chemin relatif DEPUIS build/rtype/client/ VERS rtype/client/assets/
  // Dans SpriteManager::loadAssets()
const std::string basePath = "rtype/client/assets/"; // ✅ Chemin depuis la racine

sprites["player"] = LoadTexture((basePath + "ship.png").c_str());
sprites["enemy"] = LoadTexture((basePath + "enemy.png").c_str());
sprites["missile"] = LoadTexture((basePath + "fireball.png").c_str());
// ... (avec vérifications d'erreur)
    // Ajoute le chargement des autres sprites nécessaires
    // sprites["powerup"] = LoadTexture((basePath + "powerup.png").c_str()); // Exemple

    // --- Vérification INDISPENSABLE ---
    if (sprites["player"].id == 0) std::cerr << "ERREUR: Chargement échoué pour " << (basePath + "ship.png") << std::endl;
    if (sprites["enemy"].id == 0) std::cerr << "ERREUR: Chargement échoué pour " << (basePath + "enemy.png") << std::endl;
    if (sprites["missile"].id == 0) std::cerr << "ERREUR: Chargement échoué pour " << (basePath + "fireball.png") << std::endl;
    // ... vérifier les autres ...

    std::cout << "[SpriteManager] Assets chargés (espérons-le)." << std::endl;
}

// Reste de SpriteManager.cpp (getSprite, unload)
Texture2D SpriteManager::getSprite(const std::string& name) const {
    auto it = sprites.find(name);
    if (it != sprites.end()) {
        return it->second;
    }
    std::cerr << "[SpriteManager] Attention: Sprite '" << name << "' non trouvé!" << std::endl;
    return Texture2D{0}; // Retourne une texture invalide
}

void SpriteManager::unload() {
    for (auto& pair : sprites) {
        if (pair.second.id != 0) UnloadTexture(pair.second);
    }
    sprites.clear();
    std::cout << "[SpriteManager] Assets déchargés." << std::endl;
}