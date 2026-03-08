// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file helpers.hpp
 * @brief Shared test helpers for KoiloScript language integration tests.
 *
 * Provides StringFileReader, MultiFileReader, RunScript utilities,
 * and the TEST/PASS/FAIL macros used across all language test files.
 */

#pragma once

#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/systems/scene/animation/animationclip.hpp>
#include <koilo/systems/scene/animation/animationmixer.hpp>
#include <koilo/assets/image/texture.hpp>
#include <koilo/systems/scene/primitivemesh.hpp>
#include <koilo/systems/scene/sprite.hpp>
#include <koilo/systems/physics/rigidbody.hpp>
#include <koilo/systems/physics/physicsworld.hpp>
#include <koilo/systems/ui/widget.hpp>
#include <koilo/systems/ui/ui.hpp>
#include <koilo/systems/physics/capsulecollider.hpp>
#include <koilo/systems/physics/spherecollider.hpp>
#include <koilo/systems/physics/boxcollider.hpp>
#include <koilo/systems/render/raster/rasterizer.hpp>
#include <koilo/core/geometry/3d/aabb.hpp>
#include <koilo/core/geometry/3d/plane.hpp>
#include <koilo/core/geometry/ray.hpp>
#include <koilo/core/geometry/2d/aabb2d.hpp>
#include <koilo/core/geometry/2d/rectangle.hpp>
#include <koilo/core/geometry/2d/circle.hpp>
#include <koilo/debug/debugdraw.hpp>
#include <koilo/debug/debugrenderer.hpp>
#include <koilo/ecs/script_entity_manager.hpp>
#include <koilo/ecs/entitymanager.hpp>
#include <koilo/systems/ai/script_ai_manager.hpp>
#include <koilo/systems/scene/animation/skeleton.hpp>
#include <koilo/systems/scene/animation/skeleton_animator.hpp>
#include <koilo/systems/ai/behaviortree.hpp>
#include <koilo/systems/ai/behaviortree/behaviortreenode.hpp>
#include <koilo/systems/ai/behaviortree/behaviortreeaction.hpp>
#include <koilo/systems/ai/statemachine/statemachine.hpp>
#include <koilo/systems/ai/pathfinding/pathfinder.hpp>
#include <koilo/systems/audio/script_audio_manager.hpp>
#include <koilo/systems/audio/audioclip.hpp>
#include <koilo/systems/audio/audiosource.hpp>
#include <koilo/systems/audio/audiomanager.hpp>
#include <koilo/core/time/timemanager.hpp>
#include <koilo/systems/world/reflectionserializer.hpp>
#include <koilo/systems/world/levelserializer.hpp>
#include <koilo/systems/world/worldmanager.hpp>
#include <koilo/systems/world/script_world_manager.hpp>
#include <koilo/ecs/components/tagcomponent.hpp>
#include <koilo/ecs/components/transformcomponent.hpp>
#include <koilo/ecs/components/velocitycomponent.hpp>
#include <iostream>
#include <cassert>
#include <cmath>
#include <chrono>
#include <map>
#include <string>

using namespace koilo::scripting;
using koilo::AABB2D;
using koilo::Circle2D;
using koilo::Rectangle2D;
using koilo::Vector2D;
using koilo::Color888;

// ---------------------------------------------------------------------------
// File readers
// ---------------------------------------------------------------------------

class StringFileReader : public koilo::platform::IScriptFileReader {
    std::string content_;
    std::string error_;
public:
    void SetContent(const std::string& c) { content_ = c; }
    bool Read(const char*, std::string& out) override { out = content_; return true; }
    bool Exists(const char*) override { return true; }
    size_t GetFileSize(const char*) override { return content_.size(); }
    const char* GetLastError() const override { return error_.c_str(); }
};

class MultiFileReader : public koilo::platform::IScriptFileReader {
    std::map<std::string, std::string> files_;
    std::string error_;
public:
    void AddFile(const std::string& path, const std::string& content) {
        files_[path] = content;
    }
    bool Read(const char* filepath, std::string& out) override {
        auto it = files_.find(filepath);
        if (it != files_.end()) { out = it->second; return true; }
        error_ = std::string("File not found: ") + filepath;
        return false;
    }
    bool Exists(const char* filepath) override { return files_.count(filepath) > 0; }
    size_t GetFileSize(const char* filepath) override {
        auto it = files_.find(filepath);
        return it != files_.end() ? it->second.size() : 0;
    }
    const char* GetLastError() const override { return error_.c_str(); }
};

// ---------------------------------------------------------------------------
// Test counters (defined in runner.cpp, declared extern here)
// ---------------------------------------------------------------------------

extern int testsPassed;
extern int testsFailed;

// ---------------------------------------------------------------------------
// Test macros
// ---------------------------------------------------------------------------

#define TEST(name) std::cout << "  " << name << "... ";
#define PASS() { std::cout << "PASS" << std::endl; testsPassed++; }
#define FAIL(msg) { std::cout << "FAIL: " << msg << std::endl; testsFailed++; }

// ---------------------------------------------------------------------------
// Script execution helpers
// ---------------------------------------------------------------------------

inline bool RunScript(const std::string& script, StringFileReader& reader, KoiloScriptEngine& engine) {
    reader.SetContent(script);
    if (!engine.LoadScript("test.ks")) {
        FAIL(engine.GetError());
        return false;
    }
    if (!engine.BuildScene()) {
        FAIL("BuildScene failed");
        return false;
    }
    return true;
}

inline std::string WrapInSetup(const std::string& body) {
    return "fn Setup() {\n" + body + "\n}\n";
}

inline std::string WrapWithFunctions(const std::string& funcs, const std::string& body) {
    return funcs + "\n" + WrapInSetup(body);
}
