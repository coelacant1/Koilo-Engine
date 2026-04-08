// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file runner.cpp
 * @brief Entry point for KoiloScript language integration tests.
 *
 * Calls all feature-grouped test functions and reports pass/fail totals.
 * Run via: koilo_script_language_tests
 */

#include "helpers.hpp"
#include <iostream>

// ---------------------------------------------------------------------------
// Test counters (extern'd in helpers.hpp)
// ---------------------------------------------------------------------------
int testsPassed = 0;
int testsFailed  = 0;

// ---------------------------------------------------------------------------
// Forward declarations  implemented in each feature file
// ---------------------------------------------------------------------------

// test_language_core.cpp
void TestVarDeclarations();
void TestWhileLoops();
void TestForLoops();
void TestBreakContinue();
void TestFunctions();
void TestLogicalOps();
void TestTernary();
void TestTypeUtils();
void TestChainedCalls();
void TestConstructorExpressions();
void TestTempCleanup();
void TestPersistentObjects();

// test_language_collections.cpp
void TestStringOps();
void TestBuiltins();
void TestTableOps();
void TestArrayOps();
void TestColorBuiltins();
void TestPhase24Builtins();

// test_language_classes.cpp
void TestCodeFirstSyntax();
void TestPhase31ScriptClasses();
void TestPhase24StaticMethods();
void TestPhase24RefParams();
void TestPhase24TemplateRefactor();
void TestPhase24AuditFixes();

// test_language_advanced.cpp
void TestPhase24Enums();
void TestPhase9Hardening();
void TestPhase10Performance();

// test_language_imports.cpp
void TestPhase32Imports();

// test_language_signals.cpp
void TestPhase33Signals();

// test_language_operators.cpp
void TestPhase40OperatorOverloading();
void TestPhase40Coroutines();
void TestPhase43DebugErrorHardening();
void TestPhase41Validation();

// test_scripting_animation.cpp
void TestAnimationClasses();
void TestAnimationClipChannel();
void TestAnimationMixer();
void TestPhase35Skeleton();
void TestPhase24EasyEaseAnimator();

// test_scripting_scene.cpp
void TestMaterialAnimator();
void TestPrimitiveMesh();
void TestSceneNode();
void TestMultiContext();
void TestTexture();
void TestTexturedQuad();
void TestSprite();
void TestPhase11Rendering();
void TestPhase13BlenderParity();

// test_scripting_physics.cpp
void TestRigidBody();
void TestPhysicsWorld();
void TestCapsuleCollisions();
void TestCollisionCallbacks();
void TestPhysicsScriptGlobal();
void TestPhase13_2DGeometry();

// test_scripting_ui.cpp
void TestInputManager();
void TestWidgetAndUI();
void TestUIScriptIntegration();

// test_scripting_rendering.cpp
void TestPhase12DebugTools();
void TestPhase12ADebugAdditions();
void TestPhase35Profiler();

// test_scripting_systems.cpp
void TestParticleSystem();
#ifdef KOILO_ENABLE_AUDIO
void TestPhase19AudioSystem();
#endif

// test_scripting_ecs_world.cpp
void TestPhase17ECSIntegration();
void TestPhase18AISystem();
void TestPhase20WorldSerialization();

// test_scripting_modules.cpp
void TestPhase36Modules();
void TestPhase37ModuleSDK();

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    std::cout << "=== KoiloScript Language Feature Tests ===" << std::endl << std::endl;

    // --- Core language ---
    TestVarDeclarations();
    TestWhileLoops();
    TestForLoops();
    TestBreakContinue();
    TestFunctions();
    TestLogicalOps();
    TestTernary();
    TestTypeUtils();
    TestChainedCalls();
    TestConstructorExpressions();
    TestTempCleanup();
    TestPersistentObjects();

    // --- Collections & builtins ---
    TestStringOps();
    TestBuiltins();
    TestTableOps();
    TestArrayOps();
    TestColorBuiltins();
    TestPhase24Builtins();

    // --- Classes & OOP ---
    TestCodeFirstSyntax();
    TestPhase31ScriptClasses();
    TestPhase24StaticMethods();
    TestPhase24RefParams();
    TestPhase24TemplateRefactor();
    TestPhase24AuditFixes();

    // --- Advanced language features ---
    TestPhase24Enums();
    TestPhase9Hardening();
    TestPhase10Performance();

    // --- Imports, signals, operators, coroutines ---
    TestPhase32Imports();
    TestPhase33Signals();
    TestPhase40OperatorOverloading();
    TestPhase40Coroutines();
    TestPhase43DebugErrorHardening();
    TestPhase41Validation();

    // --- Scripting bindings: animation ---
    TestAnimationClasses();
    TestAnimationClipChannel();
    TestAnimationMixer();
    TestPhase35Skeleton();
    TestPhase24EasyEaseAnimator();

    // --- Scripting bindings: scene & rendering ---
    TestMaterialAnimator();
    TestPrimitiveMesh();
    TestSceneNode();
    TestMultiContext();
    TestTexture();
    TestTexturedQuad();
    TestSprite();
    TestPhase11Rendering();
    TestPhase13BlenderParity();

    // --- Scripting bindings: physics & geometry ---
    TestRigidBody();
    TestPhysicsWorld();
    TestCapsuleCollisions();
    TestCollisionCallbacks();
    TestPhysicsScriptGlobal();
    TestPhase13_2DGeometry();

    // --- Scripting bindings: input & UI ---
    TestInputManager();
    TestWidgetAndUI();
    TestUIScriptIntegration();

    // --- Scripting bindings: rendering effects & debug ---
    TestPhase12DebugTools();
    TestPhase12ADebugAdditions();
    TestPhase35Profiler();
    extern void TestDrawBuffer2DBuiltins();
    TestDrawBuffer2DBuiltins();

    // --- Scripting bindings: particles & audio ---
    TestParticleSystem();
#ifdef KOILO_ENABLE_AUDIO
    TestPhase19AudioSystem();
#endif

    // --- Scripting bindings: ECS, AI, world ---
    TestPhase17ECSIntegration();
    TestPhase18AISystem();
    TestPhase20WorldSerialization();

    // --- Scripting bindings: modules ---
    TestPhase36Modules();
    TestPhase37ModuleSDK();

    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    std::cout << "Passed: " << testsPassed << std::endl;
    std::cout << "Failed: " << testsFailed << std::endl;

    return testsFailed > 0 ? 1 : 0;
}
