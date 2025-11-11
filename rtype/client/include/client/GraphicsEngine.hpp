#pragma once

#include <string>
#include <map>
#include <memory>

class GraphicsEngine {
    public:
        GraphicsEngine(int width, int height, const std::string& title);
        ~GraphicsEngine();

        void init();
        bool shouldClose() const;
        void beginDrawing();
        void endDrawing();
        void clearScreen(int r, int g, int b);

        int loadTexture(const std::string& path);
        void drawTexture(int textureId, float x, float y);

    private:
        std::map<int, void*> m_textures;
        int m_nextTextureId = 0;
};