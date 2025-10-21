#include <iostream>
#include <thread>
#include <memory>


// NOTE : Ces includes sont pourris.
// Tu dois AJOUTER ces fichiers .cpp à ton target_link_libraries dans CMakeLists.txt
// Je les mets ici pour que ça compile en exemple.
#include "../include/server/server.hpp"
#include "../include/server/GameManager.hpp"
#include "../include/server/ThreadSafeQueue.hpp"
// PlayerManager.cpp est déjà dans ton CMakeLists.txt


int main(int argc, char* argv[]) {
    try {
        unsigned short port = 1234;
        if (argc > 1) port = static_cast<unsigned short>(std::stoi(argv[1]));

        asio::io_context io_context;
        auto incomingQueue = std::make_shared<ThreadSafeQueue<NetworkPacket>>();
        auto server = std::make_shared<UdpServer>(io_context, port, incomingQueue);
        auto gameManager = std::make_shared<GameManager>(incomingQueue, server);

        server->startReceive();

        std::thread networkThread([&io_context]() {
            try {
                std::cout << "[MAIN] Lancement du Thread Réseau..." << std::endl;
                io_context.run();
            } catch (const std::exception& e) { std::cerr << "[FATAL] Erreur Thread Réseau: " << e.what() << std::endl; }
        });

        std::thread gameThread([&gameManager]() {
            try {
                std::cout << "[MAIN] Lancement du Thread Jeu..." << std::endl;
                gameManager->run();
            } catch (const std::exception& e) { std::cerr << "[FATAL] Erreur Thread Jeu: " << e.what() << std::endl; }
        });

        networkThread.join();
        gameThread.join();

    } catch (const std::exception& e) {
        std::cerr << "[FATAL] Exception dans main: " << e.what() << std::endl;
        return 84;
    }
    return 0;
}