#ifndef ECS_HPP_
    #define ECS_HPP_

#pragma once
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
#include <algorithm>
#include "raylib.h"

// Déclaration anticipée
class registry;


class entity {
private:
    std::size_t _val = 0;

public:
    entity() = default;
    explicit entity(std::size_t val) : _val(val) {};
    operator std::size_t() const { return _val; };
    entity& operator=(std::size_t val) { _val = val; return *this; };
    friend class registry;


};

using entity_t = entity;

// ============================================================
// SPARSE ARRAY (Omitted for brevity, assumed functional)
// ============================================================

template <
    typename Component,
    typename Allocator = std::allocator<std::optional<Component>>
>
class sparse_array {
public:
    using value_type = std::optional<Component>;
    using reference_type = value_type&;
    using const_reference_type = value_type const&;
    using container_t = std::vector<value_type, Allocator>;
    using size_type = typename container_t::size_type;
    using iterator = typename container_t::iterator;
    using const_iterator = typename container_t::const_iterator;

public:
    sparse_array() = default;

    reference_type insert_at(entity_t const& to, Component const& c) {
        std::size_t idx = static_cast<std::size_t>(to);
        if (idx >= _data.size()) _data.resize(idx + 1);
        _data[idx] = c;
        return _data[idx];
    }

    template<typename ...Params>
    reference_type emplace_at(entity_t const& to, Params &&...p) {
        std::size_t idx = static_cast<std::size_t>(to);
        if (idx >= _data.size()) _data.resize(idx + 1);
        _data[idx].emplace(std::forward<Params>(p)...);
        return _data[idx];
    }

    void erase(entity_t const& from) {
        std::size_t idx = static_cast<std::size_t>(from);
        if (idx < _data.size()) {
            _data[idx].reset();
        }
    }

    size_type size() const { return _data.size(); }
    const_reference_type operator[](size_type idx) const { return _data[idx]; }
    reference_type operator[](size_type idx) { return _data[idx]; }

    // Minimal iterator support for systems
    iterator begin() { return _data.begin(); }
    const_iterator begin() const { return _data.begin(); }
    iterator end() { return _data.end(); }
    const_iterator end() const { return _data.end(); }

private:
    container_t _data;
};

// ============================================================
// REGISTRY (Omitted for brevity, assumed functional)
// ============================================================

class registry {
public:
    template<typename Component>
    sparse_array<Component>& register_component() {
        if (_components.count(typeid(Component))) {
            throw std::runtime_error("Component already registered");
        }
        auto arr_ptr = std::make_shared<sparse_array<Component>>();
        _components[typeid(Component)] = arr_ptr;
        return *arr_ptr;
    }

    template<typename Component>
    sparse_array<Component>& get_components() {
        if (!_components.count(typeid(Component))) {
            throw std::runtime_error("Component not registered");
        }
        return *std::any_cast<std::shared_ptr<sparse_array<Component>>>(_components[typeid(Component)]);
    }

    entity_t spawn_entity() {
        if (!_free_list.empty()) {
            entity_t e = _free_list.back();
            _free_list.pop_back();
            return e;
        }
        return entity_t(_next_entity_id++);
    }

    void kill_entity(entity_t const& e) {
        // En vrai, il faudrait parcourir toutes les sparse_array et appeler remove_component
        // pour simplifier on va juste ajouter a la free list.
        _free_list.push_back(e);
        // La suppression réelle dans les sparse_array sera gérée par une fonction utilitaire
        // de haut niveau si nécessaire, ou plus tard dans le projet.
    }

    template<typename Component>
    typename sparse_array<Component>::reference_type
    add_component(entity_t const &to, Component &&c) {
        auto& arr = get_components<Component>();
        std::size_t idx = static_cast<std::size_t>(to);
        return arr.insert_at(idx, std::forward<Component>(c));
    }

    template<typename Component, typename ...Params>
    typename sparse_array<Component>::reference_type
    emplace_component(entity_t const &to, Params &&...p) {
        auto& arr = get_components<Component>();
        std::size_t idx = static_cast<std::size_t>(to);
        return arr.emplace_at(idx, std::forward<Params>(p)...);
    }

    template<typename Component>
    void remove_component(entity_t const& from) {
        auto& arr = get_components<Component>();
        std::size_t idx = static_cast<std::size_t>(from);
        if (idx < arr.size()) {
            arr.erase(idx);
        }
    }

private:
    std::unordered_map<std::type_index, std::any> _components;
    std::size_t _next_entity_id = 0;
    std::vector<entity_t> _free_list;
};


// ============================================================
// COMPONENTS
// ============================================================

struct Position {
    float x = 0;
    float y = 0;
    Position operator+(const Position& pos) const { return {x + pos.x, y + pos.y}; }
};

struct Velocity {
    float x = 0;
    float y = 0;
    Velocity& operator+=(const Velocity& v) {
        x += v.x;
        y += v.y;
        return *this;
    }
};

struct controllable {
    bool Up{false};
    bool Down{false};
    bool Left{false};
    bool Right{false};
    bool Shoot{false}; // AJOUTÉ : Pour l'événement de tir
};

// AJOUTÉ : Composant pour l'affichage de débogage (sera remplacé par un composant Sprite/Texture)
struct Displayable {
    uint32_t textureId; // ID pour la texture/sprite
    float width;
    float height;
};

// AJOUTÉ : Composant pour l'identifiant réseau de l'entité côté serveur.
struct NetworkId {
    uint32_t id;
};

// ============================================================
// ECS SYSTEMS (Déclarations)
// ============================================================
void control_system(registry& r);
void movement_system(registry& r);
void rendering_debug_system(registry& r);

#endif /* !ECS_HPP_ */
