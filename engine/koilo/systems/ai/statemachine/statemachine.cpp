#include <ptx/systems/ai/statemachine/statemachine.hpp>

namespace ptx {

// === State ===

State::State(const std::string& name)
    : name(name) {
}

void State::AddTransition(const std::string& targetState, StateTransitionCondition condition) {
    transitions.emplace_back(targetState, condition);
}

void State::Enter() {
    if (onEnter) {
        onEnter();
    }
}

void State::Update(float deltaTime) {
    if (onUpdate) {
        onUpdate(deltaTime);
    }
}

void State::Exit() {
    if (onExit) {
        onExit();
    }
}

std::string State::CheckTransitions() {
    for (const auto& transition : transitions) {
        if (transition.condition && transition.condition()) {
            return transition.targetStateName;
        }
    }
    return "";  // No transition
}

// === StateMachine ===

StateMachine::StateMachine()
    : currentState(nullptr) {
}

StateMachine::~StateMachine() {
    Stop();
}

std::shared_ptr<State> StateMachine::AddState(const std::string& name) {
    auto state = std::make_shared<State>(name);
    states[name] = state;
    return state;
}

std::shared_ptr<State> StateMachine::GetState(const std::string& name) {
    auto it = states.find(name);
    if (it != states.end()) {
        return it->second;
    }
    return nullptr;
}

void StateMachine::RemoveState(const std::string& name) {
    // Exit state if it's current
    if (currentState && currentState->GetName() == name) {
        currentState->Exit();
        currentState = nullptr;
    }

    states.erase(name);
}

std::string StateMachine::GetCurrentStateName() const {
    if (currentState) {
        return currentState->GetName();
    }
    return "";
}

void StateMachine::SetInitialState(const std::string& stateName) {
    initialStateName = stateName;
}

void StateMachine::TransitionTo(const std::string& stateName) {
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

void StateMachine::Start() {
    if (!initialStateName.empty()) {
        TransitionTo(initialStateName);
    }
}

void StateMachine::Stop() {
    if (currentState) {
        currentState->Exit();
        currentState = nullptr;
    }
}

void StateMachine::Update(float deltaTime) {
    if (!currentState) {
        return;
    }

    // Update current state
    currentState->Update(deltaTime);

    // Check for transitions
    std::string nextStateName = currentState->CheckTransitions();
    if (!nextStateName.empty()) {
        TransitionTo(nextStateName);
    }
}

} // namespace ptx
