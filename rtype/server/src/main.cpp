#include <iostream>
#include <thread>
#include <memory>

// NOTE : Ces includes sont pourris.
// Tu dois AJOUTER ces fichiers .cpp à ton target_link_libraries dans CMakeLists.txt
// Je les mets ici pour que ça compile en exemple.
#include "../include/server/server.hpp"
#include "../include/server/GameManager.hpp"
#include "../include/server/ThreadSafeQueue.hpp"
#include "../include/server/chatServer.hpp"
#include "../include/server/ChatManager.hpp"
#include "../include/server/AdminAPI.hpp"
// PlayerManager.cpp est déjà dans ton CMakeLists.txt

/*
** EPITECH PROJECT, 2025
** R-Type Server
** File description:
** Main entry point - Multi-threaded server (UDP + TCP)
*/

#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <csignal>
#include <poll.h>
#include <cerrno>
#include <cstring>

// Serveur TCP + ChatManager
// Votre ChatServer existant

// ═══════════════════════════════════════════════════════════════
// Variables Globales (Arrêt propre)
// ═══════════════════════════════════════════════════════════════
/*
** EPITECH PROJECT, 2025
** R-Type Server
** File description:
** Main entry point - Multi-threaded server (UDP + TCP)
*/

#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <csignal>
#include <poll.h>
#include <cerrno>
#include <cstring>
#include <chrono>

// ═══════════════════════════════════════════════════════════════
// Variables Globales (Arrêt propre)
// ═══════════════════════════════════════════════════════════════
#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <csignal>
#include <poll.h>
#include <cerrno>
#include <cstring>
#include <chrono>

#include "../include/server/server.hpp"
#include "../include/server/GameManager.hpp"
#include "../include/server/ThreadSafeQueue.hpp"
#include "../include/server/chatServer.hpp"
#include "../include/server/ChatManager.hpp"
#include "../include/server/AdminAPI.hpp"

// ═══════════════════════════════════════════════════════════════
// Variables Globales
// ═══════════════════════════════════════════════════════════════

std::atomic<bool> g_running{true};
asio::io_context *g_io_context_ptr = nullptr;

// ═══════════════════════════════════════════════════════════════
// Signal Handler
// ═══════════════════════════════════════════════════════════════

void signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM) {
        static std::atomic<int> count{0};
        int currentCount = ++count;

        if (currentCount == 1) {
            std::cout << "\n[MAIN] 🛑 Signal d'arrêt reçu (Ctrl+C)..." << std::endl;
            std::cout << "[MAIN] ⏳ Arrêt en cours... (Ctrl+C à nouveau pour forcer)" << std::endl;
            g_running = false;
            if (g_io_context_ptr) {
                g_io_context_ptr->stop();
            }
        } else {
            std::cout << "\n[MAIN] 💀 Arrêt forcé !" << std::endl;
            std::exit(1);
        }
    }
}

// ═══════════════════════════════════════════════════════════════
// Fonction 1 : Affichage Banner + Configuration
// ═══════════════════════════════════════════════════════════════

struct ServerConfig {
    unsigned short udpPort;
    unsigned short tcpPort;
    unsigned short adminPort;
};

ServerConfig parseArguments(int argc, char *argv[])
{
    ServerConfig config;
    config.udpPort = 1234;
    config.tcpPort = 1235;
    config.adminPort = 8080;

    if (argc > 1) config.udpPort = static_cast<unsigned short>(std::stoi(argv[1]));
    if (argc > 2) config.tcpPort = static_cast<unsigned short>(std::stoi(argv[2]));
    if (argc > 3) config.adminPort = static_cast<unsigned short>(std::stoi(argv[3]));

    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════╗\n";
    std::cout << "║          R-TYPE DEDICATED SERVER              ║\n";
    std::cout << "╠═══════════════════════════════════════════════╣\n";
    std::cout << "║  UDP Port (Game)  : " << config.udpPort << "                      ║\n";
    std::cout << "║  TCP Port (Chat)  : " << config.tcpPort << "                      ║\n";
    std::cout << "║  HTTP Port (Admin): " << config.adminPort << "                      ║\n";
    std::cout << "╚═══════════════════════════════════════════════╝\n";
    std::cout << std::endl;

    return config;
}

// ═══════════════════════════════════════════════════════════════
// Fonction 2 : Initialisation Serveurs
// ═══════════════════════════════════════════════════════════════

struct ServerComponents {
    std::shared_ptr<asio::io_context> ioContext;
    std::shared_ptr<ThreadSafeQueue<NetworkPacket>> incomingQueue;
    std::shared_ptr<UdpServer> udpServer;
    std::shared_ptr<GameManager> gameManager;
    std::shared_ptr<ChatServer> chatServer;
    std::shared_ptr<ChatManager> chatManager;
    std::shared_ptr<AdminAPI> adminAPI;
};

ServerComponents initializeServers(const ServerConfig& config)
{
    ServerComponents components;

    // UDP + GameManager
    components.ioContext = std::make_shared<asio::io_context>();
    g_io_context_ptr = components.ioContext.get();

    components.incomingQueue = std::make_shared<ThreadSafeQueue<NetworkPacket>>();
    components.udpServer = std::make_shared<UdpServer>(*components.ioContext, config.udpPort, components.incomingQueue);
    components.gameManager = std::make_shared<GameManager>(components.incomingQueue, components.udpServer);

    components.udpServer->startReceive();
    std::cout << "[MAIN] ✅ Serveur UDP initialisé (port " << config.udpPort << ")" << std::endl;

    // TCP + ChatManager
    components.chatServer = std::make_shared<ChatServer>(config.tcpPort);
    components.chatManager = std::make_shared<ChatManager>(components.gameManager);
    components.chatManager->setChatServer(components.chatServer);
    std::cout << "[MAIN] ✅ Serveur TCP initialisé (port " << config.tcpPort << ")" << std::endl;

    // AdminAPI
    try {
        components.adminAPI = std::make_shared<AdminAPI>(components.gameManager, config.adminPort);
        components.adminAPI->start();
        std::cout << "[MAIN] ✅ AdminAPI démarré sur http://127.0.0.1:" << config.adminPort << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "[MAIN] ⚠️ AdminAPI échec: " << e.what() << std::endl;
        std::cerr << "[MAIN] ⚠️ Serveur fonctionne SANS interface admin" << std::endl;
    }

    return components;
}

// ═══════════════════════════════════════════════════════════════
// Fonction 3 : Thread UDP Network (ASIO)
// ═══════════════════════════════════════════════════════════════

void runNetworkThread(std::shared_ptr<asio::io_context> ioContext)
{
    try {
        std::cout << "[THREAD UDP] Démarré" << std::endl;
        ioContext->run();
        std::cout << "[THREAD UDP] Terminé" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[FATAL UDP] " << e.what() << std::endl;
    }
}

// ═══════════════════════════════════════════════════════════════
// Fonction 4 : Thread Game Loop
// ═══════════════════════════════════════════════════════════════

void runGameThread(std::shared_ptr<GameManager> gameManager)
{
    try {
        std::cout << "[THREAD JEU] Démarré" << std::endl;
        gameManager->run();
        std::cout << "[THREAD JEU] Terminé" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[FATAL JEU] " << e.what() << std::endl;
    }
}

// ═══════════════════════════════════════════════════════════════
// Fonction 5 : Thread Chat (Poll-based)
// ═══════════════════════════════════════════════════════════════

void runChatThread(std::shared_ptr<ChatServer> chatServer, std::shared_ptr<ChatManager> chatManager)
{
    try {
        std::cout << "[THREAD CHAT] Démarré" << std::endl;

        auto &id_table = chatServer->getIDtable();

        while (g_running) {
            int pollResult = poll(id_table.data(), id_table.size(), 100);

            if (pollResult < 0) {
                if (errno == EINTR) continue;
                std::cerr << "[THREAD CHAT] ❌ Erreur poll(): " << strerror(errno) << std::endl;
                break;
            }

            if (pollResult == 0) continue;

            for (int i = static_cast<int>(id_table.size()) - 1; i >= 0; i--) {
                if (id_table[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
                    if (i > 0) {
                        int fd = id_table[i].fd;
                        std::cout << "[THREAD CHAT] 🔌 Déconnexion FD " << fd << std::endl;
                        chatManager->onUserDisconnected(fd);
                        chatServer->removeUser(i);
                    }
                } 
                else if (id_table[i].revents & POLLIN) {
                    if (i == 0) {
                        try {
                            chatServer->addUser();
                            std::cout << "[THREAD CHAT] 🟢 Nouvelle connexion" << std::endl;
                        } catch (const ChatErrors& e) {
                            std::cerr << "[THREAD CHAT] ❌ addUser: " << e.what() << std::endl;
                        }
                    } else {
                        int fd = id_table[i].fd;
                        auto msgOpt = chatServer->readMessage(i);

                        if (msgOpt.has_value()) {
                            auto [username, message] = msgOpt.value();

                            if (username == "System") {
                                std::string nickname = message.substr(0, message.find(" joined"));
                                std::cout << "[THREAD CHAT] 👤 Connexion: '" << nickname << "' (FD " << fd << ")" << std::endl;
                                chatManager->onUserConnected(fd, nickname);
                            } else {
                                std::cout << "[THREAD CHAT] 💬 " << username << ": " << message << std::endl;
                                chatManager->handleMessage(fd, message);
                            }
                        } else {
                            std::cout << "[THREAD CHAT] 🔌 Client déconnecté (FD " << fd << ")" << std::endl;
                            chatManager->onUserDisconnected(fd);
                            chatServer->removeUser(i);
                        }
                    }
                }
            }
        }

        std::cout << "[THREAD CHAT] Terminé proprement" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[FATAL CHAT] " << e.what() << std::endl;
    }
}

// ═══════════════════════════════════════════════════════════════
// Fonction 6 : Attente Thread avec Timeout
// ═══════════════════════════════════════════════════════════════

bool waitForThreadWithTimeout(std::thread& thread, const std::string& threadName, int timeoutSeconds = 5)
{
    auto start = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::seconds(timeoutSeconds);

    while (thread.joinable() && (std::chrono::steady_clock::now() - start) < timeout) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (thread.joinable()) {
        thread.join();
        std::cout << "[MAIN] ✅ Thread " << threadName << " terminé" << std::endl;
        return true;
    }

    std::cout << "[MAIN] ⚠️ Thread " << threadName << " n'a pas répondu (timeout)" << std::endl;
    return false;
}

// ═══════════════════════════════════════════════════════════════
// Fonction 7 : Cleanup (Arrêt propre)
// ═══════════════════════════════════════════════════════════════
void shutdownServers(ServerComponents& components)
{
    std::cout << "[MAIN] 🛑 Début du shutdown..." << std::endl;
    
    // 1. Arrêter ASIO (débloquer networkThread)
    if (components.ioContext) {
        std::cout << "[MAIN] 🛑 Arrêt ASIO io_context..." << std::endl;
        components.ioContext->stop();
    }
    
    // 2. Arrêter AdminAPI
    if (components.adminAPI) {
        std::cout << "[MAIN] 🛑 Arrêt AdminAPI..." << std::endl;
        components.adminAPI->stop();
        std::cout << "[MAIN] ✅ AdminAPI arrêté" << std::endl;
    }
    
    std::cout << "[MAIN] ✅ Arrêt complet du serveur" << std::endl;
}
// ═══════════════════════════════════════════════════════════════
// MAIN (Clean & Minimal)
// ═══════════════════════════════════════════════════════════════

int main(int argc, char *argv[])
{
    try {
        // 1. Configuration
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        ServerConfig config = parseArguments(argc, argv);

        // 2. Initialisation
        ServerComponents servers = initializeServers(config);

        // 3. Lancement Threads
        std::cout << "[MAIN] 🚀 Démarrage des threads...\n" << std::endl;

        std::thread networkThread([&servers]() { runNetworkThread(servers.ioContext); });
        std::thread gameThread([&servers]() { runGameThread(servers.gameManager); });
        std::thread chatThread([&servers]() { runChatThread(servers.chatServer, servers.chatManager); });

        // 4. Monitoring
        std::cout << "[MAIN] 🎮 Serveur en cours d'exécution..." << std::endl;
        std::cout << "[MAIN] 💡 Appuyez sur Ctrl+C pour arrêter\n" << std::endl;

        // 5. Attente arrêt
        chatThread.join();
        std::cout << "[MAIN] ⏳ Attente arrêt threads restants..." << std::endl;

        waitForThreadWithTimeout(networkThread, "UDP", 5);
        
        if (!waitForThreadWithTimeout(gameThread, "Jeu", 5)) {
            std::cout << "[MAIN] ⚠️ Vérifiez que GameManager::run() vérifie g_running" << std::endl;
            std::exit(0);
        }

        // 6. Cleanup
        shutdownServers(servers);

    } catch (const std::exception &e) {
        std::cerr << "\n[FATAL] Exception dans main: " << e.what() << std::endl;
        return 84;
    }

    return 0;
}