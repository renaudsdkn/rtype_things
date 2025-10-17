#include "../include/client/GraphicsEngine.hpp"
#include <iostream>
#include "raylib.h"

GraphicsEngine::GraphicsEngine(int width, int height, const std::string& title)
{
    InitWindow(width, height, title.c_str());
    SetTargetFPS(60);
    m_nextTextureId = 1;
}

GraphicsEngine::~GraphicsEngine()
{
    for (auto const& [id, texturePtr] : m_textures) {
        UnloadTexture(*static_cast<Texture2D*>(texturePtr));
        delete static_cast<Texture2D*>(texturePtr);
    }
    CloseWindow();
}

void GraphicsEngine::init() {

}

bool GraphicsEngine::shouldClose() const {
    return WindowShouldClose();
}

void GraphicsEngine::beginDrawing() {
    return BeginDrawing();
}

void GraphicsEngine::endDrawing() {
    return EndDrawing();
}

void GraphicsEngine::clearScreen(int r, int g, int b) {
    ClearBackground({ (unsigned char)r, (unsigned char)g, (unsigned char)b });
}

int GraphicsEngine::loadTexture(const std::string& path) {
    Texture2D *texture = new Texture2D(LoadTexture(path.c_str()));

    if (texture->id == 0) {
        std::cerr << "Erreur: Impossible de charger la texture: " << path << std::endl;
        delete texture;
        return -1;
    }

    int newId = m_nextTextureId++;
    m_textures[newId] = texture;
    return newId;
}

void GraphicsEngine::drawTexture(int textureId, float x, float y) {
    auto it = m_textures.find(textureId);
    if (it != m_textures.end()) {
        Texture2D *texture = static_cast<Texture2D*>(it->second);
        DrawTexture(*texture, (int)x, (int)y, WHITE);
    } else {
        DrawRectangle((int)x, (int)y, 32, 32, RED);
        //DrawRectangle((float y))
    }
}
