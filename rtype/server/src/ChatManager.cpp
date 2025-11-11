#include "../include/server/ChatManager.hpp"
#include "../include/server/GameManager.hpp"
#include "../include/server/PlayerManager.hpp"
#include "../include/server/room.hpp"
#include "../include/server/chatServer.hpp"  // Votre ChatServer existant
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>


ChatManager::ChatManager(std::shared_ptr<GameManager> gameManager)
    : m_gameManager(gameManager), m_chatServer(nullptr) {
    std::cout << "[ChatManager] Initialisé." << std::endl;
}

// ═══════════════════════════════════════════════════════════════
// Callbacks
// ═══════════════════════════════════════════════════════════════

void ChatManager::onUserConnected(int fd, const std::string& nickname) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::cout << "[ChatManager] Connexion TCP: FD=" << fd 
              << ", Pseudo='" << nickname << "'" << std::endl;
    
    // ✅ Récupère PlayerID depuis PlayerManager (via GameManager)
    auto playerIdOpt = m_gameManager->getPlayerManager().getIdByNickname(nickname);
    
    if (!playerIdOpt.has_value()) {
        std::cerr << "[ChatManager] ❌ Pseudo '" << nickname 
                  << "' inconnu (pas authentifié UDP ?)" << std::endl;
        // Option 1 : Fermer la connexion
        // close(fd);
        // Option 2 : Accepter quand même (pour debug)
        return;
    }
    
    uint32_t playerId = playerIdOpt.value();
    
    // ✅ Stocker mapping bidirectionnel
    m_fdToPlayerId[fd] = playerId;
    m_playerIdToFd[playerId] = fd;
    
    std::cout << "[ChatManager] ✅ Mapping: FD " << fd 
              << " ↔ PlayerID " << playerId 
              << " ('" << nickname << "')" << std::endl;
    
    // ✅ Message de bienvenue dans le chat
    std::string welcomeMsg = "System: " + nickname + " connected to chat\n";
    
    // Broadcast aux autres joueurs de la même room (si applicable)
    auto roomId = m_gameManager->getRoomIdForPlayer(playerId);
    if (roomId.has_value()) {
        broadcastToRoom(roomId.value(), welcomeMsg);
    }
}

// ───────────────────────────────────────────────────────────────

void ChatManager::handleMessage(int fd, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // ✅ 1. Récupère PlayerID depuis FD
    uint32_t playerId = getPlayerIdFromFd(fd);
    if (playerId == 0) {
        std::cerr << "[ChatManager] ❌ FD " << fd << " non mappé (joueur inconnu)" << std::endl;
        return;
    }
    
    // ✅ 2. Récupère pseudo depuis PlayerManager
    std::string nickname = m_gameManager->getPlayerManager().getNickname(playerId);
    if (nickname.empty()) {
        std::cerr << "[ChatManager] ❌ PlayerID " << playerId << " sans pseudo" << std::endl;
        return;
    }
    
    // ✅ 3. Récupère RoomID depuis GameManager
    auto roomIdOpt = m_gameManager->getRoomIdForPlayer(playerId);
    
    if (!roomIdOpt.has_value()) {
        // Joueur dans le lobby → Chat global (optionnel)
        std::cout << "[ChatManager] Joueur " << playerId 
                  << " dans le lobby (chat global désactivé)" << std::endl;
        return;
    }
    
    uint32_t roomId = roomIdOpt.value();
    
    // ✅ 4. Formate message
    std::string formattedMsg = formatMessage(nickname, message);
    
    std::cout << "[ChatManager] Room " << roomId << " | " << formattedMsg;
    
    // ✅ 5. Broadcast aux joueurs de la room
    broadcastToRoom(roomId, formattedMsg);
}

// ───────────────────────────────────────────────────────────────

void ChatManager::onUserDisconnected(int fd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_fdToPlayerId.find(fd);
    if (it == m_fdToPlayerId.end()) {
        std::cout << "[ChatManager] Déconnexion FD " << fd << " (non mappé)" << std::endl;
        return;
    }
    
    uint32_t playerId = it->second;
    std::string nickname = m_gameManager->getPlayerManager().getNickname(playerId);
    
    std::cout << "[ChatManager] Déconnexion: FD " << fd 
              << " (PlayerID " << playerId << ", '" << nickname << "')" << std::endl;
    
    // ✅ Supprime mappings
    m_fdToPlayerId.erase(fd);
    m_playerIdToFd.erase(playerId);
    
    // ✅ Notifie les autres (optionnel)
    auto roomIdOpt = m_gameManager->getRoomIdForPlayer(playerId);
    if (roomIdOpt.has_value()) {
        std::string leaveMsg = "System: " + nickname + " left the chat\n";
        broadcastToRoom(roomIdOpt.value(), leaveMsg);
    }
}

// ═══════════════════════════════════════════════════════════════
// Helpers Publics
// ═══════════════════════════════════════════════════════════════

int ChatManager::getFdForPlayer(uint32_t playerId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_playerIdToFd.find(playerId);
    if (it != m_playerIdToFd.end()) {
        return it->second;
    }
    return -1;  // Pas connecté au chat
}

// ───────────────────────────────────────────────────────────────

void ChatManager::sendToPlayer(uint32_t playerId, const std::string& message) {
    int fd = getFdForPlayer(playerId);
    if (fd < 0) {
        std::cerr << "[ChatManager] PlayerID " << playerId << " non connecté au chat" << std::endl;
        return;
    }
    
    // ✅ Envoie via socket TCP
    ssize_t sent = write(fd, message.c_str(), message.size());
    if (sent < 0) {
        std::cerr << "[ChatManager] Erreur write() vers FD " << fd << std::endl;
    }
}

// ───────────────────────────────────────────────────────────────

void ChatManager::broadcastToRoom(uint32_t roomId, const std::string& message) {
    // ✅ 1. Récupère la liste des PlayerIDs dans la room
    std::vector<uint32_t> playerIds = m_gameManager->getPlayersInRoom(roomId);
    
    if (playerIds.empty()) {
        std::cout << "[ChatManager] Room " << roomId << " vide, pas de broadcast" << std::endl;
        return;
    }
    
    std::cout << "[ChatManager] Broadcast à Room " << roomId 
              << " (" << playerIds.size() << " joueurs)" << std::endl;
    
    // ✅ 2. Pour chaque joueur, trouve son FD et envoie
    for (uint32_t playerId : playerIds) {
        sendToPlayer(playerId, message);
    }
}

// ───────────────────────────────────────────────────────────────

void ChatManager::setChatServer(std::shared_ptr<ChatServer> chatServer) {
    m_chatServer = chatServer;
    std::cout << "[ChatManager] ChatServer lié." << std::endl;
}

// ═══════════════════════════════════════════════════════════════
// Helpers Privés
// ═══════════════════════════════════════════════════════════════

uint32_t ChatManager::getPlayerIdFromFd(int fd) const {
    // ⚠️ Déjà appelé avec lock dans handleMessage, pas besoin de re-locker
    auto it = m_fdToPlayerId.find(fd);
    if (it != m_fdToPlayerId.end()) {
        return it->second;
    }
    return 0;
}

// ───────────────────────────────────────────────────────────────

std::string ChatManager::formatMessage(const std::string& username, const std::string& message) const {
    return username + ": " + message + "\n";
}