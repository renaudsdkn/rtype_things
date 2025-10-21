#include "../include/server/GameManager.hpp"
#include "protocol/Factory.hpp"
#include "protocol/serializer.hpp"

GameManager::GameManager(
    std::shared_ptr<ThreadSafeQueue<NetworkPacket>> incoming,
    std::shared_ptr<UdpServer> server)
    : m_incomingMessages(incoming),
      m_server(server),
      m_playerManager(),
      m_messageHandler(m_playerManager, *this, *server) // Donner les références
{
    m_lastCleanupTime = std::chrono::steady_clock::now();
}

void GameManager::stop() { m_running = false; }

void GameManager::run() {
    std::cout << "[THREAD JEU] Boucle de jeu démarrée." << std::endl;
    auto lastTime = std::chrono::steady_clock::now();
    const std::chrono::milliseconds tickRate(16); // ~60 FPS

    while (m_running) {
        auto now = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(now - lastTime).count();
        lastTime = now;

        processNetworkInputs();
        updateGame(deltaTime);
        broadcastSnapshots();
        cleanupPlayers();

        auto sleepTime = tickRate - (std::chrono::steady_clock::now() - now);
        if (sleepTime > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(sleepTime);
        }
    }
}

void GameManager::processNetworkInputs() {
    std::queue<NetworkPacket> packets = m_incomingMessages->popAll();
    while (!packets.empty()) {
        auto packet = std::move(packets.front());
        packets.pop();
        
        m_playerManager.updatePlayerActivity(packet.sender);

        try {
            auto message = Protocol::MessageFactory::deserialize(packet.data);
            m_messageHandler.handleMessage(message, packet.sender);
        } catch (const std::exception& e) {
            std::cerr << "[THREAD JEU] Erreur désérialisation: " << e.what() << std::endl;
        }
    }
}

void GameManager::updateGame(float deltaTime) {
    for (auto& room : m_rooms) {
        room->update(deltaTime);
    }
}

void GameManager::broadcastSnapshots() {
    // Parcours toutes les rooms actives
    for (auto& room : m_rooms) {
        if (room->isEmpty()) continue; // N'envoie rien si la room est vide

        // 1. Récupère le snapshot pour cette room
        ProtocolData::Snapshot snapData = room->getSnapshot();

        // Ne rien envoyer si le snapshot est vide (aucune entité à synchroniser)
        if (snapData.entities.empty()) continue;

        // 2. Crée le message réseau
        Protocol::SnapshotMessage msg(snapData);

        // 3. Sérialise le message (utilise la nouvelle sérialisation manuelle)
        std::vector<uint8_t> dataToSend = msg.serialize();

        // 4. Récupère les destinataires (joueurs de cette room)
        std::vector<asio::ip::udp::endpoint> endpoints = room->getPlayerEndpoints();

        // 5. Envoie le paquet à chaque joueur de la room
        for (const auto& ep : endpoints) {
            m_server->send(dataToSend, ep); // Utilise la fonction send du UdpServer
        }
    }
}

void GameManager::cleanupPlayers() {
    // Vérifie si c'est le moment de nettoyer (ex: toutes les 35 secondes)
    if (std::chrono::steady_clock::now() - m_lastCleanupTime > std::chrono::seconds(35)) {

        // 1. Appelle removeInactivePlayers (qui retourne maintenant les IDs des joueurs supprimés)
        std::vector<uint32_t> removedPlayerIDs = m_playerManager.removeInactivePlayers(std::chrono::seconds(130)); // Timeout de 130s

        // 2. Pour chaque ID retourné...
        for (uint32_t playerId : removedPlayerIDs) {
            // ✅ ...appelle handlePlayerDisconnect pour le retirer de sa room et de l'engine.
            //    Cette fonction existe déjà et fait exactement ce qu'il faut :
            //    trouver la room du joueur et appeler room->removePlayer(playerId).
            handlePlayerDisconnect(playerId);
        }

        // Met à jour le temps du dernier nettoyage seulement si on a fait quelque chose
        if (!removedPlayerIDs.empty()) {
             std::cout << "[THREAD JEU] Nettoyage des joueurs inactifs terminé. " << removedPlayerIDs.size() << " joueur(s) supprimé(s)." << std::endl;
        }

        m_lastCleanupTime = std::chrono::steady_clock::now();
    }
}

Room* GameManager::findAvailableRoom() {
    for (auto& room : m_rooms) {
        if (!room->isFull()) return room.get();
    }
    uint32_t newId = m_rooms.size() + 1;
    m_rooms.push_back(std::make_unique<Room>(newId));
    return m_rooms.back().get();
}

void GameManager::handleNewPlayer(uint32_t playerId, const asio::ip::udp::endpoint& endpoint) {
    Room* room = findAvailableRoom();
    room->addPlayer(playerId, endpoint);
    m_playerToRoomMap[playerId] = room;
}

void GameManager::handlePlayerDisconnect(uint32_t playerId) {
    if (m_playerToRoomMap.count(playerId)) {
        m_playerToRoomMap[playerId]->removePlayer(playerId);
        m_playerToRoomMap.erase(playerId);
    }
}

void GameManager::handlePlayerInput(uint32_t playerId, const ProtocolData::PlayerInput& input) {
    if (m_playerToRoomMap.count(playerId)) {
        m_playerToRoomMap[playerId]->handleInput(playerId, input);
    }
}