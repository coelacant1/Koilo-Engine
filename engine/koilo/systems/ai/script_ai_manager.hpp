// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file script_ai_manager.hpp
 * @brief Script-facing AI manager - non-polymorphic, non-template, reflectable.
 *
 * Provides KoiloScript access to:
 * - StateMachine (create, add states, transitions, start/stop/update)
 * - BehaviorTree (create, tick)
 * - PathfinderGrid (create grid, set walkability, find path)
 */

#pragma once

#include <koilo/systems/ai/statemachine/statemachine.hpp>
#include <koilo/systems/ai/behaviortree.hpp>
#include <koilo/systems/ai/pathfinding/pathfinder.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <string>
#include <unordered_map>
#include <memory>

namespace koilo {

/**
 * @class ScriptAIManager
 * @brief Non-polymorphic AI facade for KoiloScript.
 */
class ScriptAIManager {
public:
    ScriptAIManager();
    ~ScriptAIManager();

    // -- State Machine -----------------------------------------------
    void CreateStateMachine(const std::string& name);
    void AddState(const std::string& machineName, const std::string& stateName);
    void SetInitialState(const std::string& machineName, const std::string& stateName);
    void StartMachine(const std::string& machineName);
    void StopMachine(const std::string& machineName);
    void TransitionTo(const std::string& machineName, const std::string& stateName);
    std::string GetCurrentState(const std::string& machineName) const;
    void UpdateMachine(const std::string& machineName);

    // -- Pathfinding -------------------------------------------------
    void CreateGrid(const std::string& name, int width, int height, bool diagonal);
    void SetWalkable(const std::string& gridName, int x, int y, bool walkable);
    void SetCost(const std::string& gridName, int x, int y, float cost);
    bool FindPath(const std::string& gridName, int sx, int sy, int gx, int gy);
    int GetPathLength() const;
    int GetPathX(int index) const;
    int GetPathY(int index) const;

    // -- Update all --------------------------------------------------
    void Update();

    // -- Direct C++ access -------------------------------------------
    StateMachine* GetStateMachine(const std::string& name);
    PathfinderGrid* GetGrid(const std::string& name);

    // -- Reflection --------------------------------------------------
    KL_BEGIN_FIELDS(ScriptAIManager)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ScriptAIManager)
        KL_METHOD_AUTO(ScriptAIManager, CreateStateMachine, "Create state machine"),
        KL_METHOD_AUTO(ScriptAIManager, AddState, "Add state to machine"),
        KL_METHOD_AUTO(ScriptAIManager, SetInitialState, "Set initial state"),
        KL_METHOD_AUTO(ScriptAIManager, StartMachine, "Start state machine"),
        KL_METHOD_AUTO(ScriptAIManager, StopMachine, "Stop state machine"),
        KL_METHOD_AUTO(ScriptAIManager, TransitionTo, "Force transition"),
        KL_METHOD_AUTO(ScriptAIManager, GetCurrentState, "Get current state name"),
        KL_METHOD_AUTO(ScriptAIManager, UpdateMachine, "Update one state machine"),
        KL_METHOD_AUTO(ScriptAIManager, CreateGrid, "Create pathfinding grid"),
        KL_METHOD_AUTO(ScriptAIManager, SetWalkable, "Set grid cell walkability"),
        KL_METHOD_AUTO(ScriptAIManager, SetCost, "Set grid cell cost"),
        KL_METHOD_AUTO(ScriptAIManager, FindPath, "Find A* path"),
        KL_METHOD_AUTO(ScriptAIManager, GetPathLength, "Get last path length"),
        KL_METHOD_AUTO(ScriptAIManager, GetPathX, "Get path node X"),
        KL_METHOD_AUTO(ScriptAIManager, GetPathY, "Get path node Y"),
        KL_METHOD_AUTO(ScriptAIManager, Update, "Update all state machines")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ScriptAIManager)
        KL_CTOR0(ScriptAIManager)
    KL_END_DESCRIBE(ScriptAIManager)

private:
    std::unordered_map<std::string, std::unique_ptr<StateMachine>> machines_;
    std::unordered_map<std::string, std::unique_ptr<PathfinderGrid>> grids_;
    std::vector<GridNode> lastPath_;
};

} // namespace koilo
