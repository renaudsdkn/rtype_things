#pragma once // Bonne pratique

// Includes standards nécessaires
#include <memory>
#include <optional>
#include <vector>
#include <unordered_map>
#include <any>
#include <stdexcept>
#include <functional>
#include <cstddef>
#include <iostream>
#include <typeindex>
#include <cmath>
#include <set>
#include <cstdint> // Pour uint32_t

// ============================================================
// DÉFINITION DE L'ECS (REGISTRY, SPARSE_ARRAY, ENTITY)
// ============================================================

namespace ECS {
    // --- Classe Entity ---
    class entity {
    private:
        std::size_t _val = 0;
    public:
        entity() = default;
        explicit entity(std::size_t val) : _val(val) {};
        operator std::size_t() const { return _val; };
        entity& operator=(std::size_t val) { _val = val; return *this; };
        friend class registry; // Permet à la registry d'accéder à _val si besoin
    };
    using entity_t = entity; // Alias pratique

    // --- Classe SparseArray ---
    // Gère un type de composant pour toutes les entités
    template <typename Component, typename Alloc = std::allocator<std::optional<Component>>>
    class sparse_array {
    public:
        using value_type = std::optional<Component>; // Un composant peut exister ou non pour une entité
        using reference_type = value_type&;
        using const_reference_type = value_type const&;
        using container_t = std::vector<value_type, Alloc>;
        using size_type = typename container_t::size_type;
        using iterator = typename container_t::iterator;
        using const_iterator = typename container_t::const_iterator;
    private:
        container_t _data; // Le vecteur qui stocke les composants
    public:
        sparse_array() = default; // Constructeur par défaut

        // Accès direct (peut redimensionner le vecteur si l'index est hors limites)
        reference_type operator[](size_t idx) {
            if (idx >= _data.size()) _data.resize(idx + 1); // Redimensionne si nécessaire
            return _data[idx];
        }
        // Accès constant (vérifie les limites)
        const_reference_type operator[](size_t idx) const {
            if (idx >= _data.size()) throw std::out_of_range("Index out of range!");
            return _data[idx];
        }

        // Itérateurs pour parcourir les composants
        iterator begin() { return _data.begin(); }
        const_iterator begin() const { return _data.begin(); }
        iterator end() { return _data.end(); }
        const_iterator end() const { return _data.end(); }

        // Taille actuelle du vecteur (nombre max d'entités + 1)
        size_type size() const { return _data.size(); }

        // Insère ou met à jour un composant à un index donné (par copie)
        reference_type insert_at(size_type pos, Component const& a) {
            if (pos >= _data.size()) _data.resize(pos + 1);
            _data[pos] = a;
            return _data[pos];
        }
        // Version optimisée pour les rvalues (par déplacement)
        reference_type insert_at(size_type pos, Component&& a) {
            if (pos >= _data.size()) _data.resize(pos + 1);
            _data[pos] = std::move(a);
            return _data[pos];
        }
        // Construit un composant directement en mémoire à l'index donné
        template<class... Params>
        reference_type emplace_at(size_type pos, Params&&... params) {
            if (pos >= _data.size()) _data.resize(pos + 1);
            _data[pos].emplace(std::forward<Params>(params)...); // Utilise le constructeur de Component
            return _data[pos];
        }

        // Supprime un composant (marque l'optional comme vide)
        void erase(size_type pos) {
            if (pos < _data.size()) _data[pos].reset(); // std::optional::reset()
        }
    };

    // --- Classe Registry ---
    // Le cœur de l'ECS, gère toutes les sparse_array et les entités
    class registry {
    private:
        // Map qui associe un type de composant (via type_index) à sa sparse_array (stockée dans un std::any)
        std::unordered_map<std::type_index, std::any> _components_arrays;
        // Liste de fonctions pour effacer tous les composants d'une entité lors de kill_entity
        std::vector<std::function<void(registry&, entity_t const&)>> _erase_functions;
        // Compteur pour le prochain ID d'entité unique
        std::size_t _next_entity_id = 0;
        // Liste des ID d'entités "mortes" et réutilisables
        std::vector<std::size_t> _dead_entities;

    public:
        // Enregistre un nouveau type de composant
        template<class Component>
        sparse_array<Component>& register_component() {
            using SA = sparse_array<Component>;
            auto key = std::type_index(typeid(Component)); // Clé basée sur le type

            auto it = _components_arrays.find(key);
            if (it == _components_arrays.end()) { // Si ce type n'est pas encore enregistré
                // Crée une nouvelle sparse_array et la stocke dans la map
                auto [new_it, inserted] = _components_arrays.try_emplace(key, SA{});
                it = new_it;

                // Ajoute une fonction à la liste d'effacement pour ce type de composant
                _erase_functions.push_back(
                    [](registry& reg, entity_t const& e) {
                        auto& arr = reg.get_components<Component>(); // Récupère la bonne sparse_array
                        std::size_t idx = static_cast<std::size_t>(e); // Convertit l'entité en index
                        if (idx < arr.size()) arr.erase(idx); // Efface le composant s'il existe
                    });
            }
            // Retourne une référence à la sparse_array (via std::any_cast)
            return std::any_cast<SA&>(it->second);
        }

        // Récupère la sparse_array pour un type de composant (crée si n'existe pas)
        template<class Component>
        sparse_array<Component>& get_components() {
            auto key = std::type_index(typeid(Component));
            auto it = _components_arrays.find(key);
            if (it == _components_arrays.end()) {
                 // Si le composant n'était pas enregistré, on l'enregistre maintenant
                return register_component<Component>();
            }
            return std::any_cast<sparse_array<Component>&>(it->second);
        }
        // Version constante pour récupérer la sparse_array
        template<class Component>
        const sparse_array<Component>& get_components() const {
            auto key = std::type_index(typeid(Component));
            auto it = _components_arrays.find(key);
            if (it == _components_arrays.end()) {
                throw std::out_of_range("Component not registered for const access!"); // Erreur si non enregistré en const
            }
            return std::any_cast<const sparse_array<Component>&>(it->second);
        }

        // Crée une nouvelle entité (réutilise un ID mort si possible)
        entity_t spawn_entity() {
            if (!_dead_entities.empty()) { // S'il y a des ID réutilisables
                std::size_t id = _dead_entities.back(); // Prend le dernier ID mort
                _dead_entities.pop_back();
                return entity_t{id}; // Retourne une entité avec cet ID
            }
            // Sinon, crée un nouvel ID et l'incrémente
            return entity_t{_next_entity_id++};
        }

        // Marque une entité comme morte et efface tous ses composants
        void kill_entity(entity_t const& e) {
            // Appelle toutes les fonctions d'effacement enregistrées pour cette entité
            for (auto& fn : _erase_functions) {
                fn(*this, e);
            }
            // Ajoute l'ID de l'entité à la liste des ID réutilisables
            _dead_entities.push_back(static_cast<std::size_t>(e));
        }

        // Raccourci pour ajouter un composant à une entité (par copie ou déplacement)
        template <typename Component>
        typename sparse_array<Component>::reference_type
        add_component(entity_t const &to, Component &&c) {
            auto& arr = get_components<Component>(); // Récupère ou enregistre la sparse_array
            std::size_t idx = static_cast<std::size_t>(to); // Convertit l'entité en index
            return arr.insert_at(idx, std::forward<Component>(c)); // Insère le composant
        }
        // Raccourci pour construire un composant directement pour une entité
        template<typename Component, typename ...Params>
        typename sparse_array<Component>::reference_type
        emplace_component(entity_t const &to, Params &&...p) {
            auto& arr = get_components<Component>();
            std::size_t idx = static_cast<std::size_t>(to);
            return arr.emplace_at(idx, std::forward<Params>(p)...); // Construit le composant
        }

        // Raccourci pour supprimer un composant spécifique d'une entité
        template<typename Component>
        void remove_component(entity_t const& from) {
            auto& arr = get_components<Component>();
            std::size_t idx = static_cast<std::size_t>(from);
            if (idx < arr.size()) {
                arr.erase(idx); // Appelle erase sur la sparse_array
            }
        }
    };
} // namespace ECS

// ============================================================
// DÉFINITION DES COMPOSANTS DE JEU (Anciennement dans engine.hpp)
// ============================================================

namespace Components {
    // --- Composants de base ---
    struct Velocity { float x{0.f}, y{0.f}; Velocity(float vx=0.f, float vy=0.f):x(vx),y(vy){}};
    struct Position {
        float x{0.f}, y{0.f};
        Position(float vx=0.f, float vy=0.f):x(vx),y(vy){};
        // Opérateur pratique pour appliquer la vélocité
        Position& operator+=(const Velocity& v) { x += v.x; y += v.y; return *this; }
        // Opérateur pour calculer la distance (utilisé dans les collisions)
        float operator-(const Position& other) const {
            float dx = x - other.x; float dy = y - other.y;
            return std::sqrt(dx * dx + dy * dy);
        }
    };
    // Composant pour marquer une entité comme contrôlable par un joueur
    struct Controllable {
        // Ces booléens seront mis à jour par handle_input dans l'Engine
        bool Up{false}, Down{false}, Left{false}, Right{false}, Shoot{false};
    };

    // --- Composants spécifiques au jeu ---
    enum Enemystype { None, Grubs, Flyers, Turrets, Eyes, Squids, Moths, Crabs, Gargoyle, Dobkeratops, Gel, Hades, Gomorrah, TheCore };
    enum AttackType { WaveCanon = -1, StdShot = 1, RoundShot, BounceShot, StraightShot, RippleShot };

    struct Damage_t {
        int _damage{10};
        // Opérateurs pratiques
        Damage_t &operator = (std::size_t damage) { _damage = damage; return *this; }
    };
    struct Healthpoints_t {
        int _hp{100};
        // Opérateurs pratiques
        Healthpoints_t &operator = (std::size_t hp) { _hp = hp; return *this; }
        Healthpoints_t &operator -= (Damage_t damage) { _hp -= damage._damage; return *this; }
        bool operator <= (int i) const { return _hp <= i; } // Utilisé dans killEntities_system
    };
    struct PlayerStats { int xp {0}, level{1}; }; // Pour le système de niveau
    struct Bullet { bool active{true}; }; // Pour marquer les balles actives/inactives
    struct PowerUp { bool is{true}; }; // Pour marquer les power-ups

    // --- Composant crucial pour le réseau ---
    // NOUVEAU : Lie l'entité ECS à l'ID réseau du joueur/monstre etc.
    // Cet ID sera utilisé dans les snapshots envoyés aux clients.
    struct NetworkId { uint32_t id;NetworkId(uint32_t v=0):id(v){} };
    // NOUVEAU : Composant pour l'affichage
    struct Drawable {
        // Stocke le 'type' reçu du serveur.
        // Le système de rendu saura quelle texture/couleur utiliser.
        uint8_t serverEntityType;
        // Optionnel : Tu pourrais ajouter ici la taille, la couleur, l'ID de texture, etc.
        float width = 30.0f; // Taille par défaut
        float height = 30.0f; // Taille par défaut
    };

    // NOUVEAU : Composant pour marquer l'entité locale du joueur (optionnel mais utile)
  
    // SUPPRIMÉ : Les composants graphiques (Idrawable, Sprite, Circle...)
    // Ils n'ont pas leur place dans la librairie partagée. Le client devra avoir
    // ses propres composants graphiques.
}