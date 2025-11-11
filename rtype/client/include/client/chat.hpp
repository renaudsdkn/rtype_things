/*
 *
 *
 */

#ifndef CHAT_H_
    #define CHAT_H_
    #include "chatUI.hpp"

class Chat {
    private:
        ChatUI _ui;
        ChatClient _client;

    public:
        Chat(const std::string& ipAddress, int port, const std::string& nickname) :
        _client(ipAddress, port, nickname) {}
        ChatClient &getChatClient() { return _client; };
        ChatUI &getChatUi() { return _ui; };
        void update(int width, int height);
        void handle_mouse(bool cond);
        void handleScroll();
        void handleKeyBoardInput();
        void draw() { _ui.draw(_client); };
        ~Chat() {};
};

#endif /*  */
