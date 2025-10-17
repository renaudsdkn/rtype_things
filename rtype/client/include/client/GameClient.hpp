#pragma once
#include "GraphicsEngine.hpp" // Doit exister et utiliser Raylib
#include "InputManager.hpp"   // Doit exister pour gérer les entrées
#include "Commands.hpp"       // Doit exister pour les actions du joueur
#include "ecs.hpp"            // Le cœur de l'ECS

/**
 * @brief Le cœur de l'application client.
 * Gère la boucle de jeu principale, l'I/O, et l'intégration de l'ECS.
 */
class GameClient {
public:
    /**
     * @brief Constructeur du client.
     * @param width Largeur de la fenêtre.
     * @param height Hauteur de la fenêtre.
     * @param title Titre de la fenêtre.
     */
    GameClient(int width, int height, const std::string& title);
    
    /**
     * @brief Lance la boucle de jeu principale.
     */
    void run();

private:
    GraphicsEngine m_graphics;
    InputManager m_input;
    registry m_registry;
    entity_t m_player_entity;

    /**
     * @brief Initialise les composants et systèmes de l'ECS.
     */
    void initECS();
    
    /**
     * @brief Traite la liste des commandes générées par l'InputManager.
     * @param commands La liste des commandes de jeu à traiter.
     */
    void processInput(const CommandList& commands);
    
    /**
     * @brief Exécute la logique de jeu (systèmes ECS).
     */
    void updateLogic();
    
    /**
     * @brief Effectue le rendu de la scène.
     */
    void renderScene();
};
