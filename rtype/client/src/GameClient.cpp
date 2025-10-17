#include "../include/client/GameClient.hpp"
#include <iostream>
#include <algorithm>
#include "../include/client/ecs.hpp"

void control_system(registry& r) {
    auto& velocities = r.get_components<Velocity>();
    auto& controls = r.get_components<controllable>();

    const float player_speed = 5.0f;

    for (size_t i = 0; i < velocities.size() && i < controls.size(); ++i) {
        if (velocities[i].has_value() && controls[i].has_value()) {
            auto& vel = velocities[i].value();
            auto& ctrl = controls[i].value();

            vel.x = 0;
            vel.y = 0;

            if (ctrl.Up)    vel.y -= player_speed;
            if (ctrl.Down)  vel.y += player_speed;
            if (ctrl.Left)  vel.x -= player_speed;
            if (ctrl.Right) vel.x += player_speed;
        }
    }
}

void movement_system(registry& r) {
    auto& positions = r.get_components<Position>();
    auto& velocities = r.get_components<Velocity>();

    for (size_t i = 0; i < positions.size() && i < velocities.size(); ++i) {
        if (positions[i].has_value() && velocities[i].has_value()) {
            positions[i].value().x += velocities[i].value().x;
            positions[i].value().y += velocities[i].value().y;
        }
    }
}


// Constructeur
GameClient::GameClient(int width, int height, const std::string& title)
    : m_graphics(width, height, title) 
{
    initECS();
}


// --- 1. Initialisation : Création du Monde et du Joueur ---
void GameClient::initECS() {
    m_registry.register_component<Position>();
    m_registry.register_component<Velocity>();
    m_registry.register_component<controllable>();
    m_registry.register_component<Displayable>(); 

    m_player_entity = m_registry.spawn_entity();
    

    
    m_registry.get_components<Position>().insert_at(m_player_entity, Position{100.f, 200.f});
    m_registry.get_components<Velocity>().insert_at(m_player_entity, Velocity{0.f, 0.f});
    m_registry.get_components<controllable>().insert_at(m_player_entity, controllable{});
    m_registry.get_components<Displayable>().insert_at(m_player_entity, Displayable{1, 50.0f, 30.0f}); 

    std::cout << "ECS initialised. Player entity ID: " << (size_t)m_player_entity << std::endl;
}


// --- 2. Entrées : Écrire les commandes dans le composant controllable ---
void GameClient::processInput(const CommandList& commands) {
    auto& controls_array = m_registry.get_components<controllable>();
    

    if (m_player_entity < controls_array.size() && controls_array[m_player_entity].has_value()) {
        auto& ctrl = controls_array[m_player_entity].value();
        
        
        ctrl.Up = ctrl.Down = ctrl.Left = ctrl.Right = false;

        for (PlayerAction cmd : commands) {
            switch (cmd) {
                case PlayerAction::MOVE_UP:    ctrl.Up = true; break;
                case PlayerAction::MOVE_DOWN:  ctrl.Down = true; break;
                case PlayerAction::MOVE_LEFT:  ctrl.Left = true; break;
                case PlayerAction::MOVE_RIGHT: ctrl.Right = true; break;
                case PlayerAction::SHOOT:      std::cout << "ECS: Shoot command received!" << std::endl; break;
                default: break;
            }
        }
    }
}

// --- 3. Logique : Exécuter les systèmes ECS ---
void GameClient::updateLogic() {
    control_system(m_registry); 

    movement_system(m_registry);
}

void GameClient::renderScene() {
    m_graphics.beginDrawing();
    m_graphics.clearScreen(20, 20, 20);
    
    auto& positions = m_registry.get_components<Position>();
    auto& displayables = m_registry.get_components<Displayable>();
    
    size_t max_entities = std::min(positions.size(), displayables.size());


    for (size_t entity_id = 0; entity_id < max_entities; ++entity_id) {
        

        if (positions[entity_id].has_value() && displayables[entity_id].has_value()) {
            
            const auto& pos = positions[entity_id].value();
            const auto& disp = displayables[entity_id].value();


            Color color = (disp.textureId == 1) ? GREEN : RED;
        

            DrawRectangle((int)pos.x, (int)pos.y, (int)disp.width, (int)disp.height, color);
            
            
        }
    }
    
    m_graphics.endDrawing();
}

// --- 5. La Boucle Principale ---
void GameClient::run() {
    while (!m_graphics.shouldClose()) {

        CommandList commands = m_input.getCommands();
        
        processInput(commands);

        updateLogic();

        renderScene();
    }
}
