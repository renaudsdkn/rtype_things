/*
 *
 *
 */


#include "../include/client/chatUI.hpp"

#include <raylib.h>

 ChatUI::ChatUI() : _isVisible(false), _scrollOffset(0), _inputActive(false) {
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

 void ChatUI::update(int screenW, int screenH)  {
     // Update positions if screen size changed
     float chatW = 380;
     float chatH = 500;
     _chatBounds = {screenW - chatW - 20, screenH - chatH - 20, chatW, chatH};
     _toggleButtonBounds = {screenW - 80.0f, screenH - 80.0f, 60, 60};
     _inputBounds = {_chatBounds.x + 10, _chatBounds.y + _chatBounds.height - 60, _chatBounds.width - 90, 40};
     _sendButtonBounds = {_inputBounds.x + _inputBounds.width + 10, _inputBounds.y, 60, 40};
 }

 bool ChatUI::handleToggleClick(Vector2 mousePos) {
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

bool ChatUI::handleInputClick(Vector2 mousePos) {
    if (_isVisible && CheckCollisionPointRec(mousePos, _inputBounds)) {
        _inputActive = true;
        return true;
    }
    _inputActive = false;
    return false;
}

bool ChatUI::handleSendClick(Vector2 mousePos) {
    if (_isVisible && CheckCollisionPointRec(mousePos, _sendButtonBounds)) {
        return true;
    }
    return false;
}

void ChatUI::handleScroll(float wheel) {
    if (_isVisible && CheckCollisionPointRec(GetMousePosition(), _chatBounds)) {
        _scrollOffset += wheel * 20;
        if (_scrollOffset > 0) _scrollOffset = 0;
    }
}

void ChatUI::draw(ChatClient& client) {
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
    DrawRectangleRoundedLines(_inputBounds, 0.15f, 10, 2.0f, inputBorder);



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
