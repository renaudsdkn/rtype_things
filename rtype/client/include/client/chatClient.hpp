/*
 *
 *
 */


 #ifndef CHAT_CLIENT_H_
    #define CHAT_CLIENT_H_
    #include <cstddef>
    #include <poll.h>
    #include <vector>
    #include <string>
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <iostream>
    #include <exception>
    #include <cstring>
    #include <fcntl.h>

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

class ChatClient {
private:
    int _clientFD;
    std::vector<user_t> _messages; // username, message
    std::string _nickname;
    bool _connected;
    char _inputBuffer[256];
    int _inputLength;

public:
    ChatClient(const std::string& ipAddress, int port, const std::string& nickname);

    void receiveMessages();

    void sendMessage();

    const std::vector<user_t>& getMessages() const { return _messages; }

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

#endif /* */
