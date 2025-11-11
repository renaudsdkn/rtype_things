#include "../include/client/GameClient.hpp"
#include <set> // Pour std::set
#include <vector> // Pour std::vector (si besoin)
#include <iostream> // Pour les logs
#include "raylib.h" // Pour DrawRectangleRec et les couleurs

// Dans rtype/client/src/GameClient.cpp

GameClient::PlayerLocalStats GameClient::getLocalPlayerStats() const {
    PlayerLocalStats stats;
    
    if (!m_localPlayerNetworkId.has_value() || 
        (m_state != State::PLAYING && m_state != State::GAME_OVER)) {
        return stats;
    }
    
    const auto& networkIds = m_registry.get_components<Components::NetworkId>();
    const auto& healths = m_registry.get_components<Components::Healthpoints_t>();
    const auto& playerStatsComp = m_registry.get_components<Components::PlayerStats>();
    const auto& attacks = m_registry.get_components<Components::AttackType>();
    
    // Cherche l'entité du joueur local
    for (size_t i = 0; i < networkIds.size(); ++i) {
        if (networkIds[i].has_value() && 
            networkIds[i]->id == m_localPlayerNetworkId.value()) {
            
            // ✅ Santé
            if (i < healths.size() && healths[i].has_value()) {
                stats.health = healths[i].value();
                stats.maxHealth = 100;
            }
            
            // ✅ XP & Level (directement depuis le serveur)
            if (i < playerStatsComp.size() && playerStatsComp[i].has_value()) {
                stats.xp = playerStatsComp[i]->xp;
                stats.level = playerStatsComp[i]->level;
                
                // ✅ On utilise les MÊMES valeurs que l'Engine (hard-codées ici aussi)
                // Alternative : On pourrait aussi ne PAS afficher la barre d'XP
                static const int XP_THRESHOLDS[] = {0, 100, 150, 200, 300, 500, -1};
                if (stats.level >= 1 && stats.level <= 5) {
                    stats.xpForNextLevel = XP_THRESHOLDS[stats.level];
                } else {
                    stats.xpForNextLevel = -1; // Niveau max
                }
            }
            
            // ✅ Arme (directement depuis le serveur)
            if (i < attacks.size() && attacks[i].has_value()) {
                // Correspondance AttackType → Nom (même logique que l'Engine)
                switch(attacks[i].value()) {
                    case Components::AttackType::StdShot: 
                        stats.weaponName = "Standard Shot"; break;
                    case Components::AttackType::RoundShot: 
                        stats.weaponName = "Round Shot"; break;
                    case Components::AttackType::BounceShot: 
                        stats.weaponName = "Bounce Shot"; break;
                    case Components::AttackType::StraightShot: 
                        stats.weaponName = "Straight Shot"; break;
                    case Components::AttackType::RippleShot: 
                        stats.weaponName = "Ripple Shot"; break;
                    case Components::AttackType::WaveCanon: 
                        stats.weaponName = "Wave Canon"; break;
                    default: 
                        stats.weaponName = "Unknown"; break;
                }
            }
            
            break; // Joueur trouvé
        }
    }
    
    return stats;
}

RenderData GameClient::getRenderData() const {
    RenderData data;
    
    // Ne retourne rien si pas en jeu
    if (m_state != State::PLAYING && m_state != State::GAME_OVER) {
        return data;
    }
    
    // Récupère les sparse_arrays de l'ECS client
    const auto& positions = m_registry.get_components<Components::Position>();
    const auto& velocities = m_registry.get_components<Components::Velocity>();
    const auto& networkIds = m_registry.get_components<Components::NetworkId>();
    const auto& drawables = m_registry.get_components<Components::Drawable>();
    const auto& healths = m_registry.get_components<Components::Healthpoints_t>();
    const auto& damages = m_registry.get_components<Components::Damage_t>();
    const auto& playerStats = m_registry.get_components<Components::PlayerStats>();
    const auto& attacks = m_registry.get_components<Components::AttackType>();
    const auto& controllables = m_registry.get_components<Components::Controllable>();
    const auto& bullets = m_registry.get_components<Components::Bullet>();
    const auto& powerUps = m_registry.get_components<Components::PowerUp>();
    const auto& enemyTypes = m_registry.get_components<Components::Enemystype>();
    
    // Parcours toutes les entités
    for (size_t i = 0; i < positions.size(); ++i) {
        if (!positions[i].has_value()) continue;
        
        const auto& pos = positions[i].value();
        ECS::entity_t ent(i);
        
        // Récupère la vélocité (optionnelle)
        Components::Velocity vel{0.f, 0.f};
        if (i < velocities.size() && velocities[i].has_value()) {
            vel = velocities[i].value();
        }
        
        // ✅ Identifie le TYPE d'entité et remplit la structure appropriée
        
        // --- Joueurs ---
        if (i < playerStats.size() && playerStats[i].has_value()) {
            playerInfo pInfo;
            pInfo.id = ent;
            pInfo.pos = pos;
            pInfo.vel = vel;
            pInfo.stat = playerStats[i].value();
            pInfo.hp = (i < healths.size() && healths[i].has_value()) ? healths[i].value() : 100;
            pInfo.damage = (i < damages.size() && damages[i].has_value()) ? damages[i].value() : 20;
            if (i < controllables.size() && controllables[i].has_value()) {
                pInfo.control = controllables[i].value();
            }
            data.players.push_back(pInfo);
        }
        // --- Ennemis ---
        else if (i < enemyTypes.size() && enemyTypes[i].has_value()) {
            EnemyInfo eInfo;
            eInfo.id = ent;
            eInfo.pos = pos;
            eInfo.vel = vel;
            eInfo.enemy = enemyTypes[i].value();
            data.enemies.push_back(eInfo);
        }
        // --- Balles ---
        else if (i < bullets.size() && bullets[i].has_value() && bullets[i]->active) {
            BulletInfo bInfo;
            bInfo.id = ent;
            bInfo.pos = pos;
            bInfo.vel = vel;
            bInfo.bullet = bullets[i].value();
            bInfo.dmg = (i < damages.size() && damages[i].has_value()) ? damages[i].value() : 10;
            bInfo.type = (i < attacks.size() && attacks[i].has_value()) 
                         ? attacks[i].value() 
                         : Components::AttackType::StdShot;
            data.bullets.push_back(bInfo);
        }
        // --- Power-Ups ---
        else if (i < powerUps.size() && powerUps[i].has_value()) {
            OrbsInfo oInfo;
            oInfo.id = ent;
            oInfo.pos = pos;
            oInfo.vel = vel;
            oInfo.power = powerUps[i].value();
            data.orbs.push_back(oInfo);
        }
    }
    
    return data;
}
