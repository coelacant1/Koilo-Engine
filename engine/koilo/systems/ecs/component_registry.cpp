// SPDX-License-Identifier: GPL-3.0-or-later
#include "component_registry.hpp"

namespace koilo {

ComponentRegistry::ComponentRegistry()
    : nextId_(ComponentTypeIDGenerator::GetCount()) {}

ComponentTypeID ComponentRegistry::Register(std::unique_ptr<IComponentType> type) {
    if (!type) return UINT32_MAX;

    std::string name = type->GetName();

    // Reject duplicate names
    if (nameIndex_.find(name) != nameIndex_.end()) {
        return entries_[nameIndex_[name]].id;
    }

    ComponentTypeID id = nextId_++;
    size_t idx = entries_.size();
    entries_.push_back({id, std::move(type)});
    nameIndex_[name] = idx;
    return id;
}

const IComponentType* ComponentRegistry::FindByName(const std::string& name) const {
    auto it = nameIndex_.find(name);
    if (it == nameIndex_.end()) return nullptr;
    return entries_[it->second].type.get();
}

ComponentTypeID ComponentRegistry::FindIDByName(const std::string& name) const {
    auto it = nameIndex_.find(name);
    if (it == nameIndex_.end()) return UINT32_MAX;
    return entries_[it->second].id;
}

const IComponentType* ComponentRegistry::GetType(ComponentTypeID id) const {
    for (auto& e : entries_) {
        if (e.id == id) return e.type.get();
    }
    return nullptr;
}

std::vector<std::string> ComponentRegistry::List() const {
    std::vector<std::string> names;
    names.reserve(entries_.size());
    for (auto& e : entries_) {
        names.push_back(e.type->GetName());
    }
    return names;
}

size_t ComponentRegistry::Count() const {
    return entries_.size();
}

} // namespace koilo
