// Dans rtype/client/src/main.cpp
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <thread>

#include "../include/client/GameClient.hpp"
#include "../include/client/InputManager.hpp"
#include "../include/client/client.hpp" // RTypeClient

// --- PAS de GraphicsEngine ni de renderGame ---

int main(void)
{
    try {
        std::cout << "[CLIENT MAIN] Initialisation..." << std::endl;

        // 1. Créer les objets
        InputManager input;
        RTypeClient networkClient("127.0.0.1", 1234); // Constructeur simple
        // GameClient prend RTypeClient en référence
        GameClient gameClient(input, networkClient);

        // 2. Lier RTypeClient -> GameClient via callbacks
        networkClient.setSnapshotHandler(
            // La lambda capture gameClient par référence
            [&gameClient](const ProtocolData::Snapshot& snap){
                gameClient.updateFromServer(snap); // Appelle la méthode
            }
        );
        networkClient.setWelcomeHandler(
            [&gameClient](uint32_t id){
                gameClient.setLocalPlayerId(id); // Appelle la méthode
            }
        );

        // 3. Démarrer le réseau (lance le thread de réception)
        networkClient.start();
        std::cout << "[CLIENT MAIN] Démarrage boucle principale (logique)..." << std::endl;

        // 4. Boucle principale
        bool running = true;
        while (running) {
            // Mettre à jour GameClient (lit inputs et envoie via sa ref networkClient)
            gameClient.processInput();
            // Applique la logique locale (interpolation...)
            gameClient.updatePrediction();

            // --- PAS DE RENDER ---

            // Pause
            std::this_thread::sleep_for(std::chrono::milliseconds(16));

            // TODO: Condition de sortie
            // Exemple simple: si on appuie sur Echap (nécessite modif InputManager)
            // if (input.isKeyPressed(ESCAPE_KEY_CODE)) running = false;
        }
        std::cout << "[CLIENT MAIN] Fin boucle principale." << std::endl;

        // 5. Nettoyage
        networkClient.send_disconnect();
        networkClient.stop(); // Arrête le thread réseau

    } catch (const std::exception& e) {
        std::cerr << "Erreur: " << e.what() << std::endl;
        return 84;
    }
    std::cout << "[CLIENT MAIN] Terminé." << std::endl;
    return 0;
}