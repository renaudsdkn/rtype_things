// Dans Game.cpp
#include "../include/client/Game.hpp"
#include <iostream> // Pour les logs de debug

// --- Constructeur ---
Game::Game(int screenWidth, int screenHeight)
    : screenWidth(screenWidth), screenHeight(screenHeight),
      // Initialise les composants graphiques
      spriteManager(),
      renderer(spriteManager, screenWidth, screenHeight), // Passe SpriteManager au Renderer
      starfield(screenWidth, screenHeight),
      // ✅ Initialise les nouveaux composants
      inputManager(),
      networkClient("127.0.0.1", 1234), // IP & Port serveur
      // Passe InputManager et NetworkClient à GameClient
      gameClient(inputManager, networkClient)
{
    InitWindow(screenWidth, screenHeight, "R-Type - Client Réseau"); // Initialise Raylib
    InitAudioDevice(); // Pour Raylib (si son)
    SetTargetFPS(60);
    spriteManager.loadAssets(); // Charge les sprites

    // ✅ Lier RTypeClient (réseau) à GameClient (logique) via callbacks
    networkClient.setSnapshotHandler(
        // Utilise une lambda qui capture 'this' (ou 'gameClient' par référence)
        [this](const ProtocolData::Snapshot& snap){
            this->gameClient.updateFromServer(snap); // Appelle la synchro
        }
    );
    networkClient.setWelcomeHandler(
        [this](uint32_t id){
            this->gameClient.setLocalPlayerId(id); // Informe GameClient de son ID
        }
    );
    networkClient.setPlayerEventHandler(
        [this](const ProtocolData::PlayerEvent& event){
            this->gameClient.setPlayerEventHandler(event); // Informe GameClient de l'événement
        }
    );
    // Démarrer le client réseau (thread de réception)
    networkClient.start();
    std::cout << "[Game] Client réseau démarré." << std::endl;
}

// --- Destructeur ---
Game::~Game() {
    // networkClient.stop() sera appelé après la sortie de run()
    spriteManager.unload();
    CloseAudioDevice();
    CloseWindow(); // Ferme Raylib
}

// --- Boucle Principale ---
void Game::run() {
    std::cout << "[Game] Démarrage de la boucle principale..." << std::endl;
    while (!WindowShouldClose()) { // Boucle tant que la fenêtre Raylib est ouverte

        // --- Gérer les états MENU / GAME_OVER ---
        // TODO: Adapter cette logique. Pour l'instant, on suppose qu'on est toujours en jeu.
        //       Tu pourrais ajouter un état dans GameClient ou ici.
        // if (/* état menu ? */) { /* ... Afficher menu ... */ continue; }
        // if (/* état game over ? */) { /* ... Afficher game over ... */ continue; }
        // --- FIN GESTION ÉTATS ---
        // Gérer les inputs locaux et les envoyer au serveur
        handleInput();

        // Mettre à jour la logique client (interpolation/prédiction)
        updateGameLogic();

        // Afficher la scène
        render();
    }
    std::cout << "[Game] Fin de la boucle principale." << std::endl;
    // Envoyer Disconnect avant d'arrêter le réseau
    networkClient.send_disconnect();
    networkClient.stop(); // Arrête le thread réseau proprement
}

// --- Gestion des Inputs (Modifié) ---
void Game::handleInput() {
    // Demande simplement à GameClient de lire les inputs via InputManager
    // et de les envoyer au serveur via RTypeClient.
    gameClient.processInput();
}

// --- Mise à Jour Logique (Modifié) ---
void Game::updateGameLogic() {
    // Demande à GameClient d'appliquer la logique côté client
    // (interpolation des positions, prédiction du joueur local...).
    gameClient.updatePrediction();

    // ❌ SUPPRIMÉ : Toute la logique de GameState::update (spawn local, collisions locales...)
    //               Le serveur s'en charge maintenant.
}

// --- Rendu (Modifié) ---
void Game::render() {
    BeginDrawing();
    ClearBackground(BLACK); // Fond noir

    // 1. Dessiner le fond étoilé (inchangé)
    starfield.updateAndDraw();
    // 2. ✅ Demander au Renderer de dessiner les entités
    //    en lisant l'état actuel de l'ECS DANS GameClient
    renderer.renderEntities(gameClient.getRegistry()); // Passe la registry à jour

    // 3. Dessiner l'interface utilisateur (HUD) - Exemple
    //    (Récupérer les infos depuis GameClient ou sa registry si nécessaire)
    // DrawText(("Vies: ...").c_str(), 20, 20, 30, RED);
    // DrawText(("Score: ...").c_str(), 20, 60, 30, WHITE);

    // Gérer l'affichage GAME OVER si besoin
    if (gameClient.isGameOver()) { DrawText("GAME OVER", 400, 300, 50, RED); }

    EndDrawing();
}