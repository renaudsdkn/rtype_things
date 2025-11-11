#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <cstdint>

// Forward declarations
class GameManager;
class PlayerManager;
class ChatServer;

/**
 * @class ChatManager
 * @brief Pont entre ChatServer (TCP/FD) et GameManager (PlayerID/Rooms)
 * 
 * Responsabilités :
 * - Mapper FD socket ↔ PlayerID
 * - Router messages chat vers la bonne room
 * - Synchroniser avec PlayerManager (pseudos)
 */
class ChatManager {
public:
    /**
     * @brief Constructeur
     * @param gameManager Référence au GameManager (pour accéder aux rooms)
     */
    explicit ChatManager(std::shared_ptr<GameManager> gameManager);
    
    ~ChatManager() = default;
    
    // ═══════════════════════════════════════════════════════════
    // Callbacks du Thread TCP
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Appelé quand un client TCP se connecte
     * @param fd File descriptor du socket TCP
     * @param nickname Pseudo envoyé par le client (premier message)
     * 
     * Flow :
     * 1. Récupère PlayerID depuis PlayerManager via nickname
     * 2. Mappe fd → PlayerID
     * 3. Log connexion
     */
    void onUserConnected(int fd, const std::string& nickname);
    
    /**
     * @brief Appelé quand un message chat est reçu
     * @param fd File descriptor du socket émetteur
     * @param message Contenu du message (texte brut)
     * 
     * Flow :
     * 1. Trouve PlayerID depuis fd
     * 2. Récupère pseudo depuis PlayerManager
     * 3. Trouve RoomID depuis GameManager
     * 4. Broadcast aux joueurs de la room
     */
    void handleMessage(int fd, const std::string& message);
    
    /**
     * @brief Appelé quand un client TCP se déconnecte
     * @param fd File descriptor du socket fermé
     * 
     * Flow :
     * 1. Supprime mapping fd → PlayerID
     * 2. Log déconnexion
     */
    void onUserDisconnected(int fd);
    
    // ═══════════════════════════════════════════════════════════
    // Helpers
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Récupère le FD TCP d'un joueur (pour broadcast)
     * @param playerId ID du joueur dans le jeu
     * @return FD socket si connecté, -1 sinon
     */
    int getFdForPlayer(uint32_t playerId) const;
    
    /**
     * @brief Envoie un message à un joueur spécifique
     * @param playerId ID du joueur cible
     * @param message Message à envoyer (formaté "username: text")
     */
    void sendToPlayer(uint32_t playerId, const std::string& message);
    
    /**
     * @brief Envoie un message à tous les joueurs d'une room
     * @param roomId ID de la room
     * @param message Message à envoyer
     */
    void broadcastToRoom(uint32_t roomId, const std::string& message);
    
    /**
     * @brief Lie le ChatServer (pour envoyer des messages)
     * @param chatServer Pointeur vers le ChatServer
     */
    void setChatServer(std::shared_ptr<ChatServer> chatServer);

private:
    // ═══════════════════════════════════════════════════════════
    // Membres
    // ═══════════════════════════════════════════════════════════
    
    std::shared_ptr<GameManager> m_gameManager;   ///< Accès aux rooms et players
    std::shared_ptr<ChatServer> m_chatServer;     ///< Accès au serveur TCP (pour write)
    
    /// Mapping FD socket ↔ PlayerID (thread-safe avec mutex)
    std::unordered_map<int, uint32_t> m_fdToPlayerId;
    
    /// Mapping inverse PlayerID → FD (pour broadcast rapide)
    std::unordered_map<uint32_t, int> m_playerIdToFd;
    
    /// Mutex pour protéger les mappings (accès concurrent thread TCP)
    mutable std::mutex m_mutex;
    
    // ═══════════════════════════════════════════════════════════
    // Helpers privés
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Récupère PlayerID depuis FD (thread-safe)
     * @param fd File descriptor
     * @return PlayerID si trouvé, 0 sinon
     */
    uint32_t getPlayerIdFromFd(int fd) const;
    
    /**
     * @brief Formate un message chat avec username
     * @param username Pseudo de l'émetteur
     * @param message Contenu du message
     * @return "username: message\n"
     */
    std::string formatMessage(const std::string& username, const std::string& message) const;
};