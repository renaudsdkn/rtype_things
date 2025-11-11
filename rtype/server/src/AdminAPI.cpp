#include "../include/server/AdminAPI.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>

using json = nlohmann::json;

AdminAPI::AdminAPI(std::shared_ptr<GameManager> gm, uint16_t port)
    : m_gameManager(gm), m_port(port), m_server(std::make_unique<httplib::Server>())
{
    std::cout << "[AdminAPI] Initializing on port " << m_port << "..." << std::endl;

    // --- CORS headers (permet appel depuis navigateur local)
    m_server->set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });

    // --- OPTIONS (preflight CORS) ---
    m_server->Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204; // No Content
    });

    // --- GET /admin/health ---
    m_server->Get("/admin/health", [this](const httplib::Request&, httplib::Response& res) {
        json resp;
        resp["status"] = "ok";
        resp["timestamp"] = std::time(nullptr);
        resp["rooms_count"] = m_gameManager->getRooms().size();
        
        // Count total players
        int totalPlayers = 0;
        for (const auto& room : m_gameManager->getRooms()) {
            totalPlayers += room->getPlayerCount();
        }
        resp["total_players"] = totalPlayers;
        
        res.set_content(resp.dump(2), "application/json");
        std::cout << "[AdminAPI] GET /admin/health" << std::endl;
    });

    // --- GET /admin/rooms (avec infos joueurs détaillées) ---
    m_server->Get("/admin/rooms", [this](const httplib::Request&, httplib::Response& res) {
        json rooms_arr = json::array();
        
        for (const auto& room : m_gameManager->getRooms()) {
            json r;
            r["id"] = room->getId();
            r["state"] = static_cast<int>(room->getCurrentState());
            
            // State name
            switch (room->getCurrentState()) {
                case Room::State::WAITING_FOR_PLAYERS: r["state_name"] = "WAITING"; break;
                case Room::State::STARTING: r["state_name"] = "STARTING"; break;
                case Room::State::PLAYING: r["state_name"] = "PLAYING"; break;
                case Room::State::GAME_OVER: r["state_name"] = "GAME_OVER"; break;
                case Room::State::FINISHED: r["state_name"] = "FINISHED"; break;
                default: r["state_name"] = "UNKNOWN"; break;
            }
            
            r["player_count"] = room->getPlayerCount();
            r["max_players"] = room->getConfig().maxPlayers;
            
            // --- Liste des joueurs dans cette room ---
            json players_arr = json::array();
            auto playerIds = m_gameManager->getPlayersInRoom(room->getId());
            
            for (uint32_t playerId : playerIds) {
                json p;
                p["player_id"] = playerId;
                p["nickname"] = m_gameManager->getPlayerManager().getNickname(playerId);
                
                // Score actuel (depuis Engine via Room)
                try {
                    int score = room->getEngine()->get_player_score(playerId);
                    p["score"] = score;
                } catch (...) {
                    p["score"] = 0;
                }
                
                // Endpoint (IP:port)
                auto endpoint = m_gameManager->getPlayerManager().getEndpointById(playerId);
                if (endpoint.has_value()) {
                    p["ip"] = endpoint->address().to_string();
                    p["port"] = endpoint->port();
                } else {
                    p["ip"] = "unknown";
                    p["port"] = 0;
                }
                
                players_arr.push_back(p);
            }
            
            r["players"] = players_arr;
            rooms_arr.push_back(r);
        }
        
        json resp;
        resp["rooms"] = rooms_arr;
        resp["timestamp"] = std::time(nullptr);
        res.set_content(resp.dump(2), "application/json");
        std::cout << "[AdminAPI] GET /admin/rooms (" << rooms_arr.size() << " rooms)" << std::endl;
    });

    // --- GET /admin/live_scores (scores temps réel - seulement rooms actives) ---
    m_server->Get("/admin/live_scores", [this](const httplib::Request&, httplib::Response& res) {
        json resp;
        resp["timestamp"] = std::time(nullptr);
        resp["rooms"] = json::array();
        
        try {
            for (const auto& room : m_gameManager->getRooms()) {
                // Ignore les rooms terminées
                if (room->getCurrentState() == Room::State::FINISHED) continue;
                
                json r;
                r["room_id"] = room->getId();
                r["state"] = static_cast<int>(room->getCurrentState());
                
                // State name
                switch (room->getCurrentState()) {
                    case Room::State::WAITING_FOR_PLAYERS: r["state_name"] = "WAITING"; break;
                    case Room::State::STARTING: r["state_name"] = "STARTING"; break;
                    case Room::State::PLAYING: r["state_name"] = "PLAYING"; break;
                    case Room::State::GAME_OVER: r["state_name"] = "GAME_OVER"; break;
                    default: r["state_name"] = "UNKNOWN"; break;
                }
                
                // Liste des joueurs avec scores EN DIRECT
                json players_arr = json::array();
                auto playerIds = m_gameManager->getPlayersInRoom(room->getId());
                
                for (uint32_t playerId : playerIds) {
                    json p;
                    p["player_id"] = playerId;
                    p["nickname"] = m_gameManager->getPlayerManager().getNickname(playerId);
                    
                    // ✅ SCORE EN DIRECT depuis Engine (pas depuis fichier)
                    try {
                        int score = room->getEngine()->get_player_score(playerId);
                        p["score"] = score;
                    } catch (const std::exception& e) {
                        std::cerr << "[AdminAPI] Error getting score for player " << playerId << ": " << e.what() << std::endl;
                        p["score"] = 0;
                    }
                    
                    players_arr.push_back(p);
                }
                
                r["players"] = players_arr;
                resp["rooms"].push_back(r);
            }
        } catch (const std::exception& e) {
            std::cerr << "[AdminAPI] Exception in /admin/live_scores: " << e.what() << std::endl;
            resp["error"] = e.what();
        }
        
        res.set_content(resp.dump(2), "application/json");
        std::cout << "[AdminAPI] GET /admin/live_scores (" << resp["rooms"].size() << " active rooms)" << std::endl;
    });

    // --- POST /admin/room/:id/force_end ---
    m_server->Post(R"(/admin/room/(\d+)/force_end)", [this](const httplib::Request& req, httplib::Response& res) {
        uint32_t roomId = std::stoi(req.matches[1]);
        bool success = m_gameManager->forceEndRoom(roomId);
        
        json resp;
        resp["success"] = success;
        resp["room_id"] = roomId;
        resp["message"] = success ? "Room ended successfully" : "Room not found";
        res.set_content(resp.dump(2), "application/json");
        
        std::cout << "[AdminAPI] POST /admin/room/" << roomId << "/force_end -> " 
                  << (success ? "OK" : "FAILED") << std::endl;
    });

    // --- POST /admin/player/:id/kick ---
    m_server->Post(R"(/admin/player/(\d+)/kick)", [this](const httplib::Request& req, httplib::Response& res) {
        uint32_t playerId = std::stoi(req.matches[1]);
        bool success = m_gameManager->kickPlayer(playerId);
        
        json resp;
        resp["success"] = success;
        resp["player_id"] = playerId;
        resp["message"] = success ? "Player kicked" : "Player not found";
        res.set_content(resp.dump(2), "application/json");
        
        std::cout << "[AdminAPI] POST /admin/player/" << playerId << "/kick -> " 
                  << (success ? "OK" : "FAILED") << std::endl;
    });

    // --- POST /admin/shutdown ---
    m_server->Post("/admin/shutdown", [this](const httplib::Request&, httplib::Response& res) {
        json resp;
        resp["status"] = "shutting_down";
        resp["message"] = "Server will shutdown in 2 seconds";
        res.set_content(resp.dump(2), "application/json");
        
        std::cout << "[AdminAPI] POST /admin/shutdown - initiating shutdown..." << std::endl;
        
        // Demande shutdown après 2 secondes (grace period)
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            m_gameManager->requestShutdown();
        }).detach();
    });

    // --- GET /admin/scores (lit dernier fichier JSON généré à GAME_OVER) ---
    m_server->Get("/admin/scores", [](const httplib::Request&, httplib::Response& res) {
        std::ifstream f("data/last_match_scores.json");
        if (f.is_open()) {
            std::stringstream buffer;
            buffer << f.rdbuf();
            res.set_content(buffer.str(), "application/json");
            std::cout << "[AdminAPI] GET /admin/scores - served from file" << std::endl;
        } else {
            res.status = 404;
            json resp;
            resp["error"] = "No scores available yet";
            resp["message"] = "Play a match to generate scores";
            res.set_content(resp.dump(2), "application/json");
            std::cout << "[AdminAPI] GET /admin/scores - no file found" << std::endl;
        }
    });

    std::cout << "[AdminAPI] ✅ All endpoints registered" << std::endl;
}

AdminAPI::~AdminAPI() {
    stop();
}

void AdminAPI::start() {
    if (m_running) {
        std::cout << "[AdminAPI] ⚠️ Already running" << std::endl;
        return;
    }
    m_running = true;
    m_thread = std::thread(&AdminAPI::serverLoop, this);
    std::cout << "[AdminAPI] ✅ Listening on http://127.0.0.1:" << m_port << std::endl;
}

void AdminAPI::stop() {
    if (!m_running) return;
    
    std::cout << "[AdminAPI] 🛑 Stopping..." << std::endl;
    m_running = false;
    
    // ✅ FORCER l'arrêt du serveur HTTP
    m_server->stop();
    
    // ✅ Attendre que le thread se termine
    if (m_thread.joinable()) {
        m_thread.join();
    }
    
    std::cout << "[AdminAPI] 🛑 Stopped" << std::endl;
}
void AdminAPI::serverLoop() {
    m_server->listen("127.0.0.1", m_port); // Écoute uniquement localhost (sécurité)
}