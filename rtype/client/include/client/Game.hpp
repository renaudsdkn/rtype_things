// Dans Game.hpp
#pragma once

#include "raylib.h"
#include "SpriteManager.hpp"
#include "Renderer.hpp"
// #include "GameState.hpp" // SUPPRIMÉ
#include "Starfield.hpp"
// ✅ NOUVEAUX INCLUDES
#include "client.hpp"       // Pour RTypeClient (réseau)
#include "InputManager.hpp" // Pour InputManager
#include "GameClient.hpp"   // Pour GameClient (logique + état ECS)
#include "Anima.hpp"
#include "chat.hpp"

class Game
{
public:
  Game(int screenWidth, int screenHeight, const std::string &serverIp = "127.0.0.1",
       unsigned short udpPort = 1234,
       unsigned short tcpPort = 1235);
  ~Game();
  void run(); // Boucle principale

private:
  // Garde les méthodes de la boucle, le contenu changera
  void handleInput(int &lobbySelectionIndex); // Prend l'index de sélection du lobby
  void updateGameLogic();                     // Renommé depuis update
  void render();
  void handleNicknameInput(std::string &nicknameBuffer);
  void initAnimations();
  void handleRoomCreationInput(int &selectedOption); // ✅ NOUVEAU
  int screenWidth;
  int screenHeight;

  // --- Composants Graphiques (conservés) ---
  SpriteManager spriteManager;
  Renderer renderer;
  Starfield starfield;

  // --- ✅ NOUVEAUX Composants Logique & Réseau ---
  InputManager inputManager;
  RTypeClient networkClient;
  GameClient gameClient;    // Gère l'ECS et la synchro
  std::string m_serverIp;   // ✅ NOUVEAU
  unsigned short m_tcpPort; // ✅ NOUVEAU

  float backgroundOffset = 0.0f;
  float backgroundSpeed = 100.0f;
  Anima anima;
  // ✅ MODIFIER : Chat optionnel (créé après validation pseudo)
  std::unique_ptr<Chat> m_chat; // nullptr tant que pas connecté

  // ✅ NOUVEAU : Initialise le chat avec le bon pseudo
  void initializeChat(const std::string &nickname); // État du chat (ouvert/fermé)

  // ❌ SUPPRIMÉ : GameState game;
  // ❌ SUPPRIMÉ : Texture2D missileSprite, enemySprite (sera géré par Renderer/SpriteManager)
};