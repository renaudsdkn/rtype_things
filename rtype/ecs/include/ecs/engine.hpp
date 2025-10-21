#pragma once // Bonne pratique

// Includes nécessaires
#include "bootstrap.hpp" // ✅ Inclure notre ECS nettoyé
#include <set>
#include <functional>
#include <iostream>
#include <map>
#include <cstdint> // Pour uint32_t
#include <vector> // Pour std::vector utilisé dans les systèmes

// Namespace global pour ECS et Components (comme dans ton code original)
using namespace ECS;
using namespace Components;

// NOUVEAU : Structure pour passer les inputs de manière agnostique (ni SFML, ni réseau)
struct InputData {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool shoot = false;
};

// --- Classe Engine ---
// Contient la logique principale du jeu R-Type
class Engine {
private:
    registry _reg; // ✅ L'instance de l'ECS pour ce moteur
    uint32_t m_next_network_id = 1000;
    // --- Stockage interne des joueurs ---
    std::set<entity> player_entity_set; // Ensemble des entités 'joueur' actives
    // MODIFIÉ : Map pour lier l'ID réseau (uint32_t) à l'entité ECS (entity)
    std::map<uint32_t, entity> player_id_to_entity_map;

    // --- Systèmes de jeu ---
    // Fonctions lambda stockées pour la logique de collision et de niveau
    std::function<void(entity, entity, float)> collision_system_logic; // Renommé pour clarté
    std::function<void(entity, entity, float)> levelUp_system_logic; // Renommé pour clarté
    // Vecteur contenant les fonctions des systèmes à exécuter à chaque update
    std::vector<std::function<void(float)>> _systems; // MODIFIÉ : Les systèmes prennent deltaTime

    // Map pour gérer le cooldown de tir pour chaque entité (joueur)
    std::map<entity, float> shoot_cooldowns;

public:
    Engine(); // Constructeur
    ~Engine(){}; // Destructeur (par défaut ici)

    // --- Fonctions principales ---

    // Met à jour l'état du jeu (exécute les systèmes, collisions, etc.)
    // MODIFIÉ : Prend deltaTime ET collisionBound
    void update(float deltaTime, float collisionBound);

    // Gère les inputs reçus pour un joueur spécifique
    // MODIFIÉ : Prend l'ID réseau et la structure InputData
    void handle_input(uint32_t playerId, const InputData& inputs);

    // --- Fonctions de gestion des entités ---

    // Crée une entité joueur liée à un ID réseau
    // MODIFIÉ : Prend l'ID réseau
    void spawn_player(uint32_t playerId, float x = 100.f, float y = 400.f);

    // Supprime une entité joueur (et ses composants) via son ID réseau
    // NOUVEAU : Fonction essentielle ajoutée
    void remove_player(uint32_t playerId);

    // Crée une entité ennemie d'un certain type à une position donnée
    void spawn_enemy(Components::Enemystype type, float x, float y);

    // Crée une entité balle tirée par une entité (généralement un joueur)
    // MODIFIÉ : Prend l'entité qui tire en paramètre
    void shoot_bullet(entity shooter_entity, Components::AttackType type, float x, float y);

    // Crée une entité power-up (orbe d'XP)
    void spawnLevelUporbs(float x, float y);

    // --- Fonctions d'accès ---

    // Retourne une référence à la registry (pour le snapshot par exemple)
    registry& get_registry() { return _reg; }
    const registry& get_registry() const { return _reg; } // Version const

    // Indique si tous les joueurs ont été tués
    bool hasLost() const { return player_entity_set.empty(); } // MODIFIÉ : Utilise la variable renommée

    // Retourne l'ensemble des entités joueurs (peut être utile pour des logiques spécifiques)
    std::set<entity> get_player_entities() const { return player_entity_set; } // MODIFIÉ : Utilise la variable renommée
};

// SUPPRIMÉ : Les includes SFML et les définitions de composants (déplacés dans ecs.hpp)