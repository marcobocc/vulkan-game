#pragma once
#include <tuple>
#include "ComponentStorage.hpp"

template<typename... Components>
class GameEntitiesManager {
public:
    using EntityId = uint32_t;

    EntityId createEntity() {
        EntityId id = nextEntityId_++;
        return id;
    }

    void destroyEntity(EntityId entity) { removeAllComponents(entity); }

    template<typename Component>
    Component& getComponent(EntityId entity) {
        return getStorage<Component>().getComponent(entity);
    }

    template<typename Component>
    void addComponent(EntityId entity, const Component& component) {
        getStorage<Component>().addComponent(entity, component);
    }

    template<typename Component>
    void removeComponent(EntityId entity) {
        getStorage<Component>().removeComponent(entity);
    }

private:
    void removeAllComponents(EntityId entity) {
        (std::get<ComponentStorage<Components, EntityId>>(componentStorages_).removeComponent(entity), ...);
    }

    template<typename Component>
    ComponentStorage<Component, EntityId>& getStorage() {
        return std::get<ComponentStorage<Component, EntityId>>(componentStorages_);
    }

    EntityId nextEntityId_ = 0;
    std::tuple<ComponentStorage<Components, EntityId>...> componentStorages_;
};
