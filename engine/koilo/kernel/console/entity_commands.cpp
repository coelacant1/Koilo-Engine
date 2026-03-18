// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file entity_commands.cpp
 * @brief Console commands for runtime entity/scene inspection (Phase 15).
 *
 * Provides entity.list, entity.inspect, entity.set, entity.watch/unwatch,
 * scene.hierarchy, and help --json commands for runtime introspection.
 *
 * @date 03/18/2026
 * @author Coela Can't
 */

#include "console_commands.hpp"
#include "event_bridge.hpp"
#include "../debug_overlay.hpp"
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/config/config_store.hpp>
#include <koilo/systems/ecs/script_entity_manager.hpp>
#include <koilo/systems/ecs/entitymanager.hpp>
#include <koilo/systems/scene/scene.hpp>
#include <koilo/systems/scene/scenenode.hpp>
#include <koilo/systems/scene/camera/camerabase.hpp>
#include <koilo/core/math/transform.hpp>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <dirent.h>
#include <cstdio>

namespace koilo {

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------

/**
 * @brief Format a Vector3D as a compact string.
 * @param[in] v Vector to format.
 * @return Formatted string "(x, y, z)" with 2 decimal places.
 */
static std::string FormatVec(const Vector3D& v) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2)
       << "(" << v.X << ", " << v.Y << ", " << v.Z << ")";
    return ss.str();
}

/**
 * @brief Get ScriptEntityManager from kernel services.
 * @param[in] kernel Reference to the engine kernel.
 * @return Pointer to ScriptEntityManager, or nullptr if not available.
 */
static ScriptEntityManager* GetEntities(KoiloKernel& kernel) {
    return kernel.Services().Get<ScriptEntityManager>("entities");
}

/**
 * @brief Get Scene from kernel services.
 * @param[in] kernel Reference to the engine kernel.
 * @return Pointer to Scene, or nullptr if not available.
 */
static Scene* GetScene(KoiloKernel& kernel) {
    return kernel.Services().Get<Scene>("scene");
}

/**
 * @brief Recursively build a scene hierarchy string.
 * @param[in] node Current node in the traversal.
 * @param[in] depth Indentation depth.
 * @param[out] ss Output stream to append to.
 * @param[out] jsonSs JSON output stream to append to.
 * @param[out] count Running count of nodes visited.
 */
static void WalkHierarchy(SceneNode* node, int depth,
                          std::ostringstream& ss,
                          std::ostringstream& jsonSs,
                          int& count) {
    if (!node) return;

    std::string indent(depth * 2, ' ');
    const auto& wt = const_cast<SceneNode*>(node)->GetWorldTransform();
    Mesh* mesh = node->GetMesh();

    ss << indent << node->GetName();
    if (mesh) ss << " [mesh]";
    ss << "  pos=" << FormatVec(wt.GetPosition()) << "\n";

    if (count > 0) jsonSs << ",";
    jsonSs << "{\"name\":\"" << node->GetName() << "\""
           << ",\"depth\":" << depth
           << ",\"hasMesh\":" << (mesh ? "true" : "false")
           << ",\"position\":[" << wt.GetPosition().X << ","
           << wt.GetPosition().Y << "," << wt.GetPosition().Z << "]"
           << ",\"children\":" << node->GetChildCount()
           << "}";

    ++count;

    for (auto* child : node->GetChildren()) {
        WalkHierarchy(child, depth + 1, ss, jsonSs, count);
    }
}

// -----------------------------------------------------------------------
// Registration
// -----------------------------------------------------------------------

void RegisterEntityCommands(CommandRegistry& registry) {

    // ---------------------------------------------------------------
    // entity.list [filter] - list all entities with optional tag filter
    // ---------------------------------------------------------------
    registry.Register({"entity.list", "entity.list [tag_filter]",
        "List all entities (optional filter by tag substring)",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            auto* entities = GetEntities(kernel);
            if (!entities)
                return ConsoleResult::Error("Entity system not available (no scene loaded)");

            std::string filter = args.empty() ? "" : args[0];

            std::ostringstream ss;
            std::ostringstream jsonSs;
            jsonSs << "[";
            int count = 0;
            int total = entities->GetCount();

            auto allEntities = entities->GetAllEntities();
            for (auto& ent : allEntities) {
                std::string tag = entities->GetTag(ent);
                Vector3D pos = entities->GetPosition(ent);

                // Apply filter
                if (!filter.empty() && tag.find(filter) == std::string::npos)
                    continue;

                EntityID id = ent.GetID();
                ss << "  #" << std::setw(4) << id << "  "
                   << std::setw(16) << std::left << (tag.empty() ? "(untagged)" : tag)
                   << "  pos=" << FormatVec(pos) << "\n";

                if (count > 0) jsonSs << ",";
                jsonSs << "{\"id\":" << id
                       << ",\"tag\":\"" << tag << "\""
                       << ",\"position\":[" << pos.X << "," << pos.Y << "," << pos.Z << "]}";
                ++count;
            }
            jsonSs << "]";

            std::ostringstream header;
            header << "Entities";
            if (!filter.empty()) header << " matching '" << filter << "'";
            header << " (" << count << "/" << total << "):\n";

            return ConsoleResult::Ok(header.str() + ss.str(), jsonSs.str());
        }, nullptr
    });

    // ---------------------------------------------------------------
    // entity.inspect <id|tag> - detailed view of an entity
    // ---------------------------------------------------------------
    registry.Register({"entity.inspect", "entity.inspect <id|tag>",
        "Inspect an entity's components and fields",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty())
                return ConsoleResult::Error("Usage: entity.inspect <entity_id | tag_name>");

            auto* entities = GetEntities(kernel);
            if (!entities)
                return ConsoleResult::Error("Entity system not available");

            // Resolve entity: try numeric ID first, then tag lookup
            Entity ent{0};
            bool found = false;
            try {
                EntityID id = static_cast<EntityID>(std::stoul(args[0]));
                ent = Entity{id};
                if (entities->GetManager().IsEntityValid(ent))
                    found = true;
            } catch (...) {}

            if (!found) {
                ent = entities->FindByTag(args[0]);
                if (entities->GetManager().IsEntityValid(ent))
                    found = true;
            }

            if (!found)
                return ConsoleResult::Error("Entity not found: " + args[0]);

            EntityID id = ent.GetID();
            std::string tag = entities->GetTag(ent);
            Vector3D pos = entities->GetPosition(ent);
            Quaternion rot = entities->GetRotation(ent);
            Vector3D scale = entities->GetScale(ent);
            Vector3D vel = entities->GetVelocity(ent);
            SceneNode* node = entities->GetNode(ent);

            std::ostringstream ss;
            ss << "Entity #" << id;
            if (!tag.empty()) ss << " \"" << tag << "\"";
            ss << "\n";
            ss << "  Transform:\n"
               << "    position: " << FormatVec(pos) << "\n"
               << "    rotation: (" << std::fixed << std::setprecision(3)
               << rot.W << ", " << rot.X << ", " << rot.Y << ", " << rot.Z << ")\n"
               << "    scale:    " << FormatVec(scale) << "\n";
            ss << "  Velocity:\n"
               << "    linear:   " << FormatVec(vel) << "\n";
            if (node) {
                ss << "  SceneNode: \"" << node->GetName() << "\""
                   << " (children: " << node->GetChildCount() << ")\n";
                if (node->GetMesh()) ss << "    mesh: attached\n";
            } else {
                ss << "  SceneNode: (none)\n";
            }

            // JSON
            std::ostringstream jsonSs;
            jsonSs << "{\"id\":" << id
                   << ",\"tag\":\"" << tag << "\""
                   << ",\"transform\":{\"position\":[" << pos.X << "," << pos.Y << "," << pos.Z << "]"
                   << ",\"rotation\":[" << rot.W << "," << rot.X << "," << rot.Y << "," << rot.Z << "]"
                   << ",\"scale\":[" << scale.X << "," << scale.Y << "," << scale.Z << "]}"
                   << ",\"velocity\":[" << vel.X << "," << vel.Y << "," << vel.Z << "]"
                   << ",\"sceneNode\":" << (node ? ("\"" + node->GetName() + "\"") : "null")
                   << "}";

            return ConsoleResult::Ok(ss.str(), jsonSs.str());
        }, nullptr
    });

    // ---------------------------------------------------------------
    // entity.set <id>.<field> <value> - set entity property
    // ---------------------------------------------------------------
    registry.Register({"entity.set", "entity.set <id>.<field> <value>",
        "Set an entity property (pos.X, pos.Y, pos.Z, tag, scale.X, etc.)",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.size() < 2)
                return ConsoleResult::Error("Usage: entity.set <id>.<field> <value>");

            auto* entities = GetEntities(kernel);
            if (!entities)
                return ConsoleResult::Error("Entity system not available");

            // Parse path: "42.pos.X" → id=42, field="pos.X"
            std::string path = args[0];
            auto dotPos = path.find('.');
            if (dotPos == std::string::npos)
                return ConsoleResult::Error("Expected format: <id>.<field> (e.g., 42.pos.X)");

            std::string idStr = path.substr(0, dotPos);
            std::string field = path.substr(dotPos + 1);

            EntityID id;
            try {
                id = static_cast<EntityID>(std::stoul(idStr));
            } catch (...) {
                return ConsoleResult::Error("Invalid entity ID: " + idStr);
            }

            Entity ent{id};
            if (!entities->GetManager().IsEntityValid(ent))
                return ConsoleResult::Error("Entity " + idStr + " does not exist");

            std::string value = args[1];
            // For vector fields, join remaining args
            if (args.size() > 2) {
                for (size_t i = 2; i < args.size(); ++i)
                    value += " " + args[i];
            }

            // Parse value helper
            auto parseFloat = [&](const std::string& s) -> float {
                return std::stof(s);
            };

            auto parseVec3 = [&](const std::string& s) -> Vector3D {
                // Accept "x,y,z" or "x y z"
                std::string clean = s;
                std::replace(clean.begin(), clean.end(), ',', ' ');
                std::istringstream iss(clean);
                float x = 0, y = 0, z = 0;
                iss >> x >> y >> z;
                return Vector3D(x, y, z);
            };

            try {
                if (field == "tag") {
                    std::string old = entities->GetTag(ent);
                    entities->SetTag(ent, value);
                    return ConsoleResult::Ok("tag: \"" + old + "\" -> \"" + value + "\"");
                }

                if (field == "pos" || field == "position") {
                    Vector3D old = entities->GetPosition(ent);
                    Vector3D v = parseVec3(value);
                    entities->SetPosition(ent, v);
                    return ConsoleResult::Ok("position: " + FormatVec(old) + " -> " + FormatVec(v));
                }
                if (field == "pos.X") {
                    Vector3D p = entities->GetPosition(ent); float old = p.X;
                    p.X = parseFloat(value); entities->SetPosition(ent, p);
                    return ConsoleResult::Ok("pos.X: " + std::to_string(old) + " -> " + value);
                }
                if (field == "pos.Y") {
                    Vector3D p = entities->GetPosition(ent); float old = p.Y;
                    p.Y = parseFloat(value); entities->SetPosition(ent, p);
                    return ConsoleResult::Ok("pos.Y: " + std::to_string(old) + " -> " + value);
                }
                if (field == "pos.Z") {
                    Vector3D p = entities->GetPosition(ent); float old = p.Z;
                    p.Z = parseFloat(value); entities->SetPosition(ent, p);
                    return ConsoleResult::Ok("pos.Z: " + std::to_string(old) + " -> " + value);
                }

                if (field == "scale") {
                    Vector3D old = entities->GetScale(ent);
                    Vector3D v = parseVec3(value);
                    entities->SetScale(ent, v);
                    return ConsoleResult::Ok("scale: " + FormatVec(old) + " -> " + FormatVec(v));
                }
                if (field == "scale.X") {
                    Vector3D s = entities->GetScale(ent); float old = s.X;
                    s.X = parseFloat(value); entities->SetScale(ent, s);
                    return ConsoleResult::Ok("scale.X: " + std::to_string(old) + " -> " + value);
                }
                if (field == "scale.Y") {
                    Vector3D s = entities->GetScale(ent); float old = s.Y;
                    s.Y = parseFloat(value); entities->SetScale(ent, s);
                    return ConsoleResult::Ok("scale.Y: " + std::to_string(old) + " -> " + value);
                }
                if (field == "scale.Z") {
                    Vector3D s = entities->GetScale(ent); float old = s.Z;
                    s.Z = parseFloat(value); entities->SetScale(ent, s);
                    return ConsoleResult::Ok("scale.Z: " + std::to_string(old) + " -> " + value);
                }

                if (field == "vel" || field == "velocity") {
                    Vector3D old = entities->GetVelocity(ent);
                    Vector3D v = parseVec3(value);
                    entities->SetVelocity(ent, v);
                    return ConsoleResult::Ok("velocity: " + FormatVec(old) + " -> " + FormatVec(v));
                }

                return ConsoleResult::Error(
                    "Unknown field: " + field +
                    "\nAvailable: tag, pos, pos.X/y/z, scale, scale.X/y/z, vel");

            } catch (const std::exception& e) {
                return ConsoleResult::Error("Failed to set " + field + ": " + e.what());
            }
        }, nullptr
    });

    // ---------------------------------------------------------------
    // entity.watch <id>.<field> - pin entity field to debug overlay
    // ---------------------------------------------------------------
    registry.Register({"entity.watch", "entity.watch <id>.<field>",
        "Pin an entity field to the debug overlay",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty())
                return ConsoleResult::Error("Usage: entity.watch <id>.<field>");

            if (!g_debugOverlay)
                return ConsoleResult::Error("Debug overlay not available");

            auto* entities = GetEntities(kernel);
            if (!entities)
                return ConsoleResult::Error("Entity system not available");

            std::string path = args[0];
            auto dotPos = path.find('.');
            if (dotPos == std::string::npos)
                return ConsoleResult::Error("Expected format: <id>.<field>");

            std::string idStr = path.substr(0, dotPos);
            std::string field = path.substr(dotPos + 1);

            EntityID id;
            try {
                id = static_cast<EntityID>(std::stoul(idStr));
            } catch (...) {
                return ConsoleResult::Error("Invalid entity ID: " + idStr);
            }

            Entity ent{id};
            if (!entities->GetManager().IsEntityValid(ent))
                return ConsoleResult::Error("Entity " + idStr + " does not exist");

            std::string watchName = "entity." + path;

            // Capture entities pointer and entity for the getter lambda
            if (field == "pos" || field == "position") {
                g_debugOverlay->Add(watchName, [entities, ent]() {
                    return FormatVec(entities->GetPosition(ent));
                });
            } else if (field == "tag") {
                g_debugOverlay->Add(watchName, [entities, ent]() {
                    return entities->GetTag(ent);
                });
            } else if (field == "scale") {
                g_debugOverlay->Add(watchName, [entities, ent]() {
                    return FormatVec(entities->GetScale(ent));
                });
            } else if (field == "vel" || field == "velocity") {
                g_debugOverlay->Add(watchName, [entities, ent]() {
                    return FormatVec(entities->GetVelocity(ent));
                });
            } else {
                return ConsoleResult::Error(
                    "Unknown field: " + field +
                    "\nWatchable: pos, tag, scale, vel");
            }

            return ConsoleResult::Ok("Watching: " + watchName);
        }, nullptr
    });

    // ---------------------------------------------------------------
    // entity.unwatch <name> - remove entity watch from overlay
    // ---------------------------------------------------------------
    registry.Register({"entity.unwatch", "entity.unwatch <name>",
        "Remove an entity watch from the debug overlay",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty())
                return ConsoleResult::Error("Usage: entity.unwatch <name>");

            if (!g_debugOverlay)
                return ConsoleResult::Error("Debug overlay not available");

            std::string name = args[0];
            // Add entity. prefix if not present
            if (name.find("entity.") != 0)
                name = "entity." + name;

            if (g_debugOverlay->Remove(name))
                return ConsoleResult::Ok("Removed watch: " + name);
            return ConsoleResult::Error("Watch not found: " + name);
        }, nullptr
    });

    // ---------------------------------------------------------------
    // scene.hierarchy - display scene graph tree
    // ---------------------------------------------------------------
    registry.Register({"scene.hierarchy", "scene.hierarchy",
        "Display the scene graph node hierarchy",
        [](KoiloKernel& kernel, const std::vector<std::string>&) -> ConsoleResult {
            auto* scene = GetScene(kernel);
            if (!scene)
                return ConsoleResult::Error("Scene not available");

            std::ostringstream ss;
            std::ostringstream jsonSs;
            jsonSs << "[";
            int count = 0;

            // Walk all owned nodes - find root nodes (no parent)
            size_t nodeCount = scene->GetNodeCount();
            if (nodeCount == 0)
                return ConsoleResult::Ok("Scene is empty (0 nodes)");

            // Iterate scene nodes by trying names from the internal map
            // Scene exposes Find() and GetNodeCount(), and CreateObject()
            // We need to walk the owned nodes - they're accessible via the
            // flat mesh list and node lookup

            // Since Scene doesn't expose direct node iteration, we use
            // the mesh list + node lookup as a proxy. But actually, we
            // need the internal ownedNodes_. Let's use the scene's flat
            // mesh list to find node names, then traverse from there.

            // Better approach: use ScriptEntityManager to find all entities
            // with scene nodes, then show their hierarchy
            auto* entities = GetEntities(kernel);
            if (entities) {
                int maxId = entities->GetCount() + 100;
                for (int i = 0; i < maxId; ++i) {
                    Entity ent{static_cast<EntityID>(i)};
                    if (!entities->GetManager().IsEntityValid(ent)) continue;

                    SceneNode* node = entities->GetNode(ent);
                    if (!node) continue;
                    // Only process root nodes (no parent or parent not entity-owned)
                    if (node->GetParent()) continue;

                    WalkHierarchy(node, 0, ss, jsonSs, count);
                }
            }

            // Also show nodes found via Scene directly
            // If scene exposes mesh-attached nodes we haven't seen
            for (unsigned int m = 0; m < scene->GetMeshCount(); ++m) {
                Mesh* mesh = scene->GetMesh(m);
                if (!mesh) continue;
                // Meshes in the scene flat list but we've already shown
                // entity-attached nodes. This catches orphan meshes.
            }

            jsonSs << "]";

            std::ostringstream header;
            header << "Scene hierarchy (" << count << " nodes, "
                   << scene->GetMeshCount() << " meshes, "
                   << scene->GetTotalTriangleCount() << " triangles):\n";

            return ConsoleResult::Ok(header.str() + ss.str(), jsonSs.str());
        }, nullptr
    });

    // ---------------------------------------------------------------
    // subscribe <event,...> - subscribe TCP client to MessageBus events
    // ---------------------------------------------------------------
    registry.Register({"subscribe", "subscribe <event,...>",
        "Subscribe to MessageBus events (TCP clients only)",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty()) {
                std::string names;
                for (auto& n : AllEventNames()) names += "  " + n + "\n";
                return ConsoleResult::Error(
                    "Usage: subscribe <event,...>\nAvailable events:\n" + names);
            }

            auto* bridge = kernel.Services().Get<EventBridge>("events");
            if (!bridge)
                return ConsoleResult::Error("Event bridge not available");

            auto token = g_eventBridgeToken;
            if (token == 0)
                return ConsoleResult::Error("Event subscription requires a TCP connection");

            // Parse comma-separated event names
            std::vector<std::string> subscribed;
            for (auto& arg : args) {
                // Split on commas
                size_t start = 0;
                while (start < arg.size()) {
                    size_t comma = arg.find(',', start);
                    std::string name = arg.substr(start,
                        comma == std::string::npos ? std::string::npos : comma - start);
                    start = (comma == std::string::npos) ? arg.size() : comma + 1;

                    MessageType type = ParseEventName(name);
                    if (type == MSG_NONE) {
                        return ConsoleResult::Error("Unknown event: " + name);
                    }
                    bridge->Subscribe(token, type);
                    subscribed.push_back(name);
                }
            }

            std::ostringstream ss;
            ss << "Subscribed to " << subscribed.size() << " event(s):";
            for (auto& s : subscribed) ss << " " << s;
            return ConsoleResult::Ok(ss.str());
        }, [](KoiloKernel&, const std::vector<std::string>&, const std::string& partial) -> std::vector<std::string> {
            std::vector<std::string> matches;
            for (auto& n : AllEventNames()) {
                if (partial.empty() || n.find(partial) == 0)
                    matches.push_back(n);
            }
            return matches;
        }
    });

    // ---------------------------------------------------------------
    // unsubscribe [event,...] - unsubscribe from events (all if no args)
    // ---------------------------------------------------------------
    registry.Register({"unsubscribe", "unsubscribe [event,...]",
        "Unsubscribe from MessageBus events (all if no args)",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            auto* bridge = kernel.Services().Get<EventBridge>("events");
            if (!bridge)
                return ConsoleResult::Error("Event bridge not available");

            auto token = g_eventBridgeToken;
            if (token == 0)
                return ConsoleResult::Error("Event subscription requires a TCP connection");

            if (args.empty()) {
                bridge->UnsubscribeAll(token);
                return ConsoleResult::Ok("Unsubscribed from all events");
            }

            int count = 0;
            for (auto& arg : args) {
                size_t start = 0;
                while (start < arg.size()) {
                    size_t comma = arg.find(',', start);
                    std::string name = arg.substr(start,
                        comma == std::string::npos ? std::string::npos : comma - start);
                    start = (comma == std::string::npos) ? arg.size() : comma + 1;

                    MessageType type = ParseEventName(name);
                    if (type == MSG_NONE)
                        return ConsoleResult::Error("Unknown event: " + name);
                    bridge->Unsubscribe(token, type);
                    ++count;
                }
            }
            return ConsoleResult::Ok("Unsubscribed from " + std::to_string(count) + " event(s)");
        }, nullptr
    });

    // ---------------------------------------------------------------
    // subscriptions - list active event subscriptions
    // ---------------------------------------------------------------
    registry.Register({"subscriptions", "subscriptions",
        "List active event subscriptions for this TCP client",
        [](KoiloKernel& kernel, const std::vector<std::string>&) -> ConsoleResult {
            auto* bridge = kernel.Services().Get<EventBridge>("events");
            if (!bridge)
                return ConsoleResult::Error("Event bridge not available");

            auto token = g_eventBridgeToken;
            if (token == 0)
                return ConsoleResult::Ok("No TCP connection - no subscriptions available");

            auto subs = bridge->GetSubscriptions(token);
            if (subs.empty())
                return ConsoleResult::Ok("No active subscriptions");

            std::ostringstream ss, jsonSs;
            ss << "Active subscriptions (" << subs.size() << "):\n";
            jsonSs << "[";
            int i = 0;
            for (auto type : subs) {
                ss << "  " << MessageTypeName(type) << " (" << type << ")\n";
                if (i > 0) jsonSs << ",";
                jsonSs << "{\"event\":\"" << MessageTypeName(type)
                       << "\",\"id\":" << type << "}";
                ++i;
            }
            jsonSs << "]";
            return ConsoleResult::Ok(ss.str(), jsonSs.str());
        }, nullptr
    });

    // ---------------------------------------------------------------
    // state.save <name> - save entity + CVar state snapshot
    // ---------------------------------------------------------------
    registry.Register({"state.save", "state.save <name>",
        "Save entity state + CVars to a named snapshot",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty())
                return ConsoleResult::Error("Usage: state.save <name>");

            std::string name = args[0];
            std::string dir = "states/" + name;

            // Create directory
            std::string mkdirCmd = "mkdir -p " + dir;
            if (system(mkdirCmd.c_str()) != 0)
                return ConsoleResult::Error("Failed to create directory: " + dir);

            // Save entities
            auto* entities = GetEntities(kernel);
            if (entities) {
                std::ostringstream jsonSs;
                jsonSs << "[\n";
                auto allEntities = entities->GetAllEntities();
                int count = 0;
                for (auto& ent : allEntities) {
                    if (count > 0) jsonSs << ",\n";
                    EntityID id = ent.GetID();
                    std::string tag = entities->GetTag(ent);
                    Vector3D pos = entities->GetPosition(ent);
                    Quaternion rot = entities->GetRotation(ent);
                    Vector3D scl = entities->GetScale(ent);
                    Vector3D vel = entities->GetVelocity(ent);

                    jsonSs << "  {\"id\":" << id
                           << ",\"tag\":\"" << tag << "\""
                           << ",\"pos\":[" << pos.X << "," << pos.Y << "," << pos.Z << "]"
                           << ",\"rot\":[" << rot.W << "," << rot.X << "," << rot.Y << "," << rot.Z << "]"
                           << ",\"scale\":[" << scl.X << "," << scl.Y << "," << scl.Z << "]"
                           << ",\"vel\":[" << vel.X << "," << vel.Y << "," << vel.Z << "]"
                           << "}";
                    ++count;
                }
                jsonSs << "\n]";

                std::string entPath = dir + "/entities.json";
                std::ofstream entFile(entPath);
                if (entFile.is_open()) {
                    entFile << jsonSs.str();
                    entFile.close();
                }
            }

            // Save CVars via ConfigStore
            auto* config = kernel.Services().Get<ConfigStore>("config");
            if (config) {
                std::string cvarPath = dir + "/cvars.cfg";
                config->SaveToFile(cvarPath.c_str());
            }

            // Save camera transform
            auto* camera = kernel.Services().Get<CameraBase>("camera");
            if (camera && camera->GetTransform()) {
                Transform* t = camera->GetTransform();
                Vector3D pos = t->GetPosition();
                Quaternion rot = t->GetRotation();

                std::ofstream camFile(dir + "/camera.json");
                if (camFile.is_open()) {
                    camFile << "{\"pos\":[" << pos.X << "," << pos.Y << "," << pos.Z << "]"
                            << ",\"rot\":[" << rot.W << "," << rot.X << "," << rot.Y << "," << rot.Z << "]"
                            << "}\n";
                    camFile.close();
                }
            }

            return ConsoleResult::Ok("State saved: " + dir);
        }, nullptr
    });

    // ---------------------------------------------------------------
    // state.load <name> - restore entity + CVar state
    // ---------------------------------------------------------------
    registry.Register({"state.load", "state.load <name>",
        "Load entity state + CVars from a named snapshot",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty())
                return ConsoleResult::Error("Usage: state.load <name>");

            std::string name = args[0];
            std::string dir = "states/" + name;

            // Check directory exists
            std::ifstream testFile(dir + "/entities.json");
            if (!testFile.is_open())
                return ConsoleResult::Error("State not found: " + dir);
            testFile.close();

            int restored = 0;

            // Restore entities (update positions of existing entities by tag match)
            auto* entities = GetEntities(kernel);
            if (entities) {
                std::ifstream entFile(dir + "/entities.json");
                if (entFile.is_open()) {
                    // Simple JSON array parser for our known format
                    std::string content((std::istreambuf_iterator<char>(entFile)),
                                        std::istreambuf_iterator<char>());
                    entFile.close();

                    // Parse each entity object - find tag, match existing entity, set transform
                    // Simple approach: find each "tag":"..." pattern and restore matching entities
                    size_t pos = 0;
                    while ((pos = content.find("\"tag\":\"", pos)) != std::string::npos) {
                        pos += 7; // skip "tag":"
                        size_t end = content.find("\"", pos);
                        if (end == std::string::npos) break;
                        std::string tag = content.substr(pos, end - pos);
                        pos = end;

                        // Parse pos array
                        size_t posStart = content.find("\"pos\":[", pos);
                        if (posStart == std::string::npos) continue;
                        posStart += 7;
                        float px = 0, py = 0, pz = 0;
                        if (sscanf(content.c_str() + posStart, "%f,%f,%f", &px, &py, &pz) == 3) {
                            Entity ent = entities->FindByTag(tag);
                            if (entities->GetManager().IsEntityValid(ent)) {
                                entities->SetPosition(ent, Vector3D(px, py, pz));
                                // Parse scale
                                size_t sclStart = content.find("\"scale\":[", posStart);
                                if (sclStart != std::string::npos) {
                                    sclStart += 9;
                                    float sx = 0, sy = 0, sz = 0;
                                    if (sscanf(content.c_str() + sclStart, "%f,%f,%f", &sx, &sy, &sz) == 3)
                                        entities->SetScale(ent, Vector3D(sx, sy, sz));
                                }
                                // Parse velocity
                                size_t velStart = content.find("\"vel\":[", posStart);
                                if (velStart != std::string::npos) {
                                    velStart += 7;
                                    float vx = 0, vy = 0, vz = 0;
                                    if (sscanf(content.c_str() + velStart, "%f,%f,%f", &vx, &vy, &vz) == 3)
                                        entities->SetVelocity(ent, Vector3D(vx, vy, vz));
                                }
                                ++restored;
                            }
                        }
                    }
                }
            }

            // Restore CVars
            auto* config = kernel.Services().Get<ConfigStore>("config");
            if (config) {
                std::string cvarPath = dir + "/cvars.cfg";
                std::ifstream cvarFile(cvarPath);
                if (cvarFile.is_open()) {
                    cvarFile.close();
                    config->LoadFromFile(cvarPath.c_str());
                }
            }

            return ConsoleResult::Ok("State loaded: " + name + " (" +
                                     std::to_string(restored) + " entities restored)");
        }, nullptr
    });

    // ---------------------------------------------------------------
    // state.list - show saved state snapshots
    // ---------------------------------------------------------------
    registry.Register({"state.list", "state.list",
        "List saved state snapshots",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            std::ostringstream ss;
            std::ostringstream jsonSs;
            jsonSs << "[";
            int count = 0;

            // Scan states/ directory
            std::string statesDir = "states";
            DIR* dir = opendir(statesDir.c_str());
            if (!dir)
                return ConsoleResult::Ok("No saved states found (states/ directory not found)");

            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                if (entry->d_name[0] == '.') continue;
                if (entry->d_type != DT_DIR) continue;

                std::string name = entry->d_name;
                std::string entPath = statesDir + "/" + name + "/entities.json";

                // Check if it has entities.json
                std::ifstream test(entPath);
                if (!test.is_open()) continue;
                test.close();

                ss << "  " << name << "\n";
                if (count > 0) jsonSs << ",";
                jsonSs << "\"" << name << "\"";
                ++count;
            }
            closedir(dir);

            jsonSs << "]";

            if (count == 0)
                return ConsoleResult::Ok("No saved states found");

            return ConsoleResult::Ok("Saved states (" + std::to_string(count) + "):\n" + ss.str(),
                                     jsonSs.str());
        }, nullptr
    });

    // ---------------------------------------------------------------
    // help --json - structured command documentation
    // ---------------------------------------------------------------
    registry.Register({"help", "help [command] [--json]",
        "Show help for all commands or a specific command (--json for machine-readable)",
        [&registry](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            bool jsonMode = false;
            std::string target;

            for (auto& arg : args) {
                if (arg == "--json" || arg == "-j")
                    jsonMode = true;
                else
                    target = arg;
            }

            // Override the existing help command behavior
            auto& allCmds = registry.All();

            if (jsonMode && target.empty()) {
                // Full JSON schema of all commands
                std::ostringstream jsonSs;
                jsonSs << "[";
                int count = 0;
                // Sort by name for deterministic output
                std::vector<std::string> names;
                names.reserve(allCmds.size());
                for (auto& [name, _] : allCmds) names.push_back(name);
                std::sort(names.begin(), names.end());

                for (auto& name : names) {
                    auto it = allCmds.find(name);
                    if (it == allCmds.end()) continue;
                    auto& cmd = it->second;

                    if (count > 0) jsonSs << ",";
                    // Escape strings for JSON
                    auto escape = [](const std::string& s) -> std::string {
                        std::string out;
                        out.reserve(s.size());
                        for (char c : s) {
                            if (c == '"') out += "\\\"";
                            else if (c == '\\') out += "\\\\";
                            else if (c == '\n') out += "\\n";
                            else out += c;
                        }
                        return out;
                    };

                    jsonSs << "{\"name\":\"" << escape(cmd.name) << "\""
                           << ",\"usage\":\"" << escape(cmd.usage) << "\""
                           << ",\"description\":\"" << escape(cmd.description) << "\""
                           << "}";
                    ++count;
                }
                jsonSs << "]";

                return ConsoleResult::Ok(
                    std::to_string(count) + " commands (JSON mode)",
                    jsonSs.str());
            }

            if (!target.empty()) {
                // Specific command help
                auto* cmd = registry.Find(target);
                if (!cmd)
                    return ConsoleResult::Error("Unknown command: " + target);

                std::ostringstream ss;
                ss << "  " << cmd->usage << "\n"
                   << "  " << cmd->description << "\n";

                if (jsonMode) {
                    std::ostringstream jsonSs;
                    jsonSs << "{\"name\":\"" << cmd->name << "\""
                           << ",\"usage\":\"" << cmd->usage << "\""
                           << ",\"description\":\"" << cmd->description << "\"}";
                    return ConsoleResult::Ok(ss.str(), jsonSs.str());
                }
                return ConsoleResult::Ok(ss.str());
            }

            // Default: list all commands (text mode)
            std::vector<std::string> names;
            names.reserve(allCmds.size());
            for (auto& [name, _] : allCmds) names.push_back(name);
            std::sort(names.begin(), names.end());

            std::ostringstream ss;
            ss << "Available commands (" << names.size() << "):\n";
            for (auto& name : names) {
                auto it = allCmds.find(name);
                if (it == allCmds.end()) continue;
                ss << "  " << std::setw(24) << std::left << it->second.usage
                   << " " << it->second.description << "\n";
            }
            ss << "\nUse 'help <command>' for details, 'help --json' for machine-readable output.\n";
            return ConsoleResult::Ok(ss.str());
        }, nullptr
    });
}

} // namespace koilo
