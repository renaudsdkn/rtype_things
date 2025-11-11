
#include "../include/client/Game.hpp"
#include <iostream> 
#include "../include/client/Game.hpp"
#include <iostream>
#include <cstring>


int main(int argc, char* argv[]) {
    // Valeurs par défaut
    std::string serverIp = "127.0.0.1";
    unsigned short udpPort = 1234;
    unsigned short tcpPort = 1235;

    // Parse simple (si arguments fournis)
    if (argc >= 2) serverIp = argv[1];
    if (argc >= 3) udpPort = std::atoi(argv[2]);
    if (argc >= 4) tcpPort = std::atoi(argv[3]);

    // Lance le jeu
    try {
        Game game(1280, 800, serverIp, udpPort, tcpPort);
        game.run();
    } catch (const std::exception& e) {
        std::cerr << "Erreur: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}