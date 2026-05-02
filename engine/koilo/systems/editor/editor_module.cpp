// SPDX-License-Identifier: GPL-3.0-or-later
#include "editor_module.hpp"

#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/logging/log.hpp>
#include <koilo/kernel/console/console_commands.hpp>
#include <koilo/kernel/console/command_registry.hpp>
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/scripting/reflection_bridge.hpp>
#include <koilo/systems/scene/scene.hpp>
#include <koilo/systems/scene/scenenode.hpp>
#include <koilo/systems/scene/camera/camerabase.hpp>
#include <koilo/systems/scene/camera/camera.hpp>
#include <koilo/systems/scene/mesh.hpp>
#include <koilo/systems/render/pipeline/render_pipeline.hpp>
#include <koilo/systems/physics/physics_module.hpp>
#include <koilo/systems/physics/physicsworld.hpp>
#include <koilo/systems/physics/rigidbody.hpp>
#include <koilo/systems/physics/bodypose.hpp>
#include <koilo/debug/debugdraw.hpp>
#include <koilo/core/math/matrix4x4.hpp>
#include <koilo/core/math/transform.hpp>
#include <koilo/core/math/quaternion.hpp>
#include <cmath>
#include <koilo/systems/ui/ui_module.hpp>
#include <koilo/systems/ui/ui.hpp>
#include <koilo/systems/ui/ui_context.hpp>
#include <koilo/systems/ui/widget.hpp>
#include <koilo/systems/ui/auto_inspector.hpp>
#include <koilo/systems/ui/event.hpp>
#include <koilo/core/geometry/ray.hpp>

#include <cstdio>
#include <unordered_set>

namespace koilo::editor {

namespace {
constexpr const char* kEditorKml      = "assets/ui/editor.kml";
constexpr const char* kEditorKss      = "assets/ui/editor.kss";
constexpr const char* kModuleName     = "editor";
constexpr const char* kSelectionSvc   = "selection";
constexpr const char* kHierarchyId    = "hierarchy-scroll";
constexpr const char* kHierarchyEmpty = "hierarchy-empty";
constexpr const char* kHierarchyCount = "hierarchy-count";
constexpr const char* kViewportId     = "game-viewport";
constexpr const char* kInspectorScroll = "inspector-scroll";

ModuleLoader* GetLoader(KoiloKernel& kernel) {
    auto* engine = kernel.Services().Get<scripting::KoiloScriptEngine>("script");
    return engine ? &engine->GetModuleLoader() : nullptr;
}

UI* GetUI(KoiloKernel& kernel) {
    auto* loader = GetLoader(kernel);
    if (!loader) return nullptr;
    auto* uiModule = dynamic_cast<UIModule*>(loader->GetModule("ui"));
    return uiModule ? uiModule->GetUI() : nullptr;
}

Scene* GetScene(KoiloKernel& kernel) {
    return kernel.Services().Get<Scene>("scene");
}

CameraBase* GetCamera(KoiloKernel& kernel) {
    // SceneModule registers the concrete Camera (not CameraBase) under
    // "camera"; upcast to CameraBase here since the editor only needs
    // the abstract API.
    if (auto* cam = kernel.Services().Get<Camera>("camera")) {
        return static_cast<CameraBase*>(cam);
    }
    return nullptr;
}

const ClassDesc* SceneNodeClass() {
    static const ClassDesc* desc =
        scripting::ReflectionBridge::FindClass("SceneNode");
    return desc;
}
} // namespace

EditorModule::EditorModule() = default;

EditorModule::~EditorModule() {
    if (active_) Shutdown();
}

ModuleInfo EditorModule::GetInfo() const {
    return {kModuleName, "0.1.0", ModulePhase::Overlay};
}

bool EditorModule::Initialize(KoiloKernel& kernel) {
    kernel_ = &kernel;

    UI* ui = GetUI(kernel);
    if (!ui) {
        KL_ERR("editor", "UI module not available; editor cannot attach");
        return false;
    }

    // Editor owns the SelectionManager and exposes it as a kernel
    // service so other panels (inspector in E4, viewport in E3.2)
    // can subscribe without round-tripping through the editor.
    // Created BEFORE LoadEditorMarkupAndWire so that the wiring
    // pass can use it (selection callbacks, viewport click etc.).
    selection_ = std::make_unique<SelectionManager>();
    kernel.Services().Register(kSelectionSvc, selection_.get());
    selectionToken_ = selection_->Subscribe([this]() { OnSelectionChanged(); });

    if (!LoadEditorMarkupAndWire()) {
        return false;
    }

    ApplyPlayStateToWorld();
    RefreshPlayStateUI();
    if (!scenePath_.empty()) {
        WatchScenePath(scenePath_);
    }
    WatchEditorMarkup();

    active_ = true;
    KL_LOG("editor", "Editor shell loaded (root widget id %d)", editorRoot_);
    return true;
}

bool EditorModule::LoadEditorMarkupAndWire() {
    if (!kernel_) return false;
    UI* ui = GetUI(*kernel_);
    if (!ui) return false;

    editorRoot_ = ui->LoadMarkup(kEditorKml, kEditorKss);
    if (editorRoot_ < 0) {
        KL_ERR("editor", "Failed to load editor markup (%s + %s)",
               kEditorKml, kEditorKss);
        return false;
    }

    auto& ctx = ui->Context();
    hierarchyScrollIdx_ = ctx.FindWidget(kHierarchyId);
    hierarchyEmptyIdx_  = ctx.FindWidget(kHierarchyEmpty);
    hierarchyCountIdx_  = ctx.FindWidget(kHierarchyCount);

    viewportIdx_ = ctx.FindWidget(kViewportId);
    pipeline_    = kernel_->Services().Get<rhi::RenderPipeline>("render_backend");

    if (viewportIdx_ >= 0) {
        ctx.SetOnClick(viewportIdx_, [this](ui::Widget&) {
            HandleViewportClick();
        });
        InstallEditorCameraHandlers();
    }

    inspectorScrollIdx_ = ctx.FindWidget(kInspectorScroll);
    if (inspectorScrollIdx_ >= 0) {
        StripInspectorPlaceholder();
    }

    // Re-register all editor commands. Force re-installation by
    // dropping any prior registry; fresh wiring on every reload keeps
    // the lambda captures pointing at the (still-stable) `this`.
    commands_.reset();
    saveShortcutInstalled_ = false;
    InstallSaveShortcut();
    InstallPlayCommands();
    InstallReloadCommand();
    InstallPlayToolbar();

    // Force a hierarchy + inspector rebuild on the next tick since
    // widget indices may have changed.
    lastHierarchyGen_   = 0;
    hierarchyEverBuilt_ = false;
    hierarchyRows_.clear();
    inspectorContentIdx_ = -1;
    if (selection_) RebuildInspector();
    return true;
}

void EditorModule::Update(float dt) {
    if (!active_ || !kernel_) return;
    savePollClock_ += dt;
    PollFileWatcher(dt);
    RefreshHierarchy();
    RefreshViewportTexture();
    RefreshSelectionGizmo();
    // Inspector edits write directly through reflected field pointers
    // (auto_inspector callbacks bypass setters), so any local-transform
    // change won't itself flip SceneNode::worldDirty_.  Force the
    // selected node dirty every frame so world transforms recompute
    // when the user drags a Position/Rotation/Scale slider.  Cheap: a
    // single store + a recompute on next GetWorldTransform().
    if (selection_ && !selection_->IsEmpty()) {
        const auto& e = selection_->Primary();
        if (e.instance && e.desc == SceneNodeClass()) {
            static_cast<SceneNode*>(e.instance)->MarkDirty();
        }
    }
}

void EditorModule::Shutdown() {
    if (!active_) return;

    if (selection_ && selectionToken_) {
        selection_->Unsubscribe(selectionToken_);
        selectionToken_ = 0;
    }

    if (kernel_) {
        // Unregister the selection service so a re-load doesn't see
        // a dangling pointer.
        kernel_->Services().Unregister(kSelectionSvc);

        // Re-enable physics simulation we may have gated off via the
        // editor PlayState. Without this, unloading the editor while
        // it sat in Editing/Paused would leave the runtime frozen.
        if (auto* loader = GetLoader(*kernel_)) {
            if (auto* phys = dynamic_cast<PhysicsModule*>(loader->GetModule("physics"))) {
                phys->SetSimulationEnabled(true);
            }
        }
        playState_ = PlayState::Editing;

        if (UI* ui = GetUI(*kernel_)) {
            // Detach our owned command registry before destroying it
            // so UIContext doesn't dispatch into freed memory.
            if (saveShortcutInstalled_) {
                ui->Context().SetCommandRegistry(nullptr);
                saveShortcutInstalled_ = false;
            }
            // Drop tracked rows individually so click callbacks
            // capturing `this` don't outlive the module.
            for (auto& row : hierarchyRows_) {
                if (row.widgetIdx >= 0) {
                    ui->Context().RemoveTreeNode(row.widgetIdx);
                }
            }
            hierarchyRows_.clear();
            if (editorRoot_ >= 0) {
                ui->DestroyWidget(editorRoot_);
            }
        }
    }

    commands_.reset();
    sceneDirty_         = false;
    ClearScenarioSnapshot();
    watcher_.reset();
    watcherAccum_  = 0.0f;
    savePollClock_ = 0.0f;
    lastSaveClock_ = -1.0f;
    selection_.reset();
    pipeline_           = nullptr;
    editorRoot_         = -1;
    hierarchyScrollIdx_ = -1;
    hierarchyEmptyIdx_  = -1;
    hierarchyCountIdx_  = -1;
    viewportIdx_        = -1;
    inspectorScrollIdx_  = -1;
    inspectorContentIdx_ = -1;
    lastHierarchyGen_   = 0;
    hierarchyEverBuilt_ = false;
    active_             = false;
    KL_LOG("editor", "Editor shell unloaded");
}

// ---------------------------------------------------------------------
// Hierarchy panel
// ---------------------------------------------------------------------

void EditorModule::RefreshHierarchy() {
    Scene* scene = GetScene(*kernel_);
    UI*    ui    = GetUI(*kernel_);
    if (!ui || hierarchyScrollIdx_ < 0) return;
    auto& ctx = ui->Context();

    // Cheap-noop when nothing structural changed. First tick always
    // runs (hierarchyEverBuilt_ == false) so the placeholder gets
    // replaced even for an empty scene.
    const std::uint64_t gen = scene ? scene->HierarchyGeneration() : 0;
    if (hierarchyEverBuilt_ && gen == lastHierarchyGen_) {
        return;
    }
    lastHierarchyGen_   = gen;
    hierarchyEverBuilt_ = true;

    // Tear down previous rows. The selection's pointer (if any) is a
    // SceneNode owned by Scene, not by these widgets, so dropping
    // widgets does not invalidate selection.
    for (auto& row : hierarchyRows_) {
        if (row.widgetIdx >= 0) {
            ctx.RemoveTreeNode(row.widgetIdx);
        }
    }
    hierarchyRows_.clear();

    if (!scene || scene->GetNodeCount() == 0) {
        if (hierarchyEmptyIdx_ >= 0) ctx.SetVisible(hierarchyEmptyIdx_, true);
        if (hierarchyCountIdx_ >= 0) ctx.SetText(hierarchyCountIdx_, "0 nodes");
        return;
    }

    if (hierarchyEmptyIdx_ >= 0) ctx.SetVisible(hierarchyEmptyIdx_, false);

    auto roots = scene->GetRootNodes();
    for (SceneNode* root : roots) {
        AppendNodeRow(hierarchyScrollIdx_, root, /*depth=*/0);
    }

    if (hierarchyCountIdx_ >= 0) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%zu nodes", scene->GetNodeCount());
        ctx.SetText(hierarchyCountIdx_, buf);
    }

    // Re-apply selection highlight after the rebuild.
    OnSelectionChanged();
}

void EditorModule::AppendNodeRow(int scrollIdx, SceneNode* node, int depth) {
    if (!node) return;
    UI*  ui  = GetUI(*kernel_);
    if (!ui) return;
    auto& ctx = ui->Context();

    const auto& children     = node->GetChildren();
    const bool  hasChildren  = !children.empty();
    const std::string& name  = node->GetName();

    // Use a stable id based on the node pointer so KSS rules that
    // attach to id remain deterministic across rebuilds; any leftover
    // id from a destroyed widget is harmless because we always look
    // up by the freshly-returned widget index.
    char idBuf[64];
    std::snprintf(idBuf, sizeof(idBuf), "scene-node-%p",
                  static_cast<const void*>(node));

    int widgetIdx = ctx.AddTreeNode(scrollIdx, idBuf,
                                    name.empty() ? "(unnamed)" : name.c_str(),
                                    depth, hasChildren);
    if (widgetIdx < 0) return;

    // Default each row to expanded.  `Widget::expanded` defaults to
    // false but `flags.visible` defaults to true, so the very first
    // `UpdateTreeVisibility` call (triggered by *any* chevron click)
    // would otherwise hide every depth-1 sibling because the depth-0
    // ancestor still reads as collapsed.  Auto-expanding mirrors how
    // Unity's Hierarchy panel boots: nothing hidden until the user
    // explicitly collapses a node.
    if (auto* tw = ctx.GetWidget(widgetIdx)) {
        tw->expanded = true;
    }

    HierarchyRow row;
    row.widgetIdx = widgetIdx;
    row.node      = node;
    hierarchyRows_.push_back(row);

    // Wire click -> selection. Capture by value: `node` is stable for
    // the Scene's lifetime; `selection_` outlives the callback because
    // Shutdown() removes the widget (and therefore its callback)
    // before tearing down `selection_`.
    SceneNode*        capturedNode = node;
    SelectionManager* sel          = selection_.get();
    const ClassDesc*  desc         = SceneNodeClass();
    ctx.SetOnClick(widgetIdx, [sel, capturedNode, desc](ui::Widget&) {
        if (sel) sel->Select(capturedNode, desc);
    });

    for (SceneNode* child : children) {
        AppendNodeRow(scrollIdx, child, depth + 1);
    }
}

void EditorModule::OnSelectionChanged() {
    UI* ui = GetUI(*kernel_);
    if (!ui || !selection_) return;
    auto& ctx = ui->Context();

    void* primary = selection_->IsEmpty()
                        ? nullptr
                        : selection_->Primary().instance;

    for (const auto& row : hierarchyRows_) {
        if (row.widgetIdx < 0) continue;
        const bool sel = (row.node == primary);
        ctx.SetSelected(row.widgetIdx, sel);
    }

    if (primary) {
        const auto* node = static_cast<const SceneNode*>(primary);
        KL_LOG("editor", "Selected node: %s", node->GetName().c_str());
    }

    // rebuild the inspector subtree for the new selection.
    RebuildInspector();
}

// ---------------------------------------------------------------------
// Viewport panel
// ---------------------------------------------------------------------

void EditorModule::RefreshViewportTexture() {
    if (viewportIdx_ < 0 || !kernel_) return;
    // The render pipeline is registered by the platform host AFTER the
    // editor module's Initialize runs (display/RHI selection happens
    // during the host's frame-loop bring-up).  Resolve it lazily on the
    // first frame it becomes available.
    if (!pipeline_) {
        pipeline_ = kernel_->Services().Get<rhi::RenderPipeline>("render_backend");
        if (!pipeline_) return;
    }
    UI* ui = GetUI(*kernel_);
    if (!ui) return;
    auto* widget = ui->Context().GetWidget(viewportIdx_);
    if (!widget) return;

    // Re-bind the offscreen color id every frame.  The id can change
    // when the pipeline recreates the offscreen target on resize; we
    // intentionally do NOT cache it across frames.  There is a known
    // edge-case race where a same-frame resize between Update() and UI
    // render leaves a stale id in `widget.textureId`; the next frame
    // self-heals.  See E3 follow-up notes.
    widget->textureId = pipeline_->GetOffscreenColorTextureId();
}

void EditorModule::HandleViewportClick() {
    if (!kernel_ || viewportIdx_ < 0 || !selection_) return;
    if (!pipeline_) {
        pipeline_ = kernel_->Services().Get<rhi::RenderPipeline>("render_backend");
        if (!pipeline_) return;
    }
    UI* ui = GetUI(*kernel_);
    Scene* scene = GetScene(*kernel_);
    CameraBase* camera = GetCamera(*kernel_);
    if (!ui || !scene || !camera) return;

    auto& ctx = ui->Context();
    const auto* widget = ctx.GetWidget(viewportIdx_);
    if (!widget) return;
    const auto& r = widget->computedRect;
    if (r.w <= 0.0f || r.h <= 0.0f) return;

    // DragPointerX/Y is the most recent press position recorded by
    // UIContext; for a non-drag click that equals the click point,
    // which is what we want.
    float px = ctx.DragPointerX();
    float py = ctx.DragPointerY();

    // Convert widget-local pixel coords to NDC ([-1,+1], +Y up).
    float ndcX =  2.0f * (px - r.x) / r.w - 1.0f;
    float ndcY =  1.0f - 2.0f * (py - r.y) / r.h;

    Ray ray = pipeline_->BuildPickRay(camera, ndcX, ndcY);
    SceneNode* hit = scene->PickNode(ray);
    if (hit) {
        selection_->Select(hit, SceneNodeClass());
    } else {
        selection_->ClearSelection();
    }
}

void EditorModule::RefreshSelectionGizmo() {
    if (!selection_ || selection_->IsEmpty()) return;
    const SelectionEntry& entry = selection_->Primary();
    if (!entry.instance || entry.desc != SceneNodeClass()) return;

    auto* node = static_cast<SceneNode*>(entry.instance);
    Mesh* mesh = node->GetMesh();
    if (!mesh) return;

    Vector3D mn, mx;
    mesh->GetMinMaxDimensions(mn, mx);
    const Vector3D center  = (mn + mx) * 0.5f;
    const Vector3D extents = (mx - mn) * 0.5f;
    if (extents.X <= 0.0f && extents.Y <= 0.0f && extents.Z <= 0.0f) return;

    const Transform& xf = node->GetWorldTransform();
    Matrix4x4 world = Matrix4x4::TRS(xf.GetPosition(),
                                     xf.GetRotation(),
                                     xf.GetScale());

    // Bright yellow, single-frame (duration=0), drawn-on-top so the
    // gizmo is visible even when the selected mesh is itself opaque.
    DebugDraw::GetInstance().DrawOrientedBox(center, extents, world,
                                             Color::Yellow,
                                             /*duration=*/0.0f,
                                             /*depthTest=*/false,
                                             DebugDrawMode::Wireframe);
}

// ---------------------------------------------------------------------
// Inspector panel
// ---------------------------------------------------------------------

void EditorModule::StripInspectorPlaceholder() {
    UI* ui = GetUI(*kernel_);
    if (!ui || inspectorScrollIdx_ < 0) return;
    auto& ctx = ui->Context();
    auto* scroll = ctx.GetWidget(inspectorScrollIdx_);
    if (!scroll) return;

    // Iterate from the tail so DestroyWidget's sibling-list shuffle
    // doesn't skip entries.  scroll->children indexes are pool ids.
    while (scroll->childCount > 0) {
        const int childIdx = scroll->children[scroll->childCount - 1];
        if (childIdx < 0) {
            // Defensive: stale slot; nothing to do.
            break;
        }
        ctx.DestroyWidget(childIdx);
    }
}

void EditorModule::RebuildInspector() {
    if (inspectorScrollIdx_ < 0 || !kernel_) return;
    UI* ui = GetUI(*kernel_);
    if (!ui) return;
    auto& ctx = ui->Context();

    // Drop the previous inspector tree (if any) before building a new
    // one.  The old subtree's callbacks captured the previous instance
    // pointer; destroying it is what severs those references.
    if (inspectorContentIdx_ >= 0) {
        ctx.DestroyWidget(inspectorContentIdx_);
        inspectorContentIdx_ = -1;
    }

    if (!selection_ || selection_->IsEmpty()) return;
    const SelectionEntry& entry = selection_->Primary();
    if (!entry.instance || !entry.desc) return;

    auto result = ui::GenerateInspector(entry.desc, entry.instance,
                                        ctx, inspectorScrollIdx_,
                                        [this]() { MarkSceneDirty(); });
    inspectorContentIdx_ = result.rootWidget;

    if (result.rootWidget < 0) {
        KL_LOG("editor",
               "Inspector: failed to generate widget tree for %s",
               entry.desc->name);
    } else if (result.fieldCount == 0) {
        KL_LOG("editor",
               "Inspector: %s has no reflected fields",
               entry.desc->name);
    }
}

// ---------------------------------------------------------------------
// Editor camera
// ---------------------------------------------------------------------

void EditorModule::SyncEditorCameraFromCamera() {
    if (editorCameraSynced_ || !kernel_) return;
    CameraBase* cam = GetCamera(*kernel_);
    if (!cam) return;
    Transform* t = cam->GetTransform();
    if (!t) return;

    // Initialise orbit so the first frame matches whatever the script
    // configured -- target = camera position + 5*forward; recover
    // distance + yaw/pitch from the offset vector.
    Vector3D pos = t->GetPosition();
    orbitTargetX_ = pos.X;
    orbitTargetY_ = pos.Y;
    orbitTargetZ_ = pos.Z + orbitDistance_;  // assume looking +Z by default
    // We don't try to extract yaw/pitch from the rotation -- the next
    // ApplyEditorCameraToScene() will overwrite the rotation with our
    // computed look-at, which keeps state consistent.
    editorCameraSynced_ = true;
    ApplyEditorCameraToScene();
}

void EditorModule::ApplyEditorCameraToScene() {
    if (!kernel_) return;
    CameraBase* cam = GetCamera(*kernel_);
    if (!cam) return;
    Transform* t = cam->GetTransform();
    if (!t) return;

    // Spherical -> cartesian offset from target.  Y is world up; pitch
    // is angle above the XZ plane.
    const float cp = std::cos(orbitPitch_);
    const float sp = std::sin(orbitPitch_);
    const float cy = std::cos(orbitYaw_);
    const float sy = std::sin(orbitYaw_);

    Vector3D target(orbitTargetX_, orbitTargetY_, orbitTargetZ_);
    Vector3D offset(orbitDistance_ * cp * sy,
                    orbitDistance_ * sp,
                    orbitDistance_ * cp * cy);
    Vector3D camPos = target + offset;

    Vector3D forward = (target - camPos);
    Vector3D up(0.0f, 1.0f, 0.0f);
    Quaternion q = Quaternion::LookAt(forward, up);

    t->SetPosition(camPos);
    t->SetRotation(q);
}

void EditorModule::InstallEditorCameraHandlers() {
    if (!kernel_ || viewportIdx_ < 0) return;
    UI* ui = GetUI(*kernel_);
    if (!ui) return;
    auto& ctx = ui->Context();

    ctx.SetOnPointerDown(viewportIdx_,
        [this](ui::Widget&, const ui::Event& e) {
            // Only RMB (orbit) and MMB (pan) start a camera drag.  LMB
            // continues to flow through the normal click pipeline so
            // viewport picking still works.
            if (e.pointerButton != 1 && e.pointerButton != 2) return;
            SyncEditorCameraFromCamera();
            dragButton_   = e.pointerButton;
            prevPointerX_ = e.pointerX;
            prevPointerY_ = e.pointerY;
        });

    ctx.SetOnPointerMove(viewportIdx_,
        [this](ui::Widget&, const ui::Event& e) {
            if (dragButton_ < 0) return;
            const float dx = e.pointerX - prevPointerX_;
            const float dy = e.pointerY - prevPointerY_;
            prevPointerX_ = e.pointerX;
            prevPointerY_ = e.pointerY;

            constexpr float kOrbitSpeed = 0.005f;  // rad/pixel
            constexpr float kPanSpeed   = 0.0025f; // world u/pixel/dist
            constexpr float kPitchLimit = 1.5533f; // ~89 degrees

            if (dragButton_ == 1) {
                // RMB: orbit yaw/pitch around target.
                orbitYaw_   -= dx * kOrbitSpeed;
                orbitPitch_ += dy * kOrbitSpeed;
                if (orbitPitch_ >  kPitchLimit) orbitPitch_ =  kPitchLimit;
                if (orbitPitch_ < -kPitchLimit) orbitPitch_ = -kPitchLimit;
            } else if (dragButton_ == 2) {
                // MMB: pan in the camera's right/up plane.  Speed
                // scales with distance so far views move faster.
                const float cy = std::cos(orbitYaw_);
                const float sy = std::sin(orbitYaw_);
                Vector3D right(cy, 0.0f, -sy);
                Vector3D upWS(0.0f, 1.0f, 0.0f);
                const float scale = kPanSpeed * orbitDistance_;
                orbitTargetX_ += (-dx * right.X + dy * upWS.X) * scale;
                orbitTargetY_ += (-dx * right.Y + dy * upWS.Y) * scale;
                orbitTargetZ_ += (-dx * right.Z + dy * upWS.Z) * scale;
            }
            ApplyEditorCameraToScene();
        });

    ctx.SetOnPointerUp(viewportIdx_,
        [this](ui::Widget&, const ui::Event&) {
            dragButton_ = -1;
        });

    ctx.SetOnScroll(viewportIdx_,
        [this](ui::Widget&, const ui::Event& e) {
            // Mark consumed so the parent scrollview (if any) doesn't
            // also scroll on top of the zoom.
            const_cast<ui::Event&>(e).Consume();
            SyncEditorCameraFromCamera();
            const float factor = (e.scrollDelta > 0.0f) ? 0.9f : 1.1f;
            orbitDistance_ *= factor;
            if (orbitDistance_ < 0.05f)   orbitDistance_ = 0.05f;
            if (orbitDistance_ > 10000.f) orbitDistance_ = 10000.f;
            ApplyEditorCameraToScene();
        });
}

// -- Registration helpers --------------------------------------------

// ---------------------------------------------------------------------
// dirty tracking + Save (Ctrl+S)
// ---------------------------------------------------------------------

void EditorModule::MarkSceneDirty() {
    if (sceneDirty_) return;
    sceneDirty_ = true;
    RefreshWindowTitle();
}

void EditorModule::ClearSceneDirty() {
    if (!sceneDirty_) return;
    sceneDirty_ = false;
    RefreshWindowTitle();
}

void EditorModule::RefreshWindowTitle() {
    // Title-bar update is best-effort.  No reliable cross-backend hook
    // exposes the live SDL window pointer to the editor module yet, so
    // for now we just log the dirty state so users can see edits in
    // the console. may surface a Display::SetTitle service
    // that we can call here.
    KL_LOG("editor", "Scene %s%s",
           scenePath_.empty() ? "(unsaved)" : scenePath_.c_str(),
           sceneDirty_ ? " *" : "");
}

bool EditorModule::SaveSceneNow() {
    if (!kernel_) return false;
    Scene* scene = GetScene(*kernel_);
    if (!scene) {
        KL_WARN("editor", "Save: no Scene available");
        return false;
    }

    std::string path = scenePath_;
    if (path.empty()) {
        // Default fallback so the save shortcut still produces useful
        // output even when no --editor <scene> argument was provided.
        path = "assets/scenes/editor_save.kscene";
    }

    if (!scene->SaveToKScene(path)) {
        KL_WARN("editor", "Save failed: %s", path.c_str());
        return false;
    }
    KL_LOG("editor", "Saved scene to %s", path.c_str());
    if (scenePath_.empty()) scenePath_ = path;  // Sticky for next save.
    lastSaveClock_ = savePollClock_;            // Debounce stamp for self-trigger filter.
    WatchScenePath(scenePath_);                 // Track external edits.
    ClearSceneDirty();
    return true;
}

void EditorModule::InstallSaveShortcut() {
    if (saveShortcutInstalled_ || !kernel_) return;
    UI* ui = GetUI(*kernel_);
    if (!ui) return;

    if (!commands_) commands_ = std::make_unique<ui::CommandRegistry>();
    commands_->Register(
        "editor.save", "Save Scene", "File",
        [this]() { SaveSceneNow(); },
        nullptr,
        ui::Shortcut(ui::KeyCode::S, /*ctrl*/true));

    ui->Context().SetCommandRegistry(commands_.get());
    saveShortcutInstalled_ = true;
}

// -- PlayState ----------------------------------------------

namespace {
const char* PlayStateName(EditorModule::PlayState s) {
    switch (s) {
        case EditorModule::PlayState::Editing: return "Editing";
        case EditorModule::PlayState::Playing: return "Playing";
        case EditorModule::PlayState::Paused:  return "Paused";
    }
    return "?";
}
} // namespace

void EditorModule::ApplyPlayStateToWorld() {
    if (!kernel_) return;
    auto* loader = GetLoader(*kernel_);
    if (!loader) return;
    auto* phys = dynamic_cast<PhysicsModule*>(loader->GetModule("physics"));
    if (!phys) return;
    // Aerodynamics drives forces from a physics pre-step callback, so
    // gating PhysicsModule alone is sufficient to freeze the world.
    phys->SetSimulationEnabled(playState_ == PlayState::Playing);
}

void EditorModule::SetPlayState(PlayState next) {
    if (next == playState_) return;
    PlayState prev = playState_;

    // Snapshot when leaving Editing for the first time; restore when
    // transitioning back to Editing from Playing/Paused.
    if (prev == PlayState::Editing && next != PlayState::Editing) {
        CaptureScenarioSnapshot();
    } else if (prev != PlayState::Editing && next == PlayState::Editing) {
        RestoreScenarioSnapshot();
    }

    playState_ = next;
    ApplyPlayStateToWorld();
    RefreshPlayStateUI();
    KL_LOG("editor", "PlayState %s -> %s", PlayStateName(prev), PlayStateName(next));
}

void EditorModule::InstallPlayCommands() {
    if (!commands_) commands_ = std::make_unique<ui::CommandRegistry>();
    commands_->Register("editor.play",  "Play",  "Play",
        [this]() { Play();  }, nullptr, ui::Shortcut(ui::KeyCode::F5));
    commands_->Register("editor.pause", "Pause", "Play",
        [this]() { Pause(); }, nullptr, ui::Shortcut(ui::KeyCode::F6));
    commands_->Register("editor.stop",  "Stop",  "Play",
        [this]() { Stop();  }, nullptr, ui::Shortcut(ui::KeyCode::F7));
}

void EditorModule::InstallPlayToolbar() {
    if (!kernel_) return;
    UI* ui = GetUI(*kernel_);
    if (!ui) return;
    auto& ctx = ui->Context();

    playButtonIdx_  = ctx.FindWidget("btn-play");
    pauseButtonIdx_ = ctx.FindWidget("btn-pause");
    stopButtonIdx_  = ctx.FindWidget("btn-stop");
    statusTextIdx_  = ctx.FindWidget("status-text");

    if (playButtonIdx_ >= 0) {
        ctx.SetOnClick(playButtonIdx_, [this](ui::Widget&) { Play(); });
    }
    if (pauseButtonIdx_ >= 0) {
        ctx.SetOnClick(pauseButtonIdx_, [this](ui::Widget&) {
            // Toggle between Paused and Playing so a second Pause click
            // resumes (matches Unity / Unreal toolbar behavior).
            SetPlayState(playState_ == PlayState::Paused
                            ? PlayState::Playing
                            : PlayState::Paused);
        });
    }
    if (stopButtonIdx_ >= 0) {
        ctx.SetOnClick(stopButtonIdx_, [this](ui::Widget&) { Stop(); });
    }
}

void EditorModule::RefreshPlayStateUI() {
    if (!kernel_) return;
    UI* ui = GetUI(*kernel_);
    if (!ui) return;
    auto& ctx = ui->Context();

    // Mirror state into button labels (no runtime CSS-class API yet).
    if (playButtonIdx_ >= 0) {
        ctx.SetText(playButtonIdx_,
            playState_ == PlayState::Playing ? "\xE2\x96\xB6 Playing"
                                             : "\xE2\x96\xB6 Play");
    }
    if (pauseButtonIdx_ >= 0) {
        ctx.SetText(pauseButtonIdx_,
            playState_ == PlayState::Paused ? "\xE2\x8F\xB8 Paused"
                                            : "\xE2\x8F\xB8 Pause");
    }
    if (statusTextIdx_ >= 0) {
        ctx.SetText(statusTextIdx_, PlayStateName(playState_));
    }
}

// -- snapshot + restore ---------------------------------

void EditorModule::CollectAllNodes(std::vector<SceneNode*>& out) const {
    out.clear();
    Scene* scene = kernel_ ? GetScene(*kernel_) : nullptr;
    if (!scene) return;
    auto roots = scene->GetRootNodes();
    // Iterative DFS so deeply nested hierarchies don't blow the stack
    // and the visit order is stable (siblings in declaration order,
    // children before subsequent siblings).
    std::vector<SceneNode*> stack(roots.rbegin(), roots.rend());
    while (!stack.empty()) {
        SceneNode* n = stack.back();
        stack.pop_back();
        if (!n) continue;
        out.push_back(n);
        const auto& kids = n->GetChildren();
        for (auto it = kids.rbegin(); it != kids.rend(); ++it) {
            stack.push_back(*it);
        }
    }
}

void EditorModule::CaptureScenarioSnapshot() {
    nodeSnapshot_.clear();
    bodySnapshot_.clear();
    hasSnapshot_ = false;
    if (!kernel_) return;

    std::vector<SceneNode*> nodes;
    CollectAllNodes(nodes);
    nodeSnapshot_.reserve(nodes.size());
    for (SceneNode* n : nodes) {
        if (!n) continue;
        Transform& t = n->GetLocalTransform();
        NodeSnapshot snap;
        snap.node     = n;
        snap.position = t.GetPosition();
        snap.rotation = t.GetRotation();
        snap.scale    = t.GetScale();
        nodeSnapshot_.push_back(snap);
    }

    if (auto* loader = GetLoader(*kernel_)) {
        if (auto* phys = dynamic_cast<PhysicsModule*>(loader->GetModule("physics"))) {
            if (auto* world = phys->GetWorld()) {
                int n = world->GetBodyCount();
                bodySnapshot_.reserve(static_cast<std::size_t>(n));
                for (int i = 0; i < n; ++i) {
                    RigidBody* b = world->GetBody(i);
                    if (!b) continue;
                    BodySnapshot snap;
                    snap.body            = b;
                    const BodyPose& pose = b->GetPose();
                    snap.position        = pose.position;
                    snap.orientation     = pose.orientation;
                    snap.linearVelocity  = b->GetVelocity();
                    snap.angularVelocity = b->GetAngularVelocity();
                    bodySnapshot_.push_back(snap);
                }
            }
        }
    }

    hasSnapshot_ = true;
    KL_LOG("editor", "Snapshot captured: %zu nodes, %zu bodies",
           nodeSnapshot_.size(), bodySnapshot_.size());
}

void EditorModule::RestoreScenarioSnapshot() {
    if (!hasSnapshot_) return;

    // Build a live-node set so we can defensively skip entries whose
    // SceneNode was destroyed between snapshot and restore (e.g. if
    // play-mode script deleted a node - currently not possible but
    // cheap insurance).
    std::vector<SceneNode*> live;
    CollectAllNodes(live);
    std::unordered_set<SceneNode*> liveSet(live.begin(), live.end());

    int restoredNodes = 0;
    for (const auto& snap : nodeSnapshot_) {
        if (!snap.node || !liveSet.count(snap.node)) continue;
        Transform& t = snap.node->GetLocalTransform();
        t.SetPosition(snap.position);
        t.SetRotation(snap.rotation);
        t.SetScale(snap.scale);
        snap.node->MarkDirty();
        ++restoredNodes;
    }

    int restoredBodies = 0;
    for (const auto& snap : bodySnapshot_) {
        if (!snap.body) continue;
        BodyPose pose(snap.position, snap.orientation);
        snap.body->SetPose(pose);
        // Use *Raw setters to leave sleep islands untouched - the
        // editor's intent is "restart from rest", not "wake bodies".
        snap.body->SetVelocityRaw(snap.linearVelocity);
        snap.body->SetAngularVelocityRaw(snap.angularVelocity);
        ++restoredBodies;
    }

    KL_LOG("editor", "Snapshot restored: %d nodes, %d bodies",
           restoredNodes, restoredBodies);
}

void EditorModule::ClearScenarioSnapshot() {
    nodeSnapshot_.clear();
    bodySnapshot_.clear();
    hasSnapshot_ = false;
}

// -- file watcher -------------------------------------------

void EditorModule::PollFileWatcher(float dt) {
    if (!watcher_) return;
    watcherAccum_ += dt;
    if (watcherAccum_ < kWatcherPollInterval) return;
    watcherAccum_ = 0.0f;
    watcher_->Poll();
}

void EditorModule::WatchScenePath(const std::string& path) {
    if (path.empty()) return;
    if (!watcher_) {
        watcher_ = std::make_unique<FileWatcher>();
        watcher_->SetCallback([this](const std::string& p) {
            HandleFileChanged(p);
        });
    }
    // Re-Watch is a no-op the first time we see a path and refreshes
    // mtime otherwise - both cases are correct here (we want the
    // current on-disk timestamp to be the new baseline so a fresh
    // save-then-save doesn't immediately fire HandleFileChanged on
    // the second one).
    watcher_->Unwatch(path);
    watcher_->Watch(path);
}

void EditorModule::HandleFileChanged(const std::string& path) {
    // Route editor markup edits (E6d) before the scene-debounce path
    // -- the markup files are not subject to save-self-trigger since
    // the editor never writes to them.
    if (path == kMarkupKmlPath || path == kMarkupKssPath) {
        if (reloadingMarkup_) return;  // Ignore self-induced events.
        KL_LOG("editor", "Editor markup changed on disk: %s", path.c_str());
        ReloadEditorMarkup();
        return;
    }

    // Drop watcher events that fall inside the self-trigger window
    // after our own save. The window is generous (1.5s) to cover
    // slow filesystems that defer mtime visibility.
    if (lastSaveClock_ >= 0.0f &&
        path == scenePath_ &&
        (savePollClock_ - lastSaveClock_) < kSaveSelfTriggerWindow) {
        // Re-arm so the next genuine external edit is observed.
        if (watcher_) {
            watcher_->Unwatch(path);
            watcher_->Watch(path);
        }
        return;
    }
    OnSceneFileChanged(path);
}

void EditorModule::OnSceneFileChanged(const std::string& path) {
    if (path != scenePath_) {
        // Foreign path the watcher knows about but we don't have a
        // route for. (Markup edits are handled in HandleFileChanged
        // before reaching here.)
        KL_LOG("editor", "File changed (untracked): %s", path.c_str());
        return;
    }
    if (sceneDirty_) {
        // Conflict: editor has unsaved edits and disk just changed.
        // Without a real modal UI we surface the conflict via a
        // warning + leave the in-memory scene alone. The user can
        // resolve manually by either saving (Ctrl+S, blows away
        // the disk version) or running `editor.reload` (silently
        // discards their edits in favor of disk).
        KL_WARN("editor",
                "Scene file changed on disk while editor has unsaved edits: "
                "%s. Use Ctrl+S to keep your version, or run `editor.reload` "
                "to discard edits and load disk.",
                path.c_str());
        // Re-arm so the next disk edit still fires.
        if (watcher_) {
            watcher_->Unwatch(path);
            watcher_->Watch(path);
        }
        return;
    }
    // Not dirty: silently reload from disk.
    KL_LOG("editor", "Scene file changed on disk: %s -- reloading",
           path.c_str());
    if (!ReloadSceneFromDisk()) {
        // Reload failed (script engine restored prior state). Re-arm
        // so the next edit retries.
        if (watcher_) {
            watcher_->Unwatch(path);
            watcher_->Watch(path);
        }
    }
}

bool EditorModule::ReloadSceneFromDisk() {
    if (!kernel_) return false;
    auto* engine = kernel_->Services().Get<scripting::KoiloScriptEngine>("script");
    if (!engine) {
        KL_WARN("editor", "Reload: script engine unavailable");
        return false;
    }
    // We can only reload through KoiloScriptEngine::Reload() when the
    // editor's open scene matches the script engine's currently loaded
    // file. If they diverge (e.g. editor opened a separate .kscene via
    // a hypothetical LoadScene() global -- deferred to v2), we'd need
    // a finer-grained loader. For now: log and bail out.
    const std::string& current = engine->GetCurrentScriptPath();
    if (!scenePath_.empty() && scenePath_ != current) {
        KL_WARN("editor",
                "Reload: scene path %s does not match the loaded script %s "
                "(separate .kscene loading is not yet wired)",
                scenePath_.c_str(), current.c_str());
        return false;
    }

    // Drop editor-side state that holds raw pointers into the scene.
    // The scene rebuild may invalidate every SceneNode/RigidBody we
    // still cache. SelectionManager itself survives -- we just clear
    // its current selection.
    if (selection_) selection_->ClearSelection();
    ClearScenarioSnapshot();
    hierarchyRows_.clear();
    lastHierarchyGen_   = 0;
    hierarchyEverBuilt_ = false;

    KL_LOG("editor", "Reloading scene from %s", current.c_str());
    bool ok = engine->Reload();
    if (!ok) {
        KL_ERR("editor", "Scene reload failed (previous state restored)");
        return false;
    }

    // Re-arm watcher with the fresh on-disk mtime as the new baseline
    // and reset dirty (the on-disk version IS now the in-memory one).
    if (!scenePath_.empty()) WatchScenePath(scenePath_);
    sceneDirty_ = false;
    RefreshWindowTitle();
    // Force the inspector + hierarchy to redraw against the fresh
    // scene on the next tick.
    if (selection_) RebuildInspector();
    return true;
}

void EditorModule::InstallReloadCommand() {
    if (!kernel_) return;
    UI* ui = GetUI(*kernel_);
    if (!ui) return;
    if (!commands_) commands_ = std::make_unique<ui::CommandRegistry>();
    commands_->Register(
        "editor.reload", "Reload Scene from Disk", "File",
        [this]() { ReloadSceneFromDisk(); },
        nullptr,
        ui::Shortcut());  // No keyboard shortcut (modal-substitute use).
    ui->Context().SetCommandRegistry(commands_.get());
}

// -- editor markup hot-reload -------------------------------

void EditorModule::WatchEditorMarkup() {
    // Same watcher as the scene -- creating it lazily so headless
    // tests that never enter the editor's Update loop don't pay for
    // it. Re-Watch is a refresh in the FileWatcher, so calling this
    // again after a reload is correct.
    if (!watcher_) {
        watcher_ = std::make_unique<FileWatcher>();
        watcher_->SetCallback([this](const std::string& p) {
            HandleFileChanged(p);
        });
    }
    watcher_->Unwatch(kMarkupKmlPath);
    watcher_->Unwatch(kMarkupKssPath);
    watcher_->Watch(kMarkupKmlPath);
    watcher_->Watch(kMarkupKssPath);
}

void EditorModule::ReloadEditorMarkup() {
    if (!active_ || !kernel_) return;
    if (reloadingMarkup_) return;
    reloadingMarkup_ = true;

    UI* ui = GetUI(*kernel_);
    if (!ui) {
        reloadingMarkup_ = false;
        return;
    }

    // Tear down the editor's UI subtree only -- preserve any other
    // markup the host may have loaded. UIContext::SetCommandRegistry
    // is detached so the about-to-be-recreated registry doesn't leave
    // a dangling pointer behind.
    ui->Context().SetCommandRegistry(nullptr);
    saveShortcutInstalled_ = false;
    for (auto& row : hierarchyRows_) {
        if (row.widgetIdx >= 0) ui->Context().RemoveTreeNode(row.widgetIdx);
    }
    hierarchyRows_.clear();
    if (editorRoot_ >= 0) {
        ui->DestroyWidget(editorRoot_);
        editorRoot_ = -1;
    }
    hierarchyScrollIdx_  = -1;
    hierarchyEmptyIdx_   = -1;
    hierarchyCountIdx_   = -1;
    viewportIdx_         = -1;
    inspectorScrollIdx_  = -1;
    inspectorContentIdx_ = -1;
    playButtonIdx_  = -1;
    pauseButtonIdx_ = -1;
    stopButtonIdx_  = -1;
    statusTextIdx_  = -1;

    if (!LoadEditorMarkupAndWire()) {
        KL_ERR("editor", "Markup reload failed -- editor is in a broken "
                         "state until the file is fixed and re-saved.");
        reloadingMarkup_ = false;
        return;
    }

    RefreshPlayStateUI();
    // Re-arm markup watch so the new file mtime is the baseline.
    WatchEditorMarkup();
    KL_LOG("editor", "Editor markup reloaded");
    reloadingMarkup_ = false;
}

bool PreRegisterEditorModule(KoiloKernel& kernel) {
    auto* loader = GetLoader(kernel);
    if (!loader) {
        KL_ERR("editor", "Script engine / module loader unavailable");
        return false;
    }
    if (loader->HasModule(kModuleName)) {
        KL_WARN("editor", "Editor module is already registered");
        return false;
    }
    loader->Register(std::make_unique<EditorModule>());
    KL_LOG("editor", "Editor module pre-registered; will initialize during display boot");
    return true;
}

bool RegisterEditorModule(KoiloKernel& kernel) {
    auto* loader = GetLoader(kernel);
    if (!loader) {
        KL_ERR("editor", "Script engine / module loader unavailable");
        return false;
    }
    if (loader->HasModule(kModuleName)) {
        KL_WARN("editor", "Editor module is already loaded");
        return false;
    }

    auto mod = std::make_unique<EditorModule>();
    auto* raw = mod.get();
    loader->Register(std::move(mod));

    if (!raw->Initialize(kernel)) {
        // Initialize failed: roll back the registration so we don't
        // leave a half-initialized module hanging around.
        loader->UnloadModule(kModuleName);
        return false;
    }
    return true;
}

bool UnregisterEditorModule(KoiloKernel& kernel) {
    auto* loader = GetLoader(kernel);
    if (!loader) return false;
    if (!loader->HasModule(kModuleName)) return false;
    return loader->UnloadModule(kModuleName);
}

// -- Console command -------------------------------------------------

void RegisterEditorCommands(CommandRegistry& registry) {
    registry.Register({
        "editor",
        "editor <load|unload|status>",
        "Manage the editor module at runtime. "
        "`load` attaches the editor shell on top of the running UI; "
        "`unload` detaches it; `status` reports whether it is active.",
        [](KoiloKernel& kernel, const std::vector<std::string>& args)
            -> ConsoleResult {
            const std::string sub = args.empty() ? "status" : args[0];

            if (sub == "load") {
                if (RegisterEditorModule(kernel))
                    return ConsoleResult::Ok("editor module loaded");
                return ConsoleResult::Error(
                    "failed to load editor module (already loaded or "
                    "UI not ready)");
            }
            if (sub == "unload") {
                if (UnregisterEditorModule(kernel))
                    return ConsoleResult::Ok("editor module unloaded");
                return ConsoleResult::Error(
                    "editor module is not currently loaded");
            }
            if (sub == "status") {
                auto* loader = GetLoader(kernel);
                bool loaded = loader && loader->HasModule(kModuleName);
                return ConsoleResult::Ok(loaded ? "editor: loaded"
                                                : "editor: not loaded");
            }
            return ConsoleResult::Error(
                "unknown subcommand; use load|unload|status");
        }
    });
}

} // namespace koilo::editor
