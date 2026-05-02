// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file editor_module.hpp
 * @brief Unity-style editor shell as an opt-in IModule.
 *
 * Compiled only when KL_BUILD_EDITOR=ON. The module is NOT registered
 * by RegisterDefaultModules; instead it is opted in via either:
 *   - the `--editor` runner flag (registered before display launch), or
 *   - the `editor load` console command (registered at runtime).
 *
 * On Initialize() it locates the UIModule via kernel services and asks
 * it to load `assets/ui/editor.kml` + `assets/ui/editor.kss`. Panel
 * wiring (hierarchy / inspector / viewport / play controls / console)
 * is fleshed out in Phases E3+; this skeleton just brings up the
 * markup so we can verify the shell renders end-to-end.
 *
 * Shutdown() tears down editor-owned UI state so the runtime keeps
 * running cleanly when the editor is unloaded mid-session.
 *
 * @date 04/2026
 */
#pragma once

#include <koilo/kernel/unified_module.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <koilo/systems/ui/selection.hpp>
#include <koilo/systems/ui/command_registry.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/quaternion.hpp>
#include <koilo/core/platform/file_watcher.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace koilo {
class Scene;
class SceneNode;
class CameraBase;
class RigidBody;
namespace rhi { class RenderPipeline; }
} // namespace koilo

namespace koilo::editor {

class EditorModule : public IModule {
public:
    EditorModule();
    ~EditorModule() override;

    ModuleInfo GetInfo() const override;
    bool Initialize(KoiloKernel& kernel) override;
    void Update(float dt) override;
    void Shutdown() override;

    /// True once Initialize() has successfully attached editor UI.
    bool IsActive() const { return active_; }

private:
    /// One row in the rebuilt hierarchy tree; lets us release click
    /// callbacks and tree nodes cleanly on the next rebuild.
    struct HierarchyRow {
        int          widgetIdx = -1;   ///< UIContext widget index.
        SceneNode*   node      = nullptr;
    };

    /// Rebuild the hierarchy panel from the live Scene. Cheap-noop
    /// when the scene's HierarchyGeneration() hasn't moved.
    void RefreshHierarchy();

    /// Called by SelectionManager whenever selection changes.
    /// Updates per-row "selected" highlight state.
    void OnSelectionChanged();

    /// Recursive helper: append `node` and its descendants to the
    /// hierarchy tree under `scrollIdx`, then wire each row's click
    /// handler to Selection::Select.
    void AppendNodeRow(int scrollIdx, SceneNode* node, int depth);

    /// Refresh the viewport widget's texture binding to point at the
    /// pipeline's current offscreen color target.  The id can change
    /// across resize, so this is called every Update().
    void RefreshViewportTexture();

    /// Click-in-viewport handler: builds a pick ray and selects the
    /// nearest hit SceneNode (or clears selection on miss).
    void HandleViewportClick();

    /// Submit a debug-line OBB around the currently-selected SceneNode
    /// (if any and if it has a mesh).  Re-queued every Update() because
    /// DebugDraw's queue drains after each frame.
    void RefreshSelectionGizmo();

    /// Tear down + regenerate the inspector subtree for the current
    /// selection (called from OnSelectionChanged).  No-op if the
    /// inspector container isn't present in markup.
    void RebuildInspector();

    /// Drop any placeholder children of `inspector-scroll` that came
    /// from the static markup so we have a clean container to populate.
    void StripInspectorPlaceholder();

    // -- Editor camera --------------------
    /// Wire viewport pointer/scroll callbacks to the orbit/pan/zoom
    /// camera handlers. Called once after Initialize finds the
    /// viewport widget.
    void InstallEditorCameraHandlers();

    /// Initialize orbit/pan/zoom state from the camera's current
    /// position so the editor doesn't snap when the user first drags.
    void SyncEditorCameraFromCamera();

    /// Recompute orbitTarget_ + orbitDistance_ + yaw/pitch into the
    /// scene Camera's Transform.
    void ApplyEditorCameraToScene();

    // -- Dirty tracking + save -----------------------------
    /// Mark the scene as dirty (called from inspector edit callbacks
    /// + transform widget). Updates the host window title to show a
    /// trailing '*'.
    void MarkSceneDirty();

    /// Reset the dirty flag (typically after a successful save) and
    /// refresh the host window title.
    void ClearSceneDirty();

    /// Refresh the host window title to "<scene>[*] - Koilo Editor".
    /// No-op if no host window is present.
    void RefreshWindowTitle();

    /// Write the current Scene back to scenePath_ (or, if unset, to a
    /// fallback path under assets/scenes/). Returns true on success.
    /// Hooked to Ctrl+S and the toolbar Save button.
    bool SaveSceneNow();

    /// Install global key handler that maps Ctrl+S -> SaveSceneNow().
    void InstallSaveShortcut();

    // -- Play state machine --------------------------------
public:
    /// Editor simulation lifecycle. `Editing` is the default authoring
    /// state; physics/aero are gated off so the user can poke values
    /// without the world drifting. `Playing` runs the normal Step loop.
    /// `Paused` keeps render/UI but stops stepping (rigid bodies frozen
    /// in their current pose).
    enum class PlayState { Editing, Playing, Paused };

    /// Current play state (defaults to Editing while the editor is
    /// loaded). Reading is cheap; transitions go through SetPlayState.
    PlayState GetPlayState() const { return playState_; }

    /// Transition into a new play state. Idempotent: same-state calls
    /// are dropped. Side-effects:
    ///   - Toggles PhysicsModule::SetSimulationEnabled(state == Playing).
    ///   - Logs the transition.
    /// Snapshot/restore (E5a/E5c) hooks will hang off this method.
    void SetPlayState(PlayState next);

    /// Convenience wrappers used by the toolbar / commands.
    void Play()  { SetPlayState(PlayState::Playing); }
    void Pause() { SetPlayState(PlayState::Paused);  }
    void Stop()  { SetPlayState(PlayState::Editing); }

private:
    /// Apply the simulation-enabled gate that matches `playState_` to
    /// the live PhysicsModule. Called from SetPlayState and from
    /// Initialize once the kernel is wired (so editor boot defaults to
    /// Editing = simulation off).
    void ApplyPlayStateToWorld();

    /// Register editor.play / editor.pause / editor.stop in the owned
    /// command registry. Called from Initialize once `commands_` is up.
    void InstallPlayCommands();

    /// Bind the toolbar #btn-play/#btn-pause/#btn-stop widgets to
    /// Play/Pause/Stop. Cached widget indices live in playButtonIdx_
    /// etc. so RefreshPlayStateUI can update labels without re-finding.
    void InstallPlayToolbar();

    /// Update the status bar (#status-text) and toolbar button labels
    /// to reflect the current playState_. Called after every transition.
    void RefreshPlayStateUI();

    // -- snapshot + restore -----------------------------
    /// Capture every SceneNode's local TRS plus every PhysicsWorld
    /// rigid-body's pose + linear/angular velocity into the snapshot
    /// buffers. Called once when entering Playing from Editing.
    void CaptureScenarioSnapshot();

    /// Walk the saved buffers and write values back onto the live
    /// nodes/bodies. Marks each node dirty so world transforms
    /// recompute. Called when transitioning back to Editing.
    void RestoreScenarioSnapshot();

    /// Drop snapshot buffers (e.g. on Shutdown or when the scene
    /// graph mutates structurally and stale node pointers would be
    /// dangerous to write back to).
    void ClearScenarioSnapshot();

    /// Walk all roots and append visit order to `out`. Used by
    /// CaptureScenarioSnapshot / RestoreScenarioSnapshot so the two
    /// passes traverse identically.
    void CollectAllNodes(std::vector<SceneNode*>& out) const;

    // -- file watcher ---------------------------------------
    /// Drain the file watcher (tick called from Update on a 0.5s
    /// cadence so we don't stat the disk every frame).
    void PollFileWatcher(float dt);

    /// Begin watching `path` for external modifications. No-op on
    /// empty paths.
    void WatchScenePath(const std::string& path);

    /// Watcher callback: file at `path` changed on disk. Implements
    /// the E6c save-debounce (drops events within savePollDebounce_
    /// seconds of our own SaveSceneNow) and routes surviving events
    /// to OnSceneFileChanged. Re-arm the path to clear stale mtime.
    void HandleFileChanged(const std::string& path);

    /// Reaction to a confirmed external change to the open scene
    /// file. Logs + (when not dirty) clears the in-memory scene's
    /// generation counter so the hierarchy panel will redraw on
    /// next tick. Full reload (re-execute .kscene) is E6b territory.
    void OnSceneFileChanged(const std::string& path);

    // -- scene reload ---------------------------------------
    /// Re-execute the loaded `.kscene` (== currently active script) by
    /// delegating to `KoiloScriptEngine::Reload()`. Clears editor-side
    /// state that holds raw pointers into the about-to-be-rebuilt
    /// scene (selection, snapshots, hierarchy cache) before the reload
    /// and rewires post-reload UI state (inspector, gizmo, watcher).
    ///
    /// Behavior is conservative: only triggers Reload() when the
    /// editor's watched scenePath_ matches the script engine's
    /// `GetCurrentScriptPath()`. Returns true on success; false if no
    /// script engine is reachable, paths don't match, or Reload()
    /// itself failed (script engine restores prior state on failure).
    bool ReloadSceneFromDisk();

    /// Wires `editor.reload` into the owned command registry. Called
    /// once from Initialize alongside the other editor commands so the
    /// user can trigger a manual reload from the console (covers the
    /// "Reload from disk" branch of the E6b conflict prompt until a
    /// real modal UI lands).
    void InstallReloadCommand();

    // -- editor markup hot-reload ---------------------------
    /// Begin watching `assets/ui/editor.kml` and `editor.kss` for
    /// external edits. Called once from Initialize after the markup
    /// has been loaded; safe to call again - re-Watch refreshes the
    /// mtime baseline.
    void WatchEditorMarkup();

    /// React to a confirmed change to the editor's KML or KSS file by
    /// tearing down the currently loaded UI and re-running the markup
    /// + wiring path. Editor selection state is preserved across the
    /// reload (the SelectionManager isn't owned by the UI subtree),
    /// but cached widget indices are invalidated and re-resolved.
    void ReloadEditorMarkup();

    /// Common helper used by both Initialize and ReloadEditorMarkup:
    /// loads `assets/ui/editor.kml` + `editor.kss`, refreshes the
    /// cached widget indices, strips the inspector placeholder, and
    /// re-installs viewport / save / play / reload wiring. Returns
    /// false if the markup failed to load (caller logs).
    bool LoadEditorMarkupAndWire();


    bool                            active_              = false;
    int                             editorRoot_          = -1;
    int                             hierarchyScrollIdx_  = -1;
    int                             hierarchyEmptyIdx_   = -1;
    int                             hierarchyCountIdx_   = -1;
    int                             viewportIdx_         = -1;
    int                             inspectorScrollIdx_  = -1;
    int                             inspectorContentIdx_ = -1;

    // -- Editor camera state -------------------------------------------
    bool   editorCameraSynced_ = false; ///< First sync from scene cam.
    float  orbitYaw_     = 0.0f;        ///< Around world-up (radians).
    float  orbitPitch_   = 0.0f;        ///< Above the orbit plane (radians).
    float  orbitDistance_ = 5.0f;       ///< Distance from target.
    float  orbitTargetX_ = 0.0f;        ///< Stored individually because
    float  orbitTargetY_ = 0.0f;        ///< Vector3D isn't trivially
    float  orbitTargetZ_ = 0.0f;        ///< default-constructible here.
    int    dragButton_   = -1;          ///< 0=L,1=R,2=M; -1 = no drag.
    float  prevPointerX_ = 0.0f;
    float  prevPointerY_ = 0.0f;
    std::uint64_t                   lastHierarchyGen_    = 0;
    bool                            hierarchyEverBuilt_  = false;
    std::vector<HierarchyRow>       hierarchyRows_;

    /// Editor-owned selection state. Registered into the kernel
    /// service registry as "selection" so other panels (inspector,
    /// viewport) can subscribe to the same instance.
    std::unique_ptr<SelectionManager> selection_;
    SelectionListenerToken            selectionToken_ = 0;

    /// Cached pipeline pointer; resolved lazily because the render
    /// backend service may not be registered before Initialize() runs
    /// (the editor module loads early in Overlay phase).
    rhi::RenderPipeline*              pipeline_       = nullptr;

    std::string scenePath_; ///< Optional: --editor <scene>.
    bool        sceneDirty_     = false; ///< E4c: edits since last save.
    bool        saveShortcutInstalled_ = false;
    /// Owned command registry for editor shortcuts (Ctrl+S etc.).
    /// Attached to UIContext via SetCommandRegistry on Initialize and
    /// detached on Shutdown so a re-load doesn't leak across.
    std::unique_ptr<ui::CommandRegistry> commands_;

    /// Current play state. Editing on boot so physics doesn't
    /// step until the user clicks Play.
    PlayState playState_ = PlayState::Editing;

    /// Cached toolbar widget indices so play-state transitions
    /// can refresh button labels without re-searching the tree.
    int playButtonIdx_   = -1;
    int pauseButtonIdx_  = -1;
    int stopButtonIdx_   = -1;
    int statusTextIdx_   = -1;

    // -- E5a/E5c snapshot buffers --------------------------------------
    /// One entry per SceneNode visited at snapshot time. Captures the
    /// fields the inspector / scripts can mutate during Play.
    struct NodeSnapshot {
        SceneNode* node = nullptr;   ///< Identity (raw - Scene owns it).
        Vector3D   position;
        Quaternion rotation;
        Vector3D   scale;
    };
    /// One entry per RigidBody. Pose + linear/angular velocity is the
    /// minimum needed to re-run a deterministic physics scenario.
    struct BodySnapshot {
        RigidBody* body = nullptr;
        Vector3D   position;
        Quaternion orientation;
        Vector3D   linearVelocity;
        Vector3D   angularVelocity;
    };
    std::vector<NodeSnapshot> nodeSnapshot_;
    std::vector<BodySnapshot> bodySnapshot_;
    bool                      hasSnapshot_ = false;

    // -- E6a file watcher state ----------------------------------------
    /// Owned watcher so a re-load cleans up its mtime cache.
    std::unique_ptr<FileWatcher> watcher_;
    /// Frame-time accumulator. Watcher polls once cadence threshold
    /// is met so we don't stat() every frame.
    float watcherAccum_ = 0.0f;
    /// Cadence in seconds between FileWatcher::Poll calls.
    static constexpr float kWatcherPollInterval = 0.5f;
    /// Window after a SaveSceneNow during which incoming watcher
    /// events for our own path are ignored (E6c self-trigger guard).
    static constexpr float kSaveSelfTriggerWindow = 1.5f;
    /// Monotonic time of the last successful self-save; -1 = none.
    /// Compared against a frame-driven clock (savePollClock_) so we
    /// don't pull in <chrono> just for this guard.
    float lastSaveClock_  = -1.0f;
    /// Monotonic clock advanced every Update by dt.
    float savePollClock_ = 0.0f;

    /// Watched markup paths for E6d (editor.kml / editor.kss).
    static constexpr const char* kMarkupKmlPath = "assets/ui/editor.kml";
    static constexpr const char* kMarkupKssPath = "assets/ui/editor.kss";
    /// True while ReloadEditorMarkup is mid-flight, so a watcher
    /// callback fired by us re-Watching the path during reload doesn't
    /// recurse.
    bool reloadingMarkup_ = false;

    KL_BEGIN_FIELDS(EditorModule)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(EditorModule)
        KL_METHOD_AUTO(EditorModule, GetInfo,  "Get info"),
        KL_METHOD_AUTO(EditorModule, Update,   "Update"),
        KL_METHOD_AUTO(EditorModule, Shutdown, "Shutdown"),
        KL_METHOD_AUTO(EditorModule, IsActive, "Editor active flag")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(EditorModule)
        KL_CTOR0(EditorModule)
    KL_END_DESCRIBE(EditorModule)
};

/// Register an EditorModule with the kernel's script-engine module
/// loader AND immediately initialize it. Used by the `editor load`
/// console command after a display session is already running and the
/// UI module is up. Returns false if the editor module is already
/// loaded or initialization failed (e.g. UI module unavailable).
bool RegisterEditorModule(KoiloKernel& kernel);

/// Register an EditorModule with the script-engine module loader
/// WITHOUT initializing it. Used by the `--editor` runner path: the
/// editor is added before the display session starts so that
/// ModuleLoader::InitializeAll() (called from BuildScene) brings up
/// the editor naturally, after the UI module. Returns false if the
/// editor module is already registered.
bool PreRegisterEditorModule(KoiloKernel& kernel);

/// Unload an editor module previously registered via RegisterEditorModule.
/// Returns true if a module was found and shut down.
bool UnregisterEditorModule(KoiloKernel& kernel);

/// Register the `editor` console command (load / unload / status).
} // namespace koilo::editor

namespace koilo {
class CommandRegistry;
namespace editor {
void RegisterEditorCommands(CommandRegistry& registry);
} // namespace editor
} // namespace koilo
