/**
 * @file statemachine.hpp
 * @brief Finite state machine for AI behaviors.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include "../../../registry/reflect_macros.hpp"

namespace ptx {

// Forward declarations
class State;
class StateMachine;

/**
 * @typedef StateTransitionCondition
 * @brief Condition function for state transitions (returns true if transition should occur).
 */
using StateTransitionCondition = std::function<bool()>;

/**
 * @struct StateTransition
 * @brief Represents a transition between states.
 */
struct StateTransition {
    std::string targetStateName;          ///< Target state name
    StateTransitionCondition condition;   ///< Transition condition

    StateTransition(const std::string& target, StateTransitionCondition cond)
        : targetStateName(target), condition(cond) {}

    PTX_BEGIN_FIELDS(StateTransition)
        PTX_FIELD(StateTransition, targetStateName, "Target state name", 0, 0),
        PTX_FIELD(StateTransition, condition, "Condition", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(StateTransition)
        /* No reflected methods. */
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(StateTransition)
        PTX_CTOR(StateTransition, const std::string &, StateTransitionCondition)
    PTX_END_DESCRIBE(StateTransition)

};

/**
 * @class State
 * @brief Represents a single state in a state machine.
 */
class State {
private:
    std::string name;
    std::function<void()> onEnter;           ///< Called when entering state
    std::function<void(float)> onUpdate;     ///< Called every frame while in state
    std::function<void()> onExit;            ///< Called when exiting state
    std::vector<StateTransition> transitions;

public:
    /**
     * @brief Constructor.
     */
    State(const std::string& name = "State");

    /**
     * @brief Gets the state name.
     */
    std::string GetName() const { return name; }

    /**
     * @brief Sets the enter callback.
     */
    void SetOnEnter(std::function<void()> callback) { onEnter = callback; }

    /**
     * @brief Sets the update callback.
     */
    void SetOnUpdate(std::function<void(float)> callback) { onUpdate = callback; }

    /**
     * @brief Sets the exit callback.
     */
    void SetOnExit(std::function<void()> callback) { onExit = callback; }

    /**
     * @brief Adds a transition to another state.
     */
    void AddTransition(const std::string& targetState, StateTransitionCondition condition);

    /**
     * @brief Called when entering this state.
     */
    void Enter();

    /**
     * @brief Called every frame while in this state.
     */
    void Update(float deltaTime);

    /**
     * @brief Called when exiting this state.
     */
    void Exit();

    /**
     * @brief Checks transitions and returns target state if condition met.
     * @return Target state name, or empty string if no transition.
     */
    std::string CheckTransitions();

    PTX_BEGIN_FIELDS(State)
        PTX_FIELD(State, name, "Name", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(State)
        PTX_METHOD_AUTO(State, GetName, "Get name"),
        PTX_METHOD_AUTO(State, Enter, "Enter"),
        PTX_METHOD_AUTO(State, Update, "Update"),
        PTX_METHOD_AUTO(State, Exit, "Exit")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(State)
        PTX_CTOR(State, std::string)
    PTX_END_DESCRIBE(State)
};

/**
 * @class StateMachine
 * @brief Finite state machine for managing AI states and transitions.
 */
class StateMachine {
private:
    std::unordered_map<std::string, std::shared_ptr<State>> states;
    std::shared_ptr<State> currentState;
    std::string initialStateName;

public:
    /**
     * @brief Constructor.
     */
    StateMachine();

    /**
     * @brief Destructor.
     */
    ~StateMachine();

    // === State Management ===

    /**
     * @brief Adds a state to the machine.
     */
    std::shared_ptr<State> AddState(const std::string& name);

    /**
     * @brief Gets a state by name.
     */
    std::shared_ptr<State> GetState(const std::string& name);

    /**
     * @brief Removes a state.
     */
    void RemoveState(const std::string& name);

    /**
     * @brief Gets the current state.
     */
    std::shared_ptr<State> GetCurrentState() const { return currentState; }

    /**
     * @brief Gets the current state name.
     */
    std::string GetCurrentStateName() const;

    // === State Transitions ===

    /**
     * @brief Sets the initial state.
     */
    void SetInitialState(const std::string& stateName);

    /**
     * @brief Transitions to a new state immediately.
     */
    void TransitionTo(const std::string& stateName);

    /**
     * @brief Starts the state machine (enters initial state).
     */
    void Start();

    /**
     * @brief Stops the state machine (exits current state).
     */
    void Stop();

    // === Update ===

    /**
     * @brief Updates the current state and checks for transitions.
     */
    void Update(float deltaTime);

    PTX_BEGIN_FIELDS(StateMachine)
        PTX_FIELD(StateMachine, initialStateName, "Initial state", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(StateMachine)
        PTX_METHOD_AUTO(StateMachine, GetCurrentStateName, "Get current state name"),
        PTX_METHOD_AUTO(StateMachine, SetInitialState, "Set initial state"),
        PTX_METHOD_AUTO(StateMachine, TransitionTo, "Transition to"),
        PTX_METHOD_AUTO(StateMachine, Start, "Start"),
        PTX_METHOD_AUTO(StateMachine, Stop, "Stop"),
        PTX_METHOD_AUTO(StateMachine, Update, "Update")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(StateMachine)
        PTX_CTOR0(StateMachine)
    PTX_END_DESCRIBE(StateMachine)
};

} // namespace ptx
