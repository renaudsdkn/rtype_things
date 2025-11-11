/*
 *
 *
 *
*/

#include "../include/client/chat.hpp"
void Chat::update(int width, int height) {
    _ui.update(width, height);
    _client.receiveMessages();
};

void Chat::handle_mouse(bool cond) {
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
}

void Chat::handleScroll() {
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        _ui.handleScroll(wheel);
    }
}

void Chat::handleKeyBoardInput() {
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
}
