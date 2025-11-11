// Dans Game.cpp
#include "../include/client/Game.hpp"
#include <iostream> // Pour les logs de debug

void Game::initAnimations()
{
    // --- Animation de l'ennemi ---
    Animobj enemyAnim;
    enemyAnim.first = 0;
    enemyAnim.last = 3; // 4 frames (0,1,2,3)
    enemyAnim.cur = 0;
    enemyAnim.Obj = spriteManager.getSprite("enemy");
    enemyAnim.ObjWidth = enemyAnim.Obj.width / 4; // si 4 frames horizontales
    enemyAnim.ObjHeight = enemyAnim.Obj.height;
    enemyAnim.speed = 0.15f; // vitesse d’animation
    enemyAnim.duration_left = enemyAnim.speed;
    enemyAnim.pos = {500, 300};
    enemyAnim.scale = {enemyAnim.ObjWidth * 0.5f, enemyAnim.ObjHeight * 0.5f};
    enemyAnim.type = REPEATING;
    anima.add_obj(enemyAnim);

    // --- Animation du BigBullet ---
    Animobj bigBulletAnim;
    bigBulletAnim.first = 0;
    bigBulletAnim.last = 5; // exemple : 6 frames horizontales
    bigBulletAnim.cur = 0;
    bigBulletAnim.Obj = spriteManager.getSprite("bigbullet");
    bigBulletAnim.ObjWidth = bigBulletAnim.Obj.width / 6; // si 6 frames horizontales
    bigBulletAnim.ObjHeight = bigBulletAnim.Obj.height;
    bigBulletAnim.speed = 0.1f;
    bigBulletAnim.duration_left = bigBulletAnim.speed;
    bigBulletAnim.pos = {700, 400};
    bigBulletAnim.scale = {bigBulletAnim.ObjWidth * 0.3f, bigBulletAnim.ObjHeight * 0.3f};
    bigBulletAnim.type = REPEATING;
    anima.add_obj(bigBulletAnim);
}

Game::Game(int screenWidth, int screenHeight, const std::string &serverIp,
           unsigned short udpPort,
           unsigned short tcpPort)
    : screenWidth(screenWidth), screenHeight(screenHeight), m_serverIp(serverIp), // ✅ STOCKER
      m_tcpPort(tcpPort),
      spriteManager(),
      renderer(spriteManager, screenWidth, screenHeight),
      starfield(screenWidth, screenHeight),
      inputManager(),
      networkClient(serverIp, udpPort),
      gameClient(inputManager, networkClient),
      m_chat(nullptr)
{
    InitWindow(screenWidth, screenHeight, "R-Type - Client Réseau");
    InitAudioDevice();
    initAnimations();
    SetTargetFPS(60);
    spriteManager.loadAssets();

    // Callbacks...
    networkClient.setWelcomeHandler(
        [&](uint32_t id, const ProtocolData::Welcome &welcomeData)
        { this->gameClient.handleWelcome(id, welcomeData);
            if (welcomeData.accepted == 1) {
                std::string confirmedNickname(welcomeData.confirmedNickname);
                this->initializeChat(confirmedNickname);
            } });

    networkClient.setSnapshotHandler(
        [&](const ProtocolData::Snapshot &snap)
        { this->gameClient.updateFromServer(snap); });

    networkClient.setPlayerEventHandler(
        [&](const ProtocolData::PlayerEvent &event)
        { this->gameClient.handlePlayerEvent(event); });

    networkClient.setRoomListHandler(
        [&](const ProtocolData::RoomList &list)
        { this->gameClient.handleRoomList(list); });

    networkClient.setRoomResponseHandler(
        [&](ProtocolData::MessageType type, const ProtocolData::RoomResponse &resp)
        { this->gameClient.handleRoomResponse(type, resp); });

    networkClient.setPlayerNotificationHandler(
        [&](ProtocolData::MessageType type, const ProtocolData::PlayerRoomNotification &notif)
        { this->gameClient.handlePlayerNotification(type, notif); });

    networkClient.setGameStartingHandler(
        [&]()
        { this->gameClient.handleGameStarting(); });

    networkClient.setDeltaSnapshotHandler(
        [&](const ProtocolData::DeltaSnapshot &delta)
        {
            this->gameClient.applyDeltaSnapshot(delta);
        });

    // ✅ CORRECTION : Démarre le client réseau UNE SEULE FOIS
    networkClient.start();

    std::cout << "[Game] Client réseau démarré (en attente du pseudo)." << std::endl;
}
// --- Destructeur ---
Game::~Game()
{
    spriteManager.unload();
    CloseAudioDevice();
    CloseWindow();
}
void Game::run()
{
    std::cout << "[Game] Démarrage de la boucle principale..." << std::endl;

    float lobbyRefreshTimer = 0.0f;
    const float lobbyRefreshInterval = 2.0f;
    int lobbySelectionIndex = 0;
    int roomCreationSelectedOption = 0;
    std::string nicknameInput = "";

    while (!WindowShouldClose())
    {
        gameClient.updateNetworkStats();
        gameClient.updatePrediction();

        GameClient::State currentState = gameClient.getState();

        // ✅ Réinitialise le buffer si on revient à ENTERING_NICKNAME
        static GameClient::State previousState = currentState;
        if (currentState == GameClient::State::ENTERING_NICKNAME &&
            previousState != GameClient::State::ENTERING_NICKNAME)
        {
            nicknameInput = gameClient.getTemporaryNickname();
        }
        previousState = currentState;

        // ═══════════════════════════════════════════════════════
        // SECTION 1 : GESTION DES INPUTS (AVANT le rendu)
        // ═══════════════════════════════════════════════════════
        if (currentState == GameClient::State::ENTERING_NICKNAME)
        {
            handleNicknameInput(nicknameInput);
        }
        else if (currentState == GameClient::State::CREATING_ROOM)
        {
            handleRoomCreationInput(roomCreationSelectedOption);
        }
        else
        {
            handleInput(lobbySelectionIndex);
        }

        // ═══════════════════════════════════════════════════════
        // SECTION 2 : RENDU (ENTRE BeginDrawing et EndDrawing)
        // ═══════════════════════════════════════════════════════
        BeginDrawing();
        ClearBackground(BLACK);

        // Background animé
        Texture2D background = spriteManager.getSprite("background");
        float bgWidth = (float)background.width;
        float bgHeight = (float)background.height;

        backgroundOffset += backgroundSpeed * GetFrameTime();
        if (backgroundOffset >= bgWidth)
            backgroundOffset = 0.0f;

        Rectangle src1 = {backgroundOffset, 0, (float)screenWidth, (float)screenHeight};
        Rectangle dest1 = {0, 0, (float)screenWidth, (float)screenHeight};
        DrawTexturePro(background, src1, dest1, {0, 0}, 0.0f, WHITE);

        if (backgroundOffset + screenWidth > bgWidth)
        {
            float overflow = (backgroundOffset + screenWidth) - bgWidth;
            Rectangle src2 = {0, 0, overflow, (float)screenHeight};
            Rectangle dest2 = {bgWidth - backgroundOffset, 0, overflow, (float)screenHeight};
            DrawTexturePro(background, src2, dest2, {0, 0}, 0.0f, WHITE);
        }

        // Animations
        anima.animate(anima.get_obj(0), 4);
        anima.animate(anima.get_obj(1), 6);

        // ═══════════════════════════════════════════════════════
        // RENDU SELON L'ÉTAT
        // ═══════════════════════════════════════════════════════
        if (currentState == GameClient::State::ENTERING_NICKNAME)
        {
            renderer.renderNicknameEntry(nicknameInput, gameClient.getNicknameRejectionReason());
        }
        else if (currentState == GameClient::State::CREATING_ROOM)
        {
            renderer.renderRoomCreationScreen(gameClient.getRoomConfig(), roomCreationSelectedOption);
        }
        else if (currentState == GameClient::State::PLAYING ||
                 currentState == GameClient::State::GAME_OVER)
        {
            RenderData renderData = gameClient.getRenderData();
            renderer.renderEntities(renderData);

            GameClient::PlayerLocalStats playerStats = gameClient.getLocalPlayerStats();
            renderer.renderPlayerHUD(playerStats);
            renderer.renderPlayerName(gameClient.getConfirmedNickname());
            float fps = GetFPS();
            renderer.renderNetworkStats(gameClient.getNetworkStats(), fps);

            // ✅ Hint chat (si fermé)
            if (currentState == GameClient::State::PLAYING &&
                m_chat && !m_chat->getChatUi().isVisible())
            {
                DrawText("Appuyez sur [T] pour ouvrir le chat",
                         screenWidth / 2 - 150,
                         screenHeight - 30,
                         16,
                         Fade(WHITE, 0.7f));
            }

            if (currentState == GameClient::State::GAME_OVER)
            {
                renderer.renderGameOverScreen();
            }
        }
        else if (currentState == GameClient::State::LOBBY)
        {
            auto roomList = gameClient.getRoomList();
            renderer.renderLobby(roomList, lobbySelectionIndex);

            lobbyRefreshTimer += GetFrameTime();
            if (lobbyRefreshTimer > lobbyRefreshInterval)
            {
                gameClient.requestListRooms();
                lobbyRefreshTimer = 0.0f;
            }
        }
        else if (currentState == GameClient::State::WAITING_IN_ROOM)
        {
            renderer.renderWaitingRoom(
                gameClient.getCurrentRoomId(),
                gameClient.getPlayerNicknamesInRoom());
        }
        else if (currentState == GameClient::State::CONNECTING)
        {
            DrawText("Connexion au serveur...", 10, 10, 20, WHITE);
        }

        // ═══════════════════════════════════════════════════════
        // ✅ RENDU DU CHAT (PAR-DESSUS TOUT, AVANT EndDrawing)
        // ═══════════════════════════════════════════════════════
        if (m_chat)
        {
            bool chatVisible = m_chat->getChatUi().isVisible();
            std::cout << "[DEBUG run] m_chat existe, ChatUI.isVisible = "
                      << (chatVisible ? "true" : "false") << std::endl;

            if (chatVisible)
            {

                std::cout << "[DEBUG run] 🖼️ APPEL draw()..." << std::endl;
                m_chat->draw();

                std::cout << "[DEBUG run] ✅ Chat dessiné avec succès" << std::endl;
            }
            else
            {
                std::cout << "[DEBUG run] ⏸️ Chat invisible, pas de rendu" << std::endl;
            }
        }
        else
        {
            std::cout << "[DEBUG run] ❌ m_chat est NULL" << std::endl;
        }

        EndDrawing(); // ← TOUT doit être dessiné AVANT cette ligne
    }

    std::cout << "[Game] Fin de la boucle principale." << std::endl;
    networkClient.send_disconnect();
    networkClient.stop();
}
// --- Mise à Jour Logique (Modifiée) ---
void Game::updateGameLogic()
{
    backgroundOffset += backgroundSpeed * GetFrameTime();
    Texture2D background = spriteManager.getSprite("background");
    if (backgroundOffset >= background.width)
        backgroundOffset = 0.0f;
    // Ne met à jour la prédiction que si on est en jeu
    if (gameClient.getState() == GameClient::State::PLAYING)
    {
        gameClient.updatePrediction();
    }
}

// --- Gestion des Inputs (Modifiée) ---
void Game::handleInput(int &lobbySelectionIndex)
{
    GameClient::State currentState = gameClient.getState();

    if (currentState == GameClient::State::LOBBY)
    {
        // Gère les inputs du LOBBY (via Raylib)
        if (IsKeyPressed(KEY_R))
        {
            gameClient.requestListRooms();
        }
        if (IsKeyPressed(KEY_C))
        {
            std::cout << "[Game] Ouverture écran création de room" << std::endl;
            gameClient.initDefaultRoomConfig();
            gameClient.setState(GameClient::State::CREATING_ROOM);
            return;
        }

        auto roomList = gameClient.getRoomList();
        if (!roomList.empty())
        {
            if (IsKeyPressed(KEY_DOWN))
                lobbySelectionIndex = (lobbySelectionIndex + 1) % roomList.size();
            if (IsKeyPressed(KEY_UP))
                lobbySelectionIndex = (lobbySelectionIndex - 1 + roomList.size()) % roomList.size();
            if (IsKeyPressed(KEY_ENTER))
            {
                gameClient.requestJoinRoom(roomList[lobbySelectionIndex].roomId);
            }
        }
    }
    else if (currentState == GameClient::State::PLAYING)
    {
        // ═══════════════════════════════════════════════
        // 1. GESTION DU CHAT (priorité haute)
        // ═══════════════════════════════════════════════
        if (m_chat)
        {
            std::cout << "[DEBUG handleInput] m_chat existe ✅" << std::endl;

            // ✅ Détecte la touche T
            if (IsKeyPressed(KEY_T))
            {
                std::cout << "[DEBUG handleInput] ⌨️ Touche T détectée !" << std::endl;

                // ✅ Toggle directement dans ChatUI
                bool wasVisible = m_chat->getChatUi().isVisible();
                m_chat->getChatUi().setVisibility(!wasVisible);

                std::cout << "[DEBUG handleInput] Chat maintenant "
                          << (!wasVisible ? "ACTIF ✅" : "INACTIF ❌") << std::endl;
            }

            // ✅ Vérifie l'état du ChatUI
            if (m_chat->getChatUi().isVisible())
            {
                std::cout << "[DEBUG handleInput] 📝 Traitement inputs chat..." << std::endl;
                std::cout << "[DEBUG run] 🎨 APPEL update()..." << std::endl;
                m_chat->update(screenWidth, screenHeight);
                m_chat->handle_mouse(IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
                m_chat->handleScroll();
                m_chat->handleKeyBoardInput();

                std::cout << "[DEBUG handleInput] ⏹️ BLOQUE inputs jeu" << std::endl;
                return; // ← Bloque les inputs de jeu
            }
        }
        else
        {
            std::cout << "[DEBUG handleInput] ❌ m_chat est NULL !" << std::endl;
        }

        // ═══════════════════════════════════════════════
        // 2. GESTION DU JEU (seulement si chat fermé)
        // ═══════════════════════════════════════════════
        std::cout << "[DEBUG handleInput] 🎮 Traitement inputs jeu..." << std::endl;
        gameClient.processInput();
    }
    else if (currentState == GameClient::State::GAME_OVER)
    {
        // Gère les inputs de l'écran Game Over
        if (IsKeyPressed(KEY_R))
        {
            gameClient.setState(GameClient::State::LOBBY);
            gameClient.cleanupGameEntities();
            gameClient.requestListRooms();
        }
    }
    else if (currentState == GameClient::State::WAITING_IN_ROOM)
    {
        if (IsKeyPressed(KEY_Q))
        {
            gameClient.requestLeaveRoom();
        }
        if (IsKeyPressed(KEY_SPACE))
        {
            // gameClient.requestStartGame();
        }
    }
}
void Game::handleNicknameInput(std::string &nicknameBuffer)
{
    // Gère les caractères tapés
    int key = GetCharPressed();
    while (key > 0)
    {
        if (key >= 32 && key <= 126 && nicknameBuffer.length() < 20)
        {
            nicknameBuffer += (char)key;
        }
        key = GetCharPressed();
    }

    // Gère la touche Backspace
    if (IsKeyPressed(KEY_BACKSPACE) && !nicknameBuffer.empty())
    {
        nicknameBuffer.pop_back();
    }

    // Gère la validation (touche Entrée)
    if (IsKeyPressed(KEY_ENTER) && !nicknameBuffer.empty())
    {
        std::cout << "[Game] Envoi du pseudo: '" << nicknameBuffer << "'" << std::endl;

        // Change l'état en CONNECTING
        gameClient.setState(GameClient::State::CONNECTING);
        gameClient.setTemporaryNickname(nicknameBuffer);
        gameClient.clearNicknameRejectionReason();

        // ✅ CORRECTION : Envoie CONNECT directement (sans start)
        networkClient.send_connect(nicknameBuffer);

        // Efface le buffer
        nicknameBuffer = "";
    }
}
void Game::handleRoomCreationInput(int &selectedOption)
{
    ProtocolData::RoomConfig &config = gameClient.getRoomConfig();

    // Navigation entre les options
    if (IsKeyPressed(KEY_UP))
    {
        selectedOption = (selectedOption - 1 + 8) % 8; // 8 options au total
    }
    if (IsKeyPressed(KEY_DOWN))
    {
        selectedOption = (selectedOption + 1) % 8;
    }

    // Modification selon l'option sélectionnée
    switch (selectedOption)
    {
    case 0: // Nom de la room
    {
        int key = GetCharPressed();
        while (key > 0)
        {
            size_t len = strlen(config.roomName);
            if (key >= 32 && key <= 126 && len < 31)
            {
                config.roomName[len] = (char)key;
                config.roomName[len + 1] = '\0';
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE))
        {
            size_t len = strlen(config.roomName);
            if (len > 0)
            {
                config.roomName[len - 1] = '\0';
            }
        }
        break;
    }

    case 1: // Difficulté
    {
        if (IsKeyPressed(KEY_LEFT))
        {
            if (config.difficulty > 0)
                config.difficulty--;
        }
        if (IsKeyPressed(KEY_RIGHT))
        {
            if (config.difficulty < 2)
                config.difficulty++;
        }
        break;
    }

    case 2: // Max joueurs
    {
        if (IsKeyPressed(KEY_LEFT))
        {
            if (config.maxPlayers > 2)
                config.maxPlayers--;
        }
        if (IsKeyPressed(KEY_RIGHT))
        {
            if (config.maxPlayers < 6)
                config.maxPlayers++;
        }
        break;
    }

    case 3: // Vitesse ennemis
    {
        if (IsKeyPressed(KEY_LEFT) || IsKeyDown(KEY_LEFT))
        {
            if (config.enemySpeedMultiplier > 50)
            {
                config.enemySpeedMultiplier -= 5;
            }
        }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyDown(KEY_RIGHT))
        {
            if (config.enemySpeedMultiplier < 150)
            {
                config.enemySpeedMultiplier += 5;
            }
        }
        break;
    }

    case 4: // Spawn rate
    {
        if (IsKeyPressed(KEY_LEFT) || IsKeyDown(KEY_LEFT))
        {
            if (config.spawnRateMultiplier > 50)
            {
                config.spawnRateMultiplier -= 5;
            }
        }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyDown(KEY_RIGHT))
        {
            if (config.spawnRateMultiplier < 200)
            {
                config.spawnRateMultiplier += 5;
            }
        }
        break;
    }

    case 5: // Tir ami
    {
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER))
        {
            config.friendlyFire = !config.friendlyFire;
        }
        break;
    }

    case 6: // Power-ups
    {
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER))
        {
            config.powerUpsEnabled = !config.powerUpsEnabled;
        }
        break;
    }

    case 7: // Boutons Annuler/Créer
    {
        if (IsKeyPressed(KEY_LEFT))
        {
            // Sélectionne "Annuler"
            selectedOption = 7;
        }
        if (IsKeyPressed(KEY_RIGHT))
        {
            // Sélectionne "Créer"
            selectedOption = 8;
        }
        break;
    }
    }

    // Validation finale
    if (selectedOption == 7 && IsKeyPressed(KEY_ENTER))
    {
        // Annuler : retour au lobby
        std::cout << "[Game] Création annulée, retour au lobby." << std::endl;
        gameClient.setState(GameClient::State::LOBBY);
    }

    if (selectedOption == 8 && IsKeyPressed(KEY_ENTER))
    {
        // ✅ DEBUG : Afficher la config AVANT envoi
        std::cout << "[Game DEBUG] Config à envoyer:" << std::endl;
        std::cout << "  - Nom: '" << config.roomName << "'" << std::endl;
        std::cout << "  - Difficulté: " << (int)config.difficulty << std::endl;
        std::cout << "  - Max joueurs: " << (int)config.maxPlayers << std::endl;
        std::cout << "  - Vitesse: " << (int)config.enemySpeedMultiplier << std::endl;
        std::cout << "  - Spawn: " << (int)config.spawnRateMultiplier << std::endl;

        Protocol::CreateRoomRequestMessage msg(config);
        auto data = msg.serialize();

        std::cout << "[Game DEBUG] Buffer size: " << data.size() << " octets" << std::endl;
        std::cout << "[Game DEBUG] Attendu: " << (sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::CreateRoomRequest)) << " octets" << std::endl;

        networkClient.sendRaw(data);
        gameClient.setState(GameClient::State::CONNECTING);
    }

    // Raccourci : Échap pour annuler
    if (IsKeyPressed(KEY_ESCAPE))
    {
        gameClient.setState(GameClient::State::LOBBY);
    }
}
// --- Rendu (Modifié) ---
// La logique est maintenant dans la boucle run() pour gérer les états
void Game::render()
{
    // Laissé vide car la logique est dans run()
    // Ou mieux : déplacer toute la logique de BeginDrawing/EndDrawing de run() ici
    // pour garder run() plus propre.
}

void Game::initializeChat(const std::string &nickname)
{
    if (m_chat)
    {
        std::cout << "[Game] Chat déjà initialisé" << std::endl;
        return;
    }

    try
    {
        //  Utilise m_serverIp et m_tcpPort (pas hardcodés)
        m_chat = std::make_unique<Chat>(m_serverIp, m_tcpPort, nickname);

        std::cout << "[Game]  Chat initialisé:" << std::endl;
        std::cout << "  - Serveur: " << m_serverIp << ":" << m_tcpPort << std::endl;
        std::cout << "  - Pseudo: '" << nickname << "'" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[Game]  Erreur initialisation chat: " << e.what() << std::endl;
    }
}