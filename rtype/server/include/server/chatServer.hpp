/*
 *
 *
 */
/*
 * ChatServer - Serveur TCP pour le chat
 */

#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <exception>
#include <cstring>
#include <optional>      // ✅ AJOUTER
#include <utility>       // ✅ AJOUTER (pour std::pair)

// ═══════════════════════════════════════════════════════════════
// Structures
// ═══════════════════════════════════════════════════════════════

typedef struct {
    std::string user;
    std::string text;
} user_t;

// ═══════════════════════════════════════════════════════════════
// Exception
// ═══════════════════════════════════════════════════════════════

class ChatErrors : public std::exception {
public:
    ChatErrors(const std::string& message) 
        : message_(std::string("Chat Error: ") + message) {}
    const char* what() const throw() { return message_.c_str(); }
private:
    std::string message_;
};

// ═══════════════════════════════════════════════════════════════
// ChatServer
// ═══════════════════════════════════════════════════════════════

class ChatServer {
private:
    int _serverFd;
    struct sockaddr_in s_info;
    socklen_t info_len;
    std::vector<struct pollfd> _id_tables;
    std::unordered_map<int, user_t> _users;

public:
    /**
     * @brief Constructeur
     * @param port Port TCP à écouter
     */
    ChatServer(std::size_t port);

    /**
     * @brief Accepte une nouvelle connexion
     */
    void addUser();

    /**
     * @brief Lit et broadcast un message (méthode originale)
     * @param pos Position dans _id_tables
     */
    void streamMessage(int pos);

    /**
     * @brief Broadcast un message à tous les clients
     * @param msg Message à envoyer
     */
    void broadcastMessage(const std::string& msg);

    /**
     * @brief Supprime un utilisateur
     * @param pos Position dans _id_tables (sera mise à 0)
     */
    void removeUser(int &pos);

    // ✅ NOUVELLES MÉTHODES (pour intégration ChatManager)

    /**
     * @brief Lit un message SANS le broadcaster (pour ChatManager)
     * @param pos Position dans _id_tables
     * @return {username, message} si succès, nullopt si déconnexion
     */
    std::optional<std::pair<std::string, std::string>> readMessage(int pos);

    /**
     * @brief Expose _id_tables pour poll() externe
     * @return Référence vers le vecteur de pollfd
     */
    std::vector<struct pollfd>& getIDtable() { return _id_tables; }
};