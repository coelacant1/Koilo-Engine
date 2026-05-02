// kscene_demo.ks
// =====================================================================
// Minimal example demonstrating the .kscene file format (Phase E2).
// The static world (ground + 2 walls) lives in an authored .kscene
// file; the runner script just bolts on the runtime wiring (physics
// world, gravity, body registration) and a hot-reload loop.
//
// Tip: edit assets/scenes/empty_room.kscene and reset (r key in the
// physics demo, or `Reload()` from the editor) - changes apply
// without touching this script.
// =====================================================================

display.SetType("desktop");
display.SetWidth(1280);
display.SetHeight(720);
display.SetPixelWidth(1280);
display.SetPixelHeight(720);
display.SetTargetFPS(60);
display.SetCapFPS(true);

// Pull in the authored scene. Variables declared in the .kscene
// (groundBody, wallNBody, wallSBody, ...) are now visible here.
import "../../assets/scenes/empty_room.kscene"

fn Setup() {
    physics.SetGravity(Vector3D(0.0, 0.0 - 9.81, 0.0));
    physics.SetFixedTimestep(1.0 / 120.0);
    physics.SetMaxSubSteps(4);

    physics.AddBody(groundBody);
    physics.AddBody(wallNBody);
    physics.AddBody(wallSBody);

    cam.SetPerspective(60.0, 0.1, 500.0);
    cam.SetBackfaceCulling(true);
    debug.Enable();

    print("=== KoiloEngine kscene_demo ===");
    print("Loaded scene: empty_room.kscene");
}

fn Update(dt) {
    // Just visualize the imported geometry.
    debug.DrawBox(Vector3D(0.0, 0.0 - 0.5, 0.0),
                  Vector3D(40.0, 0.5, 40.0),
                  Color(0.30, 0.55, 0.30, 1.0), 0.0, true, 0);
    debug.DrawBox(Vector3D(0.0, 5.0, 40.0),
                  Vector3D(40.0, 5.0, 0.5),
                  Color(0.50, 0.50, 0.70, 1.0), 0.0, true, 0);
    debug.DrawBox(Vector3D(0.0, 5.0, 0.0 - 40.0),
                  Vector3D(40.0, 5.0, 0.5),
                  Color(0.50, 0.50, 0.70, 1.0), 0.0, true, 0);
    debug.DrawGrid(Vector3D(0.0, 0.0, 0.0), 80.0, 40,
                   Color(0.18, 0.18, 0.20, 1.0), 0.0, true);
    debug.DrawAxes(Vector3D(0.0, 0.0, 0.0), 5.0, 0.0, true);
}
