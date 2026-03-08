// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/ai/statemachine/statemachine.hpp>
#include <koilo/core/time/timemanager.hpp>

namespace koilo {

// === State ===

koilo::State::State(const std::string& name)
    : name(name) {
}

void koilo::State::AddTransition(const std::string& targetState, StateTransitionCondition condition) {
    transitions.emplace_back(targetState, condition);
}

void koilo::State::Enter() {
    if (onEnter) {
        onEnter();
    }
}

void koilo::State::Update() {
    if (onUpdate) {
        float deltaTime = TimeManager::GetInstance().GetDeltaTime();
        onUpdate(deltaTime);
    }
}

void koilo::State::Exit() {
    if (onExit) {
        onExit();
    }
}

std::string koilo::State::CheckTransitions() {
    for (const auto& transition : transitions) {
        if (transition.condition && transition.condition()) {
            return transition.targetStateName;
        }
    }
    return "";  // No transition
}

// === StateMachine ===

koilo::StateMachine::StateMachine()
    : currentState(nullptr) {
}

koilo::StateMachine::~StateMachine() {
    Stop();
}

std::shared_ptr<State> koilo::StateMachine::AddState(const std::string& name) {
    auto state = std::make_shared<State>(name);
    states[name] = state;
    return state;
}

std::shared_ptr<State> koilo::StateMachine::GetState(const std::string& name) {
    auto it = states.find(name);
    if (it != states.end()) {
        return it->second;
    }
    return nullptr;
}

void koilo::StateMachine::RemoveState(const std::string& name) {
    // Exit state if it's current
    if (currentState && currentState->GetName() == name) {
        currentState->Exit();
        currentState = nullptr;
    }

    states.erase(name);
}

std::string koilo::StateMachine::GetCurrentStateName() const {
    if (currentState) {
        return currentState->GetName();
    }
    return "";
}

void koilo::StateMachine::SetInitialState(const std::string& stateName) {
    initialStateName = stateName;
}

void koilo::StateMachine::TransitionTo(const std::string& stateName) {
    auto nextState = GetState(stateName);
    if (!nextState) {
        return;  // State doesn't exist
    }

    // Exit current state
    if (currentState) {
        currentState->Exit();
    }

    // Enter new state
    currentState = nextState;
    currentState->Enter();
}

void koilo::StateMachine::Start() {
    if (!initialStateName.empty()) {
        TransitionTo(initialStateName);
    }
}

void koilo::StateMachine::Stop() {
    if (currentState) {
        currentState->Exit();
        currentState = nullptr;
    }
}

void koilo::StateMachine::Update() {
    if (!currentState) {
        return;
    }

    // Update current state
    currentState->Update();

    // Check for transitions
    std::string nextStateName = currentState->CheckTransitions();
    if (!nextStateName.empty()) {
        TransitionTo(nextStateName);
    }
}

} // namespace koilo
