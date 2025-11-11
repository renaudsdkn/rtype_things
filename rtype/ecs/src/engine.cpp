#include "../include/ecs/bootstrap.hpp"
/*#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>*/
#include "../include/ecs/engine.hpp"
#include <cstddef>
#include <set>
#include <cmath>   // Pour std::sin, std::cos, std::abs, std::sqrt
#include <vector>  // Pour std::vector
#include <map>     // Pour std::map
#include <iostream> // Pour les logs (optionnel mais utile)

// SUPPRIMÉ : Les includes SFML

using namespace ECS; // Utilisation de l'alias pour le namespace ECS

Engine::Engine() {
    // --- Enregistrement des composants ---
    // Enregistre tous les types de composants utilisés par le jeu auprès de la registry.
    // C'est nécessaire pour que l'ECS sache comment stocker et gérer ces composants.
    _reg.register_component<Components::Position>();
    _reg.register_component<Components::Velocity>();
    _reg.register_component<Components::Controllable>();
    _reg.register_component<Components::Enemystype>();
    _reg.register_component<Components::AttackType>();
    _reg.register_component<Components::Healthpoints_t>();
    _reg.register_component<Components::Damage_t>();
    _reg.register_component<Components::PlayerStats>();
    _reg.register_component<Components::Bullet>();
    _reg.register_component<Components::PowerUp>();
    _reg.register_component<Components::NetworkId>(); // ✅ NOUVEAU : Enregistrement du NetworkId
    _reg.register_component<Components::BulletOwner>();
    // --- Définition et enregistrement des Systèmes ---
    // Les systèmes contiennent la logique de jeu qui opère sur les composants.
    // Ils sont stockés dans le vecteur _systems pour être exécutés à chaque update.

    // Système de Mouvement : Met à jour la position en fonction de la vélocité et du deltaTime
    auto movement_system = [&](float dt) { // MODIFIÉ : Prend deltaTime
        auto& positions = _reg.get_components<Components::Position>();
        auto& velocities = _reg.get_components<Components::Velocity>();
        // Parcours toutes les entités potentielles
        for (std::size_t i = 0; i < positions.size() && i < velocities.size(); ++i) {
            // Si l'entité a les deux composants
            if (positions[i].has_value() && velocities[i].has_value()) {
                 // Met à jour la position : position += velocité * deltaTime
                positions[i]->x += velocities[i]->x * dt; // ✅ Utilisation de deltaTime
                positions[i]->y += velocities[i]->y * dt; // ✅ Utilisation de deltaTime
            }
        }
    };
    _systems.push_back(movement_system); // Ajoute le système à la liste

    // Système de Contrôle : Met à jour la vélocité du joueur en fonction des inputs (Controllable)
    auto control_system = [&](float dt) { // MODIFIÉ : Prend deltaTime (même s'il n'est pas utilisé ici, pour la cohérence)
        auto& controls = _reg.get_components<Components::Controllable>();
        auto& velocities = _reg.get_components<Components::Velocity>();
        auto& positions = _reg.get_components<Components::Position>(); // Pour vérifier les limites de l'écran

        for (std::size_t i = 0; i < controls.size(); ++i) {
            // Si l'entité est contrôlable et a une vélocité/position
            if (controls[i].has_value() && i < velocities.size() && velocities[i].has_value() && i < positions.size() && positions[i].has_value()) {
                auto& control = controls[i].value();
                auto& velocity = velocities[i].value();
                auto& pos = positions[i].value();

                // Réinitialise la vélocité
                velocity.x = velocity.y = 0.f;
                // Applique la vitesse en fonction des booléens dans 'control' et des limites écran
                // NOTE : Les limites écran (20, 780, 1180) sont codées en dur, à rendre configurables idéalement.
                const float player_speed = 300.0f; // Vitesse en unités par seconde
                if (control.Up && pos.y > 20.f) velocity.y = -player_speed;
                if (control.Down && pos.y < 780.f) velocity.y = player_speed;
                if (control.Left && pos.x > 20.f) velocity.x = -player_speed;
                if (control.Right && pos.x < 1180.f) velocity.x = player_speed;
            }
        }
    };
    _systems.push_back(control_system);

    // Système des Balles : Gère la trajectoire et la désactivation des balles hors écran
    auto bullet_system = [&](float dt) { // MODIFIÉ : Prend deltaTime
        auto &positions = _reg.get_components<Components::Position>();
        auto &velocities = _reg.get_components<Components::Velocity>();
        auto &bullets = _reg.get_components<Components::Bullet>();
        auto &attacks = _reg.get_components<Components::AttackType>();

        for (std::size_t i = 0; i < bullets.size(); ++i) {
             // Si l'entité est une balle active avec position, vélocité et type d'attaque
            if (bullets[i].has_value() && bullets[i]->active && i < positions.size() && positions[i].has_value() &&
                i < velocities.size() && velocities[i].has_value() && i < attacks.size() && attacks[i].has_value())
            {
                auto &pos = positions[i].value();
                auto &vel = velocities[i].value();
                auto attack_type = attacks[i].value();

                // Définit la vélocité en fonction du type d'attaque (trajectoire)
                // Ces vitesses sont maintenant en unités par seconde
                switch (attack_type) {
                    case Components::AttackType::StdShot: vel.x = 600.f; vel.y = 0.f; break;
                    case Components::AttackType::RoundShot: vel.x = 600.f; vel.y = std::sin(pos.x * 0.01f) * 30.f; break; // Ajusté les multiplicateurs
                    case Components::AttackType::BounceShot: vel.x = 600.f; vel.y = std::sin(pos.x * 0.02f) * 120.f; break;
                    case Components::AttackType::StraightShot: vel.x = 900.f; vel.y = 0.f; break;
                    case Components::AttackType::RippleShot: vel.x = 720.f; vel.y = std::sin(pos.x * 0.03f) * 180.f; break;
                    case Components::AttackType::WaveCanon: vel.x = 480.f; vel.y = std::sin(pos.x * 0.015f) * 90.f; break;
                }

                // Désactive la balle si elle sort de l'écran (limites codées en dur)
                if (pos.x > 1300.f || pos.x < -100.f || pos.y > 900.f || pos.y < -100.f) {
                    bullets[i]->active = false;
                    _reg.kill_entity(entity(i)); 
                     std::cout << "[ENGINE] Balle " << i << " détruite (hors écran)" << std::endl;
                }
            }
        }
    };
    _systems.push_back(bullet_system);

    // Système d'IA Ennemie : Gère les déplacements basiques des ennemis
    auto enemy_ai_system = [&](float dt) { // MODIFIÉ : Prend deltaTime
        auto& positions = _reg.get_components<Components::Position>();
        auto& velocities = _reg.get_components<Components::Velocity>();
        auto& enemies = _reg.get_components<Components::Enemystype>();

        for (std::size_t i = 0; i < enemies.size(); ++i) {
            // Si l'entité est un ennemi avec position et vélocité
            if (enemies[i].has_value() && i < positions.size() && positions[i].has_value() && i < velocities.size() && velocities[i].has_value()) {
                auto& pos = positions[i].value();
                auto& vel = velocities[i].value();
                auto enemy_type = enemies[i].value();

                // Définit la vélocité en fonction du type d'ennemi (pattern de mouvement)
                // Ces vitesses sont maintenant en unités par seconde
                switch (enemy_type) {
                    case Components::Enemystype::Grubs: vel.x = -30.f; vel.y = 0.0f; break;
                    case Components::Enemystype::Flyers: vel.x = -60.f; vel.y = std::sin(pos.x * 0.01f) * 120.f; break;
                    case Components::Enemystype::Turrets: vel.x = 0.0f; vel.y = 0.0f; break; // Immobile
                    case Components::Enemystype::Eyes: vel.x = -18.f; vel.y = std::cos(pos.x * 0.015f) * 90.f; break;
                    case Components::Enemystype::Squids: vel.x = -48.f; vel.y = std::sin(pos.x * 0.02f) * 180.f; break;
                    case Components::Enemystype::Moths: vel.x = -72.f + std::sin(pos.y * 0.03f) * 30.f; vel.y = std::cos(pos.x * 0.025f) * 150.f; break;
                    case Components::Enemystype::Crabs: vel.x = -42.f; vel.y = std::sin(pos.x * 0.05f) * 60.f; break;
                    case Components::Enemystype::Gargoyle: vel.x = -90.f; vel.y = std::sin(pos.x * 0.008f) * 240.f; break;
                    case Components::Enemystype::Dobkeratops: vel.x = -12.f; vel.y = std::sin(pos.x * 0.005f) * 60.f; break; // Boss lent
                    case Components::Enemystype::Gel: vel.x = -36.f; vel.y = std::abs(std::sin(pos.x * 0.04f)) * 120.f - 60.f; break;
                    case Components::Enemystype::Hades: vel.x = -18.f + std::cos(pos.y * 0.01f) * 30.f; vel.y = std::sin(pos.x * 0.01f) * 120.f; break;
                    case Components::Enemystype::Gomorrah: vel.x = -108.f; vel.y = std::sin(pos.x * 0.012f) * 210.f; break;
                    case Components::Enemystype::TheCore: vel.x = 0.0f; vel.y = std::sin(pos.x * 0.003f) * 30.f; break; // Boss quasi immobile
                    case Components::Enemystype::None: default: break;
                }
            }
        }
    };
    _systems.push_back(enemy_ai_system);

    // Système de "Mort" : Tue les entités dont la vie est <= 0
    auto killEntities_system = [&](float dt) { // MODIFIÉ : Prend deltaTime
        auto& healths = _reg.get_components<Components::Healthpoints_t>();
        auto& enemies = _reg.get_components<Components::Enemystype>();
        auto& positions = _reg.get_components<Components::Position>();
        auto& playerSts = _reg.get_components<Components::PlayerStats>();

        for (std::size_t i = 0; i < healths.size(); i++) {
            // Si l'entité a de la vie et que celle-ci est <= 0
            if (healths[i].has_value() && healths[i].value() <= 0) {
                 // Si c'était un ennemi avec une position, fait spawner un power-up
                if (i < enemies.size() && enemies[i].has_value() &&
                    i < positions.size() && positions[i].has_value()) {
                    spawnLevelUporbs(positions[i].value().x, positions[i].value().y);
                }
                 // Si c'était un joueur, le retire des listes internes de l'Engine
                if (i < playerSts.size() && playerSts[i].has_value()) {
                    entity ent_to_remove = entity(i);
                    // Trouve l'ID réseau correspondant à cette entité et le supprime de la map
                    for (auto it = player_id_to_entity_map.begin(); it != player_id_to_entity_map.end(); ++it) {
                        if (it->second == ent_to_remove) {
                            player_id_to_entity_map.erase(it);
                            break;
                        }
                    }
                    player_entity_set.erase(ent_to_remove); // Supprime de l'ensemble des joueurs actifs
                }
                _reg.kill_entity(entity(i)); // Tue l'entité dans l'ECS
            }
        }
    };
    _systems.push_back(killEntities_system);

    // --- Logique de Collision et LevelUp (stockées comme fonctions membres) ---
    // Ces logiques sont appelées séparément dans la fonction update car elles nécessitent
    // de comparer toutes les paires d'entités.

    // Logique pour le Level Up : quand un joueur touche un PowerUp
    levelUp_system_logic = [&](entity first, entity second, float collisionbounds) {
        // ... (Ton ancienne logique de levelUp_system, semble correcte) ...
        // Récupération des composants nécessaires
        auto &positions = _reg.get_components<Components::Position>();
        auto &powerups = _reg.get_components<Components::PowerUp>();
        auto &status = _reg.get_components<Components::PlayerStats>();
        auto &attacks = _reg.get_components<Components::AttackType>();

        // Vérifications de base (si les entités existent et ont une position)
        if (first >= positions.size() || second >= positions.size() || !positions[first].has_value() || !positions[second].has_value())
            return;

        // Calcul de distance
        float distance = positions[first].value() - positions[second].value();
        if (distance > collisionbounds) // Pas de collision si trop loin
            return;

        // Détermine si on a une collision Joueur <-> PowerUp
        bool first_is_powerup = (first < powerups.size() && powerups[first].has_value());
        bool second_is_powerup = (second < powerups.size() && powerups[second].has_value());
        // On utilise player_entity_set pour identifier un joueur
        bool first_is_player = player_entity_set.count(first);
        bool second_is_player = player_entity_set.count(second);

        entity powerup_entity;
        entity player_entity_id;
        bool collision_found = false;

        if (first_is_player && second_is_powerup) {
            player_entity_id = first; powerup_entity = second; collision_found = true;
        } else if (first_is_powerup && second_is_player) {
            player_entity_id = second; powerup_entity = first; collision_found = true;
        }

        // Si collision Joueur <-> PowerUp trouvée
        if (collision_found && player_entity_id < status.size() && status[player_entity_id].has_value()) {
            status[player_entity_id]->xp += 10; // Donne de l'XP

            // Logique de passage de niveau (semble correcte)
            bool leveled_up = false;
            int current_level = status[player_entity_id]->level;
            int current_xp = status[player_entity_id]->xp;
            int xp_needed = 0;
            switch(current_level) { // Seuils d'XP pour chaque niveau
                case 1: xp_needed = 100; break;
                case 2: xp_needed = 150; break;
                case 3: xp_needed = 200; break;
                case 4: xp_needed = 300; break;
                case 5: xp_needed = 500; break;
                default: xp_needed = -1; // Niveau max atteint
            }

            if (xp_needed > 0 && current_xp >= xp_needed) {
                status[player_entity_id]->level++;
                status[player_entity_id]->xp -= xp_needed; // Garde l'XP excédentaire ? Ou reset à 0 ? (reset ici)
                status[player_entity_id]->xp = 0;
                leveled_up = true;
            }

            // Si le joueur a level up, améliore son arme (AttackType)
            if (leveled_up && player_entity_id < attacks.size() && attacks[player_entity_id].has_value()) {
                // Le niveau correspond directement à l'enum AttackType (sauf WaveCanon)
                if(status[player_entity_id]->level <= 6) { // Assure de ne pas dépasser
                   attacks[player_entity_id].value() = static_cast<Components::AttackType>(status[player_entity_id]->level);
                    // Pour le niveau 6, on donne WaveCanon
                    if (status[player_entity_id]->level == 6) {
                        attacks[player_entity_id].value() = Components::AttackType::WaveCanon;
                    }
                    std::cout << "[ENGINE] Level Up! Joueur (entité " << (size_t)player_entity_id << ") niveau " << status[player_entity_id]->level << std::endl;
                }
            }
            _reg.kill_entity(powerup_entity); // Détruit le power-up collecté
        }
    };

    // Logique pour les Collisions : Balle<->Ennemi, Joueur<->Ennemi
    collision_system_logic = [&](entity first, entity second, float collisionbounds) {
        // ... (Ton ancienne logique de collision_system, semble correcte) ...
        // Récupération des composants
        auto &positions = _reg.get_components<Components::Position>();
        auto &enemies = _reg.get_components<Components::Enemystype>();
        auto &bullets = _reg.get_components<Components::Bullet>();
        auto &health_ps = _reg.get_components<Components::Healthpoints_t>();
        auto &damages = _reg.get_components<Components::Damage_t>();

        // Vérifications de base
        if (first >= positions.size() || second >= positions.size() || !positions[first].has_value() || !positions[second].has_value())
            return;

        // Calcul distance
        float distance = positions[first].value() - positions[second].value();
        if (distance > collisionbounds)
            return;

        // Détermine les types des deux entités
        bool first_is_bullet = (first < bullets.size() && bullets[first].has_value() && bullets[first]->active);
        bool second_is_bullet = (second < bullets.size() && bullets[second].has_value() && bullets[second]->active);
        bool first_is_enemy = (first < enemies.size() && enemies[first].has_value());
        bool second_is_enemy = (second < enemies.size() && enemies[second].has_value());
        bool first_is_player = player_entity_set.count(first);
        bool second_is_player = player_entity_set.count(second);

        // --- Logique des collisions ---

        // Cas 1: Balle touche Ennemi
        // Exemple modifié du cas "Balle touche Ennemi"
        if (first_is_bullet && second_is_enemy) {
            // Vérifie si les deux ont les composants Damage et Healthpoints
            if (first < damages.size() && damages[first].has_value() &&
                second < health_ps.size() && health_ps[second].has_value()) {
                health_ps[second].value() -= damages[first].value(); // Ennemi perd vie

                // Récupère l'owner de la bullet (si présent) et attribue points si l'ennemi meurt
                uint32_t killerNetId = 0;
                auto& owners = _reg.get_components<Components::BulletOwner>();
                if (first < owners.size() && owners[first].has_value()) {
                    killerNetId = owners[first]->ownerNetworkId;
                }

                // Désactive / tue la balle
                bullets[first]->active = false;
                _reg.kill_entity(first);

                // Si l'ennemi est mort (hp <= 0) : award points to killer
            if (second < health_ps.size() && health_ps[second].has_value() && health_ps[second]->_hp <= 0) {
                    // Définit la quantité de points pour ce type d'ennemi (ex: 100)
                    int points = 100;
                    // Optionnel: adapter selon enemy type
                    auto& enemyTypes = _reg.get_components<Components::Enemystype>();
                    if (second < enemyTypes.size() && enemyTypes[second].has_value()) {
                        switch (enemyTypes[second].value()) {
                            case Components::Enemystype::Grubs: points = 50; break;
                            case Components::Enemystype::Flyers: points = 75; break;
                            case Components::Enemystype::Dobkeratops: points = 500; break;
                            case Components::Enemystype::TheCore: points = 1000; break;
                            case Components::Enemystype::Turrets: points = 30; break;
                            case Components::Enemystype::Eyes: points = 80; break;
                            case Components::Enemystype::Squids: points = 90; break;
                            case Components::Enemystype::Moths: points = 120; break;
                            case Components::Enemystype::Crabs: points = 110; break;
                            case Components::Enemystype::Gargoyle: points = 150; break;
                            case Components::Enemystype::Gel: points = 70; break;
                            case Components::Enemystype::Hades: points = 400; break;
                            case Components::Enemystype::Gomorrah: points = 600; break;
                            case Components::Enemystype::None:
                            default: points = 100; break;
                        }
                    }
                    if (killerNetId != 0) {
                        add_score(killerNetId, points);
                        std::cout << "[ENGINE] Awarded " << points << " points to player " << killerNetId << std::endl;
                    }
                }
            }
        }
        // Cas 2: Ennemi touche Balle (symétrique)
        else if (first_is_enemy && second_is_bullet) {
            if (second < damages.size() && damages[second].has_value() &&
                first < health_ps.size() && health_ps[first].has_value()) {
                health_ps[first].value() -= damages[second].value(); // Ennemi perd vie (si PV)
                bullets[second]->active = false; // Balle désactivée
                 _reg.kill_entity(second);
            }
        }
        // Cas 3: Joueur touche Ennemi (collision directe)
        else if (first_is_player && second_is_enemy) {
            // Suppose que les deux ont Healthpoints et Damage
             if (first < health_ps.size() && health_ps[first].has_value() &&
                 second < damages.size() && damages[second].has_value() &&
                 first < damages.size() && damages[first].has_value() && // Suppose que le joueur a aussi un Damage_t
                 second < health_ps.size() && health_ps[second].has_value())
             {
                 health_ps[first].value() -= damages[second].value(); // Joueur perd vie
                 health_ps[second].value() -= damages[first].value(); // Ennemi perd vie
             }
        }
        // Cas 4: Ennemi touche Joueur (symétrique)
        else if (first_is_enemy && second_is_player) {
             if (second < health_ps.size() && health_ps[second].has_value() &&
                 first < damages.size() && damages[first].has_value() &&
                 second < damages.size() && damages[second].has_value() && // Suppose que le joueur a aussi un Damage_t
                 first < health_ps.size() && health_ps[first].has_value())
             {
                 health_ps[second].value() -= damages[first].value(); // Joueur perd vie
                 health_ps[first].value() -= damages[second].value(); // Ennemi perd vie
            }
        }
    };
}; // Fin du constructeur Engine::Engine()

// --- Implémentation des fonctions membres ---

// Crée une entité joueur dans l'ECS et l'associe à l'ID réseau
void Engine::spawn_player(uint32_t playerId, float x, float y) {
    std::cout << "[ENGINE] spawn_player() appelé pour ID " << playerId 
              << " à position (" << x << ", " << y << ")" << std::endl;
    
    // ✅ Vérifie si déjà existant
    if (player_id_to_entity_map.count(playerId)) {
        std::cerr << "[ENGINE] ⚠️ Joueur " << playerId << " DÉJÀ SPAWNÉ !" << std::endl;
        std::cerr << "[ENGINE] Entité existante: " << (size_t)player_id_to_entity_map[playerId] << std::endl;
        
        // ✅ DEBUG : Affiche TOUTE la map
        std::cout << "[ENGINE] player_id_to_entity_map actuelle:" << std::endl;
        for (const auto& [id, ent] : player_id_to_entity_map) {
            std::cout << "  - ID " << id << " → Entité " << (size_t)ent << std::endl;
        }
        
        return;
    }
    
    auto ent = _reg.spawn_entity();
    std::cout << "[ENGINE] Entité ECS créée: " << (size_t)ent << std::endl;

    // Ajoute composants
    _reg.emplace_component<Components::Position>(ent, x, y);
    _reg.emplace_component<Components::Velocity>(ent, 0.f, 0.f);
    _reg.emplace_component<Components::Controllable>(ent);
    _reg.emplace_component<Components::AttackType>(ent, Components::AttackType::StdShot);
    _reg.emplace_component<Components::Healthpoints_t>(ent, 100);
    _reg.emplace_component<Components::Damage_t>(ent, 20);
    _reg.emplace_component<Components::PlayerStats>(ent);
    _reg.emplace_component<Components::NetworkId>(ent, playerId);

    // ✅ Stocke dans les maps
    player_entity_set.insert(ent);
    player_id_to_entity_map[playerId] = ent;
    
    std::cout << "[ENGINE] ✅ Joueur spawné avec succès !" << std::endl;
    std::cout << "[ENGINE]   - ID réseau: " << playerId << std::endl;
    std::cout << "[ENGINE]   - Entité ECS: " << (size_t)ent << std::endl;
    std::cout << "[ENGINE]   - Total joueurs actifs: " << player_entity_set.size() << std::endl;
}

// Supprime l'entité joueur associée à l'ID réseau
// NOUVEAU : Implémentation ajoutée
void Engine::remove_player(uint32_t playerId) {
    if (player_id_to_entity_map.count(playerId)) { // Vérifie si ce joueur existe
        entity ent = player_id_to_entity_map[playerId]; // Trouve l'entité ECS
        _reg.kill_entity(ent); // Demande à l'ECS de supprimer l'entité et ses composants
        player_entity_set.erase(ent); // Retire de l'ensemble des joueurs actifs
        player_id_to_entity_map.erase(playerId); // Retire de la map de correspondance
        std::cout << "[ENGINE] Joueur remove (ID réseau: " << playerId << ", Entité: " << (size_t)ent << ")" << std::endl;
    } else {
         std::cerr << "[ENGINE] Erreur : Tentative de remove un joueur inconnu (ID réseau: " << playerId << ")" << std::endl;
    }
}

// Crée une entité power-up
void Engine::spawnLevelUporbs(float x, float y) {
    auto ent = _reg.spawn_entity();
    uint32_t newNetworkId =  powers_ups++;
    _reg.emplace_component<Components::NetworkId>(ent, newNetworkId);  
    _reg.emplace_component<Components::Position>(ent, x, y);
    _reg.emplace_component<Components::Velocity>(ent, -120.f, 0.f); // Vitesse vers la gauche (unités/sec)
    _reg.emplace_component<Components::PowerUp>(ent);
};

// Crée une entité balle
// MODIFIÉ : Prend l'entité tireur en paramètre
void Engine::shoot_bullet(entity shooter_entity, Components::AttackType type, float x, float y) {
    auto ent = _reg.spawn_entity();
    uint32_t newNetworkId = ball_network++; // Utilise ta fonction helper
    _reg.emplace_component<Components::NetworkId>(ent, newNetworkId); // ✅ AJOUTER CETTE LIGNE
    _reg.emplace_component<Components::Position>(ent, x, y);
    // La vélocité sera définie par le bullet_system en fonction du type
    _reg.emplace_component<Components::Velocity>(ent, 0.f, 0.f);
    _reg.emplace_component<Components::Bullet>(ent); // Marque comme balle
    _reg.emplace_component<Components::AttackType>(ent, type); // Type d'arme
    






    uint32_t ownerNetId = 0;
    auto& networkIds = _reg.get_components<Components::NetworkId>();
    if (shooter_entity < networkIds.size() && networkIds[shooter_entity].has_value()) {
        ownerNetId = networkIds[shooter_entity]->id;
    }
    // Attache le composant BulletOwner à la bullet
    _reg.emplace_component<Components::BulletOwner>(ent, Components::BulletOwner(ownerNetId));




    // Définit les dégâts de la balle en fonction du type
    Components::Damage_t dmg;
    switch(type) {
        case Components::AttackType::StdShot: dmg = 10; break;
        case Components::AttackType::RoundShot: dmg = 15; break;
        case Components::AttackType::BounceShot: dmg = 12; break;
        case Components::AttackType::StraightShot: dmg = 25; break;
        case Components::AttackType::RippleShot: dmg = 30; break;
        case Components::AttackType::WaveCanon: dmg = 50; break;
        default: dmg = 10; break;
    }
    _reg.emplace_component<Components::Damage_t>(ent, dmg);
};

// Met à jour le composant Controllable du joueur correspondant à l'ID réseau
// MODIFIÉ : Prend l'ID réseau et la structure InputData
void Engine::handle_input(uint32_t playerId, const InputData& inputs) {
    // ✅ DEBUG : Log AVANT vérification
    std::cout << "[ENGINE] handle_input() appelé pour joueur " << playerId << std::endl;
    std::cout << "[ENGINE] player_id_to_entity_map contient " << player_id_to_entity_map.size() << " joueurs" << std::endl;
    
    if (!player_id_to_entity_map.count(playerId)) {
        std::cerr << "[ENGINE] Erreur: Input reçu pour joueur INCONNU (ID réseau: " << playerId << ")" << std::endl;
        
        // ✅ DEBUG : Affiche les IDs connus
        std::cerr << "[ENGINE] IDs connus: ";
        for (const auto& [id, ent] : player_id_to_entity_map) {
            std::cerr << id << " ";
        }
        std::cerr << std::endl;
        
        return;
    }

    entity ent = player_id_to_entity_map[playerId];
    auto& controls = _reg.get_components<Components::Controllable>();

    if (ent < controls.size() && controls[ent].has_value()) {
        auto& ctrl = controls[ent].value();
        ctrl.Up = inputs.up;
        ctrl.Down = inputs.down;
        ctrl.Left = inputs.left;
        ctrl.Right = inputs.right;
        ctrl.Shoot = inputs.shoot;
        
        // ✅ DEBUG : Confirme mise à jour
        // std::cout << "[ENGINE] ✅ Input appliqué au joueur " << playerId << " (entité " << (size_t)ent << ")" << std::endl;
    } else {
        std::cerr << "[ENGINE]  Joueur " << playerId << " (entité " << (size_t)ent 
                  << ") n'a pas de composant Controllable !" << std::endl;
    }
}

// Fonction principale de mise à jour du jeu
// MODIFIÉ : Prend deltaTime ET collisionBound
void Engine::update(float deltaTime, float collisionBound) {
    // --- Gestion des Tirs ---
    // (Déplacé depuis l'ancien main SFML)
    auto shoot_system = [&]() { // Exécuté immédiatement ici
        auto& controls = _reg.get_components<Components::Controllable>();
        auto& positions = _reg.get_components<Components::Position>();
        auto& attacks = _reg.get_components<Components::AttackType>();

        for (auto player_ent : player_entity_set) { // Parcours les joueurs actifs
            // Si le joueur existe, a le composant Controllable, et veut tirer
            if (player_ent < controls.size() && controls[player_ent].has_value() && controls[player_ent]->Shoot) {
                // Vérifie le cooldown de tir pour ce joueur
                if (!shoot_cooldowns.count(player_ent) || shoot_cooldowns[player_ent] <= 0.f) {
                    // Si le cooldown est terminé, tire !
                    if(player_ent < positions.size() && positions[player_ent].has_value() &&
                       player_ent < attacks.size() && attacks[player_ent].has_value())
                    {
                        auto& pos = positions[player_ent].value();
                        auto type = attacks[player_ent].value();
                        // Appelle shoot_bullet pour créer l'entité balle
                        shoot_bullet(player_ent, type, pos.x + 30.f, pos.y); // Légèrement devant le joueur
                        shoot_cooldowns[player_ent] = 0.2f; // Réinitialise le cooldown (200ms)
                    }
                }
            }
        }
    };
    shoot_system(); // Exécute la logique de tir

    // --- Mise à jour des cooldowns ---
    for (auto& [ent, cooldown] : shoot_cooldowns) {
        if (cooldown > 0.f) {
             cooldown -= deltaTime; // Réduit le cooldown restant
        }
    }

    // --- Exécution des systèmes principaux ---
    // Appelle chaque fonction système enregistrée dans le constructeur
    for (auto& system_func : _systems) {
        system_func(deltaTime); // ✅ Passe deltaTime à chaque système
    }

    // --- Gestion des Collisions et LevelUp ---
    // Compare toutes les paires d'entités pour les collisions
    auto &positions = _reg.get_components<Components::Position>();
    size_t max_entity_index = positions.size(); // Pour éviter de dépasser les limites
    for (std::size_t i = 0; i < max_entity_index; ++i) {
        if (!positions[i].has_value()) continue; // Ignore si l'entité n'a pas de position
        // Compare avec toutes les entités suivantes (j > i)
        for (std::size_t j = i + 1; j < max_entity_index; ++j) {
            if (!positions[j].has_value()) continue; // Ignore si l'entité n'a pas de position

            // Appelle les logiques de collision et de level up
            collision_system_logic(entity(i), entity(j), collisionBound);
            levelUp_system_logic(entity(i), entity(j), collisionBound);
        }
    }
}

// Crée une entité ennemie
void Engine::spawn_enemy(Components::Enemystype type, float x, float y) {
    auto ent = _reg.spawn_entity();
    uint32_t newNetworkId = m_next_network_id++;
_reg.emplace_component<Components::NetworkId>(ent, newNetworkId);
    _reg.emplace_component<Components::Position>(ent, x, y);
    _reg.emplace_component<Components::Velocity>(ent); // Sera défini par l'IA
    _reg.emplace_component<Components::Enemystype>(ent, type);
    // Définit la vie et les dégâts en fonction du type d'ennemi (simplifié)
    int hp = 100;
    int dmg = 10;
    if (type >= Components::Enemystype::Dobkeratops) { // Boss
        hp = 500;
        dmg = 50;
    }
    _reg.emplace_component<Components::Healthpoints_t>(ent, hp);
    _reg.emplace_component<Components::Damage_t>(ent, dmg);
    // ✅ NOUVEAU : Donne un ID réseau temporaire ou unique aux ennemis ?
    // Pour l'instant, on ne leur donne pas de NetworkId, seuls les joueurs en ont un.
    // Le client devra les identifier par leur position ou un ID temporaire généré par le snapshot.
}
std::vector<OrbsInfo> Engine::getOrbs(void) {
    std::vector<OrbsInfo> ret;
    auto &powerUs = _reg.get_components<Components::PowerUp>();
    auto &positions = _reg.get_components<Components::Position>();
    auto &velocities = _reg.get_components<Components::Velocity>();
    auto &networkIds = _reg.get_components<Components::NetworkId>(); // ✅ NOUVEAU

    for (std::size_t i = 0; i < powerUs.size(); i++) {
        if (powerUs[i].has_value()) {
            uint32_t netId = 0;
            if (i < networkIds.size() && networkIds[i].has_value()) {
                netId = networkIds[i]->id;
            }
            
            ret.push_back(
                {
                    entity(i),
                    netId,                  // ✅ NOUVEAU
                    velocities[i].value(),
                    positions[i].value(),
                    powerUs[i].value()
                }
            );
        }
    }
    return ret;
}

std::vector<playerInfo> Engine::getPlayersInfo(void) {
    std::vector<playerInfo> ret;
    auto &positions = _reg.get_components<Components::Position>();
    auto &velocities = _reg.get_components<Components::Velocity>();
    auto &controls = _reg.get_components<Components::Controllable>();
    auto &health_ps = _reg.get_components<Components::Healthpoints_t>();
    auto &damages = _reg.get_components<Components::Damage_t>();
    auto &status = _reg.get_components<Components::PlayerStats>();
    auto &networkIds = _reg.get_components<Components::NetworkId>(); // ✅ NOUVEAU

    for (std::size_t i {0}; i < status.size(); i++) {
        if (status[i].has_value()) {
            // ✅ Récupère le NetworkId
            uint32_t netId = 0;
            if (i < networkIds.size() && networkIds[i].has_value()) {
                netId = networkIds[i]->id;
            }
            
            ret.push_back(
                {
                    (int)health_ps[i].value(),
                    entity(i),
                    netId,                      // ✅ NOUVEAU
                    (int)damages[i].value(),
                    velocities[i].value(),
                    positions[i].value(),
                    status[i].value(),
                    controls[i].value(),
                    health_ps[i].value()
                }
            );
        }
    }
    return ret;
}

std::vector<EnemyInfo> Engine::getEnemysInfo(void) {
    std::vector<EnemyInfo> ret;
    auto& positions = _reg.get_components<Components::Position>();
    auto& velocities = _reg.get_components<Components::Velocity>();
    auto& enemies = _reg.get_components<Components::Enemystype>();
    auto& networkIds = _reg.get_components<Components::NetworkId>(); // ✅ NOUVEAU

    for (std::size_t i {0}; i < enemies.size(); i++) {
        if (enemies[i].has_value()) {
            uint32_t netId = 0;
            if (i < networkIds.size() && networkIds[i].has_value()) {
                netId = networkIds[i]->id;
            }
            
            ret.push_back(
                {
                    entity(i),
                    netId,                  // ✅ NOUVEAU
                    velocities[i].value(),
                    positions[i].value(),
                    enemies[i].value()
                }
            );
        }
    }
    return ret;
}

std::vector<BulletInfo> Engine::getBulletsInfo(void) {
    std::vector<BulletInfo> ret;
    auto &positions = _reg.get_components<Components::Position>();
    auto &velocities = _reg.get_components<Components::Velocity>();
    auto &damages = _reg.get_components<Components::Damage_t>();
    auto &bullets = _reg.get_components<Components::Bullet>();
    auto &attacks = _reg.get_components<Components::AttackType>();
    auto &networkIds = _reg.get_components<Components::NetworkId>(); // ✅ NOUVEAU

    for (std::size_t i {0}; i < bullets.size(); i++) {
        if (bullets[i].has_value()) {
            uint32_t netId = 0;
            if (i < networkIds.size() && networkIds[i].has_value()) {
                netId = networkIds[i]->id;
            }
            
            ret.push_back(
                {
                    entity(i),
                    netId,                      // ✅ NOUVEAU
                    damages[i].value(),
                    velocities[i].value(),
                    positions[i].value(),
                    bullets[i].value(),
                    attacks[i].value()
                }
            );
        }
    }
    return ret;
}


#include <mutex>
static std::mutex g_score_mutex; // protège accès si jamais appelé depuis plusieurs threads (par sécurité)

void Engine::add_score(uint32_t playerNetworkId, int delta) {
    if (playerNetworkId == 0) return;
    // Safely find the entity and update PlayerStats.score
    // On suppose que player_id_to_entity_map mappe networkId -> entity
    auto it = player_id_to_entity_map.find(playerNetworkId);
    if (it == player_id_to_entity_map.end()) {
        // player not found (maybe disconnected)
        return;
    }
    entity ent = it->second;
    auto& statsArr = _reg.get_components<Components::PlayerStats>();
    std::size_t idx = static_cast<std::size_t>(ent);
    if (idx < statsArr.size() && statsArr[idx].has_value()) {
        std::lock_guard<std::mutex> lock(g_score_mutex);
        statsArr[idx]->score += delta;
    }
}

int Engine::get_player_score(uint32_t playerNetworkId) const {
    auto it = player_id_to_entity_map.find(playerNetworkId);
    if (it == player_id_to_entity_map.end()) return 0;
    entity ent = it->second;
    const auto& statsArr = _reg.get_components<Components::PlayerStats>();
    std::size_t idx = static_cast<std::size_t>(ent);
    if (idx < statsArr.size() && statsArr[idx].has_value()) {
        return statsArr[idx]->score;
    }
    return 0;
}

std::vector<Engine::PlayerScoreInfo> Engine::collect_all_player_scores() const {
    std::vector<PlayerScoreInfo> out;
    const auto& statsArr = _reg.get_components<Components::PlayerStats>();
    const auto& networkIds = _reg.get_components<Components::NetworkId>();
    for (std::size_t i = 0; i < statsArr.size(); ++i) {
        if (statsArr[i].has_value()) {
            uint32_t netid = 0;
            if (i < networkIds.size() && networkIds[i].has_value()) netid = networkIds[i]->id;
            Engine::PlayerScoreInfo info;
            info.playerId = netid;
            info.score = statsArr[i]->score;
            // kills/deaths: si tu trackes, lis-les sinon 0
            info.kills = 0;
            info.deaths = 0;
            out.push_back(info);
        }
    }
    return out;
}