#include <cstddef>
#include <sys/poll.h>
#include <vector>
#include <poll.h>
#include <set>
#include <time.h>
#include <string>
#include <stdio.h>
#include <string.h>
#include <raylib.h>
#include <dirent.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include <iostream>
#include <algorithm>
#include <exception>
#include <filesystem>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

typedef enum {
    REPEATING = 1,
    ONESHOT
} AnimType;

typedef struct {
    int first;
    int last;
    int cur;
    float ObjWidth;      // frame width
    float ObjHeight;     // frame height
    float speed;         // duration per frame in seconds
    float duration_left;
    Vector2 pos;         // screen position
    Vector2 scale;       // display scale (width, height)
    AnimType type;
    Texture2D Obj;       // Changed from reference to value
} Animobj;

class Anima {
private:
    std::vector<Animobj> _arr;

public:
    Anima() {};
    ~Anima() {};

    Animobj &get_obj(std::size_t pos) { return _arr[pos]; }
    void modify_obj(std::size_t pos, Animobj &info) { _arr[pos] = info; }
    void add_obj(Animobj info) { _arr.push_back(info); }
    void remove_obj(std::size_t pos) { UnloadTexture(_arr[pos].Obj); _arr.erase(_arr.begin() + pos); }
    std::size_t size() const { return _arr.size(); }

    void animate(Animobj &info, int numFramesPerRow = 1) {
        float dt = GetFrameTime();

        // Calculate source rectangle from spritesheet
        int x = (info.cur % numFramesPerRow) * info.ObjWidth;
        int y = (info.cur / numFramesPerRow) * info.ObjHeight;

        info.duration_left -= dt;
        if (info.duration_left <= 0) {
            info.duration_left = info.speed;
            info.cur++;

            if (info.cur > info.last) {
                switch (info.type) {
                    case REPEATING:
                        info.cur = info.first;
                        break;
                    case ONESHOT:
                        info.cur = info.last;
                        break;
                }
            }
        }

        Rectangle source = {
            .x = (float)x,
            .y = (float)y,
            .width = info.ObjWidth,
            .height = info.ObjHeight
        };

        Rectangle dest = {
            .x = info.pos.x,
            .y = info.pos.y,
            .width = info.scale.x,
            .height = info.scale.y
        };

        DrawTexturePro(info.Obj, source, dest, {0, 0}, 0.f, WHITE);
    }

    void animate(int numFramesPerRow = 1) {
        for (auto &i : _arr) {
            float dt = GetFrameTime();

            // Calculate source rectangle from spritesheet
            int x = (i.cur % numFramesPerRow) * i.ObjWidth;
            int y = (i.cur / numFramesPerRow) * i.ObjHeight;

            i.duration_left -= dt;
            if (i.duration_left <= 0) {
                i.duration_left = i.speed;
                i.cur++;

                if (i.cur > i.last) {
                    switch (i.type) {
                        case REPEATING:
                            i.cur = i.first;
                            break;
                        case ONESHOT:
                            i.cur = i.last;
                            break;
                    }
                }
            }

            Rectangle source = {
                .x = (float)x,
                .y = (float)y,
                .width = i.ObjWidth,
                .height = i.ObjHeight
            };

            Rectangle dest = {
                .x = i.pos.x,
                .y = i.pos.y,
                .width = i.scale.x,
                .height = i.scale.y
            };

            DrawTexturePro(i.Obj, source, dest, {0, 0}, 0.f, WHITE);
        }
    }
};

typedef struct {
    // int fd;
    std::string user;
    std::string text;
} user_t;

class ChatErrors : public std::exception {
    public:
    ChatErrors(const std::string& message) : message_(std::string("Text chat Error: ") + message) {}
    const char* what() const throw() { return message_.c_str(); }
    private:
    std::string message_;
};

class ChatServer {
private:
    int _serverFd;
    socklen_t info_len;
    struct sockaddr_in s_info;
    std::map<int, user_t> _users; // Maps fd to user info
    std::vector<struct pollfd> _id_tables;

public:
    int getFD() { return _serverFd; }
    std::vector<struct pollfd>& getIDtable() { return _id_tables; }

    ChatServer(std::size_t port = 8080) {
        _serverFd = socket(AF_INET, SOCK_STREAM, 0);
        if (_serverFd < 0)
            throw ChatErrors("socket creation failed...\n");

        /*
            // Allow socket reuse
            int opt = 1;
            setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        */

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
    }

    void addUser() {
        auto newfd = accept(_serverFd, (struct sockaddr*)&s_info, &info_len);
        if (newfd < 0)
            throw ChatErrors("Client couldn't connect\n");

        _id_tables.push_back((struct pollfd) {
            .fd = newfd,
            .events = POLLIN,
            .revents = 0
        });

        _users[newfd] = {/*newfd,*/ "", ""};
        // std::cout << "New connection on fd " << newfd << std::endl;
    }

    void streamMessage(int pos) {
        int fd = _id_tables[pos].fd;
        char buffer[2048];
        int size = read(fd, buffer, sizeof(buffer) - 1);

        if (size <= 0) {
            removeUser(pos);
            return;
        }
        while (size > 0 && (buffer[size-1] == '\n' || buffer[size-1] == '\r')) {
                buffer[--size] = '\0';
        }
        buffer[size] = '\0';
        // First message is username
        if (_users[fd].user.empty()) {
            _users[fd].user = std::string(buffer);
            std::cout << "User '" << _users[fd].user << "' joined (fd: " << fd << ")\n";

            // Notify others
            std::string joinMsg = _users[fd].user + " joined the chat\n";
            broadcastMessage(joinMsg/*, fd*/);
            joinMsg.clear();
        } else {
            // Regular message
            std::string msg = _users[fd].user + ": " + std::string(buffer);
            std::cout << msg;
            broadcastMessage(msg);
            msg.clear();
        }
    }

    void broadcastMessage(const std::string& msg/*, int excludeFd = -1*/) {
        for (std::size_t i = 1; i < _id_tables.size(); i++) {
            int fd = _id_tables[i].fd;
            // if (fd != excludeFd && !_users[fd].username.empty())
                write(fd, msg.c_str(), msg.size());
                std::cout << msg << std::endl;
        }
    }

    void removeUser(int &pos) {
        int fd = _id_tables[pos].fd;

        if (!_users[fd].user.empty()) {
            std::string leaveMsg = _users[fd].user + " left the chat\n";
            broadcastMessage(leaveMsg/*, fd*/);
        }

        pos = 0;
        _users.erase(fd);
        close(fd);
        _id_tables.erase(_id_tables.begin() + pos);
        // pos = 0;
    }

    ~ChatServer() {
        for (auto& pfd : _id_tables)
            close(pfd.fd);
    }
};

#include <cstddef>
#include <poll.h>
#include <vector>
#include <string>
#include <raylib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>
#include <exception>
#include <cstring>
#include <fcntl.h>

// typedef struct {
//     int fd;
//     std::string text;
//     std::string username;
// } user_t;

// class ChatErrors : public std::exception {
// public:
//     ChatErrors(const std::string& message) : message_(std::string("Chat Error: ") + message) {}
//     const char* what() const throw() { return message_.c_str(); }
// private:
//     std::string message_;
// };

class ChatClient {
private:
    int _clientFD;
    std::vector<user_t> _messages; // username, message
    std::string _nickname;
    bool _connected;
    char _inputBuffer[256];
    int _inputLength;

public:
    ChatClient(const std::string& ipAddress, int port, const std::string& nickname)
        : _nickname(nickname), _connected(false), _inputLength(0) {

        memset(_inputBuffer, 0, sizeof(_inputBuffer));

        _clientFD = socket(AF_INET, SOCK_STREAM, 0);
        if (_clientFD < 0) {
            throw ChatErrors("Failed to create socket");
        }

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);

        // Convert IP address from string to binary
        if (inet_pton(AF_INET, ipAddress.c_str(), &server_addr.sin_addr) <= 0) {
            close(_clientFD);
            throw ChatErrors("Invalid IP address: " + ipAddress);
        }

        if (connect(_clientFD, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
            close(_clientFD);
            throw ChatErrors("Failed to connect to server at " + ipAddress + ":" + std::to_string(port));
        }

        // Set socket to non-blocking
        int flags = fcntl(_clientFD, F_GETFL, 0);
        fcntl(_clientFD, F_SETFL, flags | O_NONBLOCK);

        // Send nickname
        write(_clientFD, nickname.c_str(), nickname.size());
        _connected = true;

        std::cout << "Connected to server as " << nickname << std::endl;
    }

    void receiveMessages() {
        if (!_connected) return;

        char buffer[2048];
        int size = read(_clientFD, buffer, sizeof(buffer) - 1);

        if (size > 0) {
            buffer[size] = '\0';
            std::string msg(buffer);

            // Parse message format "username: message"
            size_t colonPos = msg.find(':');
            if (colonPos != std::string::npos) {
                std::string user = msg.substr(0, colonPos);
                std::string text = msg.substr(colonPos + 2); // Skip ": "
                _messages.push_back({user, text});
            } else {
                // System message
                _messages.push_back({"System", msg});
            }
        } else if (size == 0) {
            _connected = false;
            _messages.push_back({"System", "Disconnected from server\n"});
        }
    }

    void sendMessage() {
        if (!_connected || !_inputLength) return;
        write(_clientFD, _inputBuffer, _inputLength);
    }

    const std::vector<user_t>& getMessages() const {
        return _messages;
    }

    bool isConnected() const { return _connected; }

    const std::string& getNickname() const { return _nickname; }

    char* getInputBuffer() { return _inputBuffer; }
    int& getInputLength() { return _inputLength; }

    ~ChatClient() {
        if (_clientFD >= 0) {
            close(_clientFD);
        }
    }
};

// Modern UI Chat Window
class ChatUI {
private:
    bool _isVisible;
    float _scrollOffset;
    Rectangle _chatBounds;
    Rectangle _toggleButtonBounds;
    Rectangle _inputBounds;
    Rectangle _sendButtonBounds;
    bool _inputActive;

    // Modern colors
    Color bgColor = {20, 20, 30, 245};
    Color headerColor = {30, 30, 45, 255};
    Color inputColor = {35, 35, 50, 255};
    Color accentColor = {88, 101, 242, 255};
    Color textColor = {220, 220, 230, 255};
    Color usernameColor = {255, 255, 255, 255};
    Color hoverColor = {100, 113, 255, 255};

public:
    ChatUI() : _isVisible(false), _scrollOffset(0), _inputActive(false) {
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();

        // Chat window in bottom-right
        float chatW = 380;
        float chatH = 500;
        _chatBounds = {screenW - chatW - 20, screenH - chatH - 20, chatW, chatH};

        // Toggle button (when chat is hidden)
        _toggleButtonBounds = {screenW - 80.0f, screenH - 80.0f, 60, 60};

        // Input area
        _inputBounds = {_chatBounds.x + 10, _chatBounds.y + _chatBounds.height - 60, _chatBounds.width - 90, 40};
        _sendButtonBounds = {_inputBounds.x + _inputBounds.width + 10, _inputBounds.y, 60, 40};
    }

    void update(int screenW, int screenH) {
        // Update positions if screen size changed
        float chatW = 380;
        float chatH = 500;
        _chatBounds = {screenW - chatW - 20, screenH - chatH - 20, chatW, chatH};
        _toggleButtonBounds = {screenW - 80.0f, screenH - 80.0f, 60, 60};
        _inputBounds = {_chatBounds.x + 10, _chatBounds.y + _chatBounds.height - 60, _chatBounds.width - 90, 40};
        _sendButtonBounds = {_inputBounds.x + _inputBounds.width + 10, _inputBounds.y, 60, 40};
    }

    bool handleToggleClick(Vector2 mousePos) {
        if (!_isVisible) {
            if (CheckCollisionPointRec(mousePos, _toggleButtonBounds)) {
                _isVisible = true;
                return true;
            }
        } else {
            Rectangle closeButton = {_chatBounds.x + _chatBounds.width - 35, _chatBounds.y + 5, 30, 30};
            if (CheckCollisionPointRec(mousePos, closeButton)) {
                _isVisible = false;
                _inputActive = false;
                return true;
            }
        }
        return false;
    }

    bool handleInputClick(Vector2 mousePos) {
        if (_isVisible && CheckCollisionPointRec(mousePos, _inputBounds)) {
            _inputActive = true;
            return true;
        }
        _inputActive = false;
        return false;
    }

    bool handleSendClick(Vector2 mousePos) {
        if (_isVisible && CheckCollisionPointRec(mousePos, _sendButtonBounds)) {
            return true;
        }
        return false;
    }

    void handleScroll(float wheel) {
        if (_isVisible && CheckCollisionPointRec(GetMousePosition(), _chatBounds)) {
            _scrollOffset += wheel * 20;
            if (_scrollOffset > 0) _scrollOffset = 0;
        }
    }

    void draw(ChatClient& client) {
        Vector2 mousePos = GetMousePosition();

        if (!_isVisible) {
            // Draw toggle button with glow effect
            bool hover = CheckCollisionPointRec(mousePos, _toggleButtonBounds);
            Color btnColor = hover ? hoverColor : accentColor;

            DrawCircle(_toggleButtonBounds.x + 30, _toggleButtonBounds.y + 30, 32, Fade(btnColor, 0.3f));
            DrawCircle(_toggleButtonBounds.x + 30, _toggleButtonBounds.y + 30, 28, btnColor);

            // Chat icon
            DrawRectangle(_toggleButtonBounds.x + 18, _toggleButtonBounds.y + 20, 24, 3, WHITE);
            DrawRectangle(_toggleButtonBounds.x + 18, _toggleButtonBounds.y + 28, 24, 3, WHITE);
            DrawRectangle(_toggleButtonBounds.x + 18, _toggleButtonBounds.y + 36, 24, 3, WHITE);

            // Notification badge if there are messages
            if (!client.getMessages().empty()) {
                DrawCircle(_toggleButtonBounds.x + 50, _toggleButtonBounds.y + 15, 8, RED);
            }
            return;
        }

        // Draw chat window with shadow
        DrawRectangle(_chatBounds.x + 4, _chatBounds.y + 4, _chatBounds.width, _chatBounds.height, Fade(BLACK, 0.3f));
        DrawRectangleRounded(_chatBounds, 0.05f, 10, bgColor);

        // Header
        Rectangle header = {_chatBounds.x, _chatBounds.y, _chatBounds.width, 45};
        DrawRectangleRounded(header, 0.05f, 10, headerColor);
        DrawRectangle(_chatBounds.x, _chatBounds.y + 30, _chatBounds.width, 15, headerColor);

        DrawText("Chat", _chatBounds.x + 15, _chatBounds.y + 12, 20, textColor);

        // Status indicator
        Color statusColor = client.isConnected() ? GREEN : RED;
        DrawCircle(_chatBounds.x + 70, _chatBounds.y + 22, 5, statusColor);

        // Close button
        Rectangle closeBtn = {_chatBounds.x + _chatBounds.width - 35, _chatBounds.y + 5, 30, 30};
        bool closeHover = CheckCollisionPointRec(mousePos, closeBtn);
        DrawRectangleRounded(closeBtn, 0.3f, 10, closeHover ? Fade(RED, 0.8f) : Fade(RED, 0.5f));
        DrawText("X", closeBtn.x + 9, closeBtn.y + 6, 18, WHITE);

        // Messages area
        Rectangle msgArea = {_chatBounds.x + 10, _chatBounds.y + 55, _chatBounds.width - 20, _chatBounds.height - 125};
        DrawRectangleRounded(msgArea, 0.05f, 10, Fade(BLACK, 0.2f));

        // Scissor mode for scrollable content
        BeginScissorMode(msgArea.x, msgArea.y, msgArea.width, msgArea.height);

        float yPos = msgArea.y + 10 + _scrollOffset;
        const auto& messages = client.getMessages();

        for (const auto& msg : messages) {
            // Username in bold (simulated with shadow)
            std::string username = msg.user + ":";
            // std::cout << "username: " << username << "message: " << msg.text << std::endl;
            DrawText(username.c_str(), msgArea.x + 11, yPos + 1, 16, Fade(BLACK, 0.5f));
            DrawText(username.c_str(), msgArea.x + 10, yPos, 16, usernameColor);

            // Message text (word wrap)
            std::string text = msg.text;
            int textWidth = msgArea.width - 20;
            int xOffset = msgArea.x + 10;
            yPos += 22;

            // Simple word wrap
            std::string line = "";
            for (char c : text) {
                if (c == '\n' || MeasureText((line + c).c_str(), 14) > textWidth) {
                    DrawText(line.c_str(), xOffset, yPos, 14, textColor);
                    yPos += 18;
                    line = (c == '\n') ? "" : std::string(1, c);
                } else {
                    line += c;
                }
            }
            if (!line.empty()) {
                DrawText(line.c_str(), xOffset, yPos, 14, textColor);
                yPos += 18;
            }

            yPos += 8; // Space between messages
        }

        EndScissorMode();

        // Scroll indicator
        if (_scrollOffset < 0) {
            DrawRectangle(msgArea.x + msgArea.width - 6, msgArea.y + 5, 4, 30, accentColor);
        }

        // Input box
        bool inputHover = CheckCollisionPointRec(mousePos, _inputBounds);
        Color inputBorder = _inputActive ? accentColor : (inputHover ? Fade(accentColor, 0.5f) : Fade(textColor, 0.3f));

        DrawRectangleRounded(_inputBounds, 0.15f, 10, inputColor);
        DrawRectangleRoundedLines(_inputBounds, 0.15f, 10, inputBorder);



        // Draw input text
        std::string inputText(client.getInputBuffer());
        if (!inputText.empty()) {
            DrawText(inputText.c_str(), _inputBounds.x + 10, _inputBounds.y + 11, 16, textColor);
        } else if (_inputActive) {
            DrawText("Type a message...", _inputBounds.x + 10, _inputBounds.y + 11, 16, Fade(textColor, 0.4f));
        }

        // Cursor
        if (_inputActive && ((int)(GetTime() * 2) % 2 == 0)) {
            int cursorX = _inputBounds.x + 10 + MeasureText(inputText.c_str(), 16);
            DrawRectangle(cursorX, _inputBounds.y + 8, 2, 24, textColor);
        }

        // Send button
        bool sendHover = CheckCollisionPointRec(mousePos, _sendButtonBounds);
        DrawRectangleRounded(_sendButtonBounds, 0.15f, 10, sendHover ? hoverColor : accentColor);
        DrawText("SEND", _sendButtonBounds.x + 8, _sendButtonBounds.y + 11, 16, WHITE);
    }

    bool isVisible() const { return _isVisible; }
    bool isInputActive() const { return _inputActive; }
    void setInputActive(bool active) { _inputActive = active; }
};

class Chat {
private:
    ChatUI _ui;
    ChatClient _client;

public:
    Chat(const std::string& ipAddress, int port, const std::string& nickname) :
        _client(ipAddress, port, nickname) {}
    ChatClient &getChatClient() { return _client; };
    ChatUI &getChatUi() { return _ui; };

    void update(int width, int height) {
        _ui.update(width, height);
        _client.receiveMessages();
    };

    void handle_mouse(bool cond) {
        if (!cond)
            return;
        Vector2 mousePos = GetMousePosition();

        if (_ui.handleToggleClick(mousePos)) {
            // Toggled
        } else if (_ui.handleSendClick(mousePos)) {
            // std::string msg(client.getInputBuffer());
            if (_client.getInputLength()) {
                _client.sendMessage();//(msg);
                memset(_client.getInputBuffer(), 0, 256);
                _client.getInputLength() = 0;
            }
        } else
            _ui.handleInputClick(mousePos);
    };

    void handleScroll() {
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            _ui.handleScroll(wheel);
        }
    };

    void handleKeyBoardInput() {
        if (_ui.isInputActive()) {
            int key = GetCharPressed();
            while (key > 0) {
                if (key >= 32 && key <= 125 && _client.getInputLength() < 255) {
                    _client.getInputBuffer()[_client.getInputLength()] = (char)key;
                    _client.getInputLength()++;
                }
                key = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE) && _client.getInputLength() > 0) {
                _client.getInputLength()--;
                _client.getInputBuffer()[_client.getInputLength()] = '\0';
            }

            if (IsKeyPressed(KEY_ENTER)) {
                // std::string msg(_client.getInputBuffer());
                if (_client.getInputLength()) {
                    _client.sendMessage();//(msg);
                    memset(_client.getInputBuffer(), 0, 256);
                    _client.getInputLength() = 0;
                }
            }
        }
    };

    void draw() {
        _ui.draw(_client);
    };

    ~Chat() {};
};


int main(int argc, char* argv[]) {
    std::string ipAddress = "127.0.0.1";
    int port = 8080;
    std::string nickname = "User";

    if (argc > 1) {
        // ipAddress = argv[1];
        // port = std::stoi(argv[2]);
        nickname = argv[1];
    }

    ChatClient client(ipAddress, port, nickname);
    try {
        Chat chat(ipAddress, port, nickname);

        InitWindow(800, 600, "Modern Chat Client");
        SetTargetFPS(60);

        while (!WindowShouldClose()) {
            chat.update(GetScreenWidth(), GetScreenHeight());
            chat.handle_mouse(IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
            chat.handleScroll();
            chat.handleKeyBoardInput();

            BeginDrawing();

            ClearBackground(BLACK);
            chat.draw();

            EndDrawing();
        }

        CloseWindow();
    } catch (const ChatErrors& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}

/*
int main() {
    ChatServer chat(8080);

    auto &id_table = chat.getIDtable();
    while (1) {
        poll(id_table.data(), id_table.size(), 100);

        // Iterate backwards to safely handle removals
        for (int i = id_table.size() - 1; i >= 0; i--) {
            // Check for disconnect/error first
            if (id_table[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
                if (i > 0) {  // Don't remove server socket
                    chat.removeUser(i);
                }
            }
            // Only process POLLIN if we didn't just remove this user
            else if (id_table[i].revents & POLLIN) {
                if (i == 0) {
                    try {
                        chat.addUser();
                    } catch(const ChatErrors& e) {
                        std::cerr << e.what();
                    }
                } else {
                    chat.streamMessage(i);  // No try-catch needed now
                }
            }
        }
    }
    return 0;
}
*/
