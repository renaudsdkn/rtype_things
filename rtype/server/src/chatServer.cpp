/*
 *
 *
 */
#include "../include/server/chatServer.hpp" 
/*
 * ChatServer Implementation
 */


// ═══════════════════════════════════════════════════════════════
// Constructeur
// ═══════════════════════════════════════════════════════════════

ChatServer::ChatServer(std::size_t port) {
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0)
        throw ChatErrors("socket creation failed...\n");

    // ✅ RECOMMANDÉ : Activer SO_REUSEADDR (évite "Address already in use")
    int opt = 1;
    setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    s_info = {AF_INET, htons(port), (struct in_addr){INADDR_ANY}};
    info_len = sizeof(s_info);

    if (bind(_serverFd, (struct sockaddr*)&s_info, info_len) == -1) {
        close(_serverFd);
        throw ChatErrors("Bind Failed\n");
    }

    if (listen(_serverFd, 100) != 0)
        throw ChatErrors("Failed to listen\n");

    _id_tables.push_back((struct pollfd) {
        .fd = _serverFd,
        .events = POLLIN,
        .revents = 0
    });

    std::cout << "[ChatServer] Serveur TCP créé sur port " << port << std::endl;
}

// ═══════════════════════════════════════════════════════════════
// Accepter nouvelle connexion
// ═══════════════════════════════════════════════════════════════

void ChatServer::addUser() {
    auto newfd = accept(_serverFd, (struct sockaddr*)&s_info, &info_len);
    if (newfd < 0)
        throw ChatErrors("Client couldn't connect\n");

    _id_tables.push_back((struct pollfd) {
        .fd = newfd,
        .events = POLLIN,
        .revents = 0
    });

    _users[newfd] = {"", ""};
    std::cout << "[ChatServer] Nouvelle connexion acceptée (fd: " << newfd << ")" << std::endl;
}

// ═══════════════════════════════════════════════════════════════
// Lit et broadcast message (méthode originale)
// ═══════════════════════════════════════════════════════════════

void ChatServer::streamMessage(int pos) {
    int fd = _id_tables[pos].fd;
    char buffer[2048];
    int size = read(fd, buffer, sizeof(buffer) - 1);

    if (size <= 0) {
        removeUser(pos);
        return;
    }

    // Nettoie \n\r
    while (size > 0 && (buffer[size-1] == '\n' || buffer[size-1] == '\r')) {
        buffer[--size] = '\0';
    }
    buffer[size] = '\0';

    // Premier message = username
    if (_users[fd].user.empty()) {
        _users[fd].user = std::string(buffer);
        std::cout << "[ChatServer] User '" << _users[fd].user 
                  << "' joined (fd: " << fd << ")\n";

        // Notifie les autres
        std::string joinMsg = _users[fd].user + " joined the chat\n";
        broadcastMessage(joinMsg);
    } else {
        // Message normal
        std::string msg = _users[fd].user + ": " + std::string(buffer);
        std::cout << "[ChatServer] " << msg;
        broadcastMessage(msg);
    }
}

// ═══════════════════════════════════════════════════════════════
// Broadcast à tous
// ═══════════════════════════════════════════════════════════════

void ChatServer::broadcastMessage(const std::string& msg) {
    for (std::size_t i = 1; i < _id_tables.size(); i++) {
        int fd = _id_tables[i].fd;
        write(fd, msg.c_str(), msg.size());
    }
}

// ═══════════════════════════════════════════════════════════════
// Supprime utilisateur
// ═══════════════════════════════════════════════════════════════

void ChatServer::removeUser(int &pos) {
    int fd = _id_tables[pos].fd;

    if (!_users[fd].user.empty()) {
        std::string leaveMsg = _users[fd].user + " left the chat\n";
        std::cout << "[ChatServer] " << leaveMsg;
        broadcastMessage(leaveMsg);
    }

    _users.erase(fd);
    close(fd);
    _id_tables.erase(_id_tables.begin() + pos);
    pos = 0;  // Réinitialise position après suppression
}

// ═══════════════════════════════════════════════════════════════
// ✅ NOUVELLE MÉTHODE : Lit message sans broadcaster
// ═══════════════════════════════════════════════════════════════

std::optional<std::pair<std::string, std::string>> ChatServer::readMessage(int pos) {
    int fd = _id_tables[pos].fd;
    char buffer[2048];
    int size = read(fd, buffer, sizeof(buffer) - 1);

    // Déconnexion ou erreur
    if (size <= 0) {
        return std::nullopt;
    }

    // Nettoie \n\r
    while (size > 0 && (buffer[size-1] == '\n' || buffer[size-1] == '\r')) {
        buffer[--size] = '\0';
    }
    buffer[size] = '\0';

    // Premier message = username (connexion)
    if (_users[fd].user.empty()) {
        _users[fd].user = std::string(buffer);
        std::cout << "[ChatServer] User '" << _users[fd].user 
                  << "' registered (fd: " << fd << ")\n";

        // Retourne message système de connexion
        return std::make_pair("System", _users[fd].user + " joined the chat");
    }

    // Message normal
    std::string username = _users[fd].user;
    std::string message = std::string(buffer);

    return std::make_pair(username, message);
}