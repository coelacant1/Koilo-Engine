// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file script_ai_manager.cpp
 * @brief ScriptAIManager implementation - bridges AI to KoiloScript.
 */

#include <koilo/systems/ai/script_ai_manager.hpp>

namespace koilo {

ScriptAIManager::ScriptAIManager() {}
ScriptAIManager::~ScriptAIManager() {}

// -- State Machine -----------------------------------------------------------

void ScriptAIManager::CreateStateMachine(const std::string& name) {
    machines_[name] = std::make_unique<StateMachine>();
}

void ScriptAIManager::AddState(const std::string& machineName, const std::string& stateName) {
    auto it = machines_.find(machineName);
    if (it != machines_.end()) {
        it->second->AddState(stateName);
    }
}

void ScriptAIManager::SetInitialState(const std::string& machineName, const std::string& stateName) {
    auto it = machines_.find(machineName);
    if (it != machines_.end()) {
        it->second->SetInitialState(stateName);
    }
}

void ScriptAIManager::StartMachine(const std::string& machineName) {
    auto it = machines_.find(machineName);
    if (it != machines_.end()) {
        it->second->Start();
    }
}

void ScriptAIManager::StopMachine(const std::string& machineName) {
    auto it = machines_.find(machineName);
    if (it != machines_.end()) {
        it->second->Stop();
    }
}

void ScriptAIManager::TransitionTo(const std::string& machineName, const std::string& stateName) {
    auto it = machines_.find(machineName);
    if (it != machines_.end()) {
        it->second->TransitionTo(stateName);
    }
}

std::string ScriptAIManager::GetCurrentState(const std::string& machineName) const {
    auto it = machines_.find(machineName);
    if (it != machines_.end()) {
        return it->second->GetCurrentStateName();
    }
    return "";
}

void ScriptAIManager::UpdateMachine(const std::string& machineName) {
    auto it = machines_.find(machineName);
    if (it != machines_.end()) {
        it->second->Update();
    }
}

// -- Pathfinding -------------------------------------------------------------

void ScriptAIManager::CreateGrid(const std::string& name, int width, int height, bool diagonal) {
    grids_[name] = std::make_unique<PathfinderGrid>(width, height, diagonal);
}

void ScriptAIManager::SetWalkable(const std::string& gridName, int x, int y, bool walkable) {
    auto it = grids_.find(gridName);
    if (it != grids_.end()) {
        it->second->SetWalkable(x, y, walkable);
    }
}

void ScriptAIManager::SetCost(const std::string& gridName, int x, int y, float cost) {
    auto it = grids_.find(gridName);
    if (it != grids_.end()) {
        it->second->SetCost(x, y, cost);
    }
}

bool ScriptAIManager::FindPath(const std::string& gridName, int sx, int sy, int gx, int gy) {
    lastPath_.clear();
    auto it = grids_.find(gridName);
    if (it != grids_.end()) {
        return it->second->FindPath(sx, sy, gx, gy, lastPath_);
    }
    return false;
}

int ScriptAIManager::GetPathLength() const {
    return static_cast<int>(lastPath_.size());
}

int ScriptAIManager::GetPathX(int index) const {
    if (index >= 0 && index < static_cast<int>(lastPath_.size())) {
        return lastPath_[index].x;
    }
    return -1;
}

int ScriptAIManager::GetPathY(int index) const {
    if (index >= 0 && index < static_cast<int>(lastPath_.size())) {
        return lastPath_[index].y;
    }
    return -1;
}

// -- Update all --------------------------------------------------------------

void ScriptAIManager::Update() {
    for (auto& [name, machine] : machines_) {
        machine->Update();
    }
}

// -- Direct access -----------------------------------------------------------

StateMachine* ScriptAIManager::GetStateMachine(const std::string& name) {
    auto it = machines_.find(name);
    return (it != machines_.end()) ? it->second.get() : nullptr;
}

PathfinderGrid* ScriptAIManager::GetGrid(const std::string& name) {
    auto it = grids_.find(name);
    return (it != grids_.end()) ? it->second.get() : nullptr;
}

} // namespace koilo
