/*
 *
 *
 *
 *
 */

 #ifndef CHATUI_H_
    #define CHATUI_H_
    #include <raylib.h>
    #include "chatClient.hpp"

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
    ChatUI();

    void update(int screenW, int screenH);

    bool handleToggleClick(Vector2 mousePos);

    bool handleInputClick(Vector2 mousePos);

    bool handleSendClick(Vector2 mousePos);

    void handleScroll(float wheel);

    void draw(ChatClient& client);

 bool isVisible() const { return _isVisible; }
bool isInputActive() const { return _inputActive; }
void setInputActive(bool active) { _inputActive = active; }
void setVisibility(bool visible) { _isVisible = visible; } 
};

#endif /*  */
