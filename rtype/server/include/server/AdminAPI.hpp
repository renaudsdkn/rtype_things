#pragma once
#include <memory>
#include <thread>
#include <atomic>
#include <string>
#include <httplib.h>
#include "room.hpp"
// Forward declarations
class GameManager;
class Room;

class AdminAPI {
public:
    explicit AdminAPI(std::shared_ptr<GameManager> gm, uint16_t port = 8080);
    ~AdminAPI();

    void start();  // Lance thread HTTP server
    void stop();   // Arrête proprement

private:
    void serverLoop();  // Boucle principale HTTP
    
    std::shared_ptr<GameManager> m_gameManager;
    std::unique_ptr<httplib::Server> m_server;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    uint16_t m_port;
};