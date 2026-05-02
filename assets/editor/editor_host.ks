// editor_host.ks - Bootstrap script for `koilo --editor`
// =======================================================
// This script intentionally does nothing visible. It only
// configures the host window. The EditorModule (registered
// by the runner under -DKL_BUILD_EDITOR=ON) owns the actual
// editor.kml/kss markup and all editor behavior.
//
// Users normally launch this via:
//
//   koilo --editor
//
// rather than running it directly. Without the editor module
// registered, you'll get an empty desktop window.
// =======================================================

// -- Display Configuration ----------------------------
display.SetType("desktop");
display.SetWidth(1280);
display.SetHeight(720);
display.SetPixelWidth(1280);
display.SetPixelHeight(720);
display.SetTargetFPS(60);
display.SetCapFPS(true);

fn Setup() {
    // EditorModule::Initialize() loads assets/ui/editor.{kml,kss}
    // for us. Drop a couple of demo SceneNodes in so the live
    // hierarchy panel has something to show (Phase E3.1).
    var root  = scene.CreateObject("Root");
    var camN  = scene.CreateObject("MainCamera");
    var lightN = scene.CreateObject("DirectionalLight");
    var ground = scene.CreateObject("Ground");
    var player = scene.CreateObject("Player");
    var pmesh  = scene.CreateObject("PlayerMesh");
    camN.SetParent(root);
    lightN.SetParent(root);
    ground.SetParent(root);
    player.SetParent(root);
    pmesh.SetParent(player);

    // Phase E3.2: attach a quad mesh to PlayerMesh so the viewport
    // picking has something to hit.  Also push it forward into the
    // default camera's view (camera sits at (0,0,8) looking down -Z).
    var prim = PrimitiveMesh();
    prim.CreateQuad(2.0, 2.0);
    pmesh.SetMesh(prim.GetMesh());
    pmesh.SetPosition(Vector3D(0.0, 0.0, 0.0));

    scene.BumpHierarchyGeneration();
}

fn Update(dt) {
    // EditorModule::Update() drives editor state.
}
