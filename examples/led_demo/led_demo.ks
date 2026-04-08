// =============================================================================
// KoiloEngine LED Volume Demo - Artisans Hub + Cynder on ProtoDR panel
// Orbits camera around Cynder for a recognizable LED view
// =============================================================================

display.SetWidth(128);
display.SetHeight(128);
display.SetPixelWidth(128);
display.SetPixelHeight(128);
display.SetTargetFPS(120);
display.SetCapFPS(true);

var canvas = Canvas2D();
canvas.Resize(64, 64);
canvas.Scale(128, 128);
canvas.Attach();

// --- Textures ---
var texMap = Texture();
var texCynder = Texture();

// --- Materials ---
var matMap = KSLMaterial("texture");
var matCynder = KSLMaterial("texture");
var matSky = KSLMaterial("sky");

// --- Meshes ---
var mapLoader = MorphableMesh();
var cynderLoader = MorphableMesh();
var mapMesh = 0;
var cynderMesh = 0;

// Camera state
var camX = 200.0;
var camY = -50.0;
var camZ = -400.0;
var yaw = 180.0;
var pitch = -5.0;
var time_acc = 0.0;

fn Setup() {
    if (not texMap.LoadFile("assets/textures/artisans.ktex")) {
        print("WARNING: Failed to load artisans texture");
    }
    if (not texCynder.LoadFile("assets/textures/cynder.ktex")) {
        print("WARNING: Failed to load cynder texture");
    }

    if (not mapLoader.Load("assets/kmesh/artisans.kmesh")) {
        print("ERROR: Failed to load artisans map");
        return;
    }
    if (not cynderLoader.Load("assets/kmesh/cynder.kmesh")) {
        print("ERROR: Failed to load cynder");
        return;
    }

    mapMesh = mapLoader.GetMesh();
    cynderMesh = cynderLoader.GetMesh();

    matMap.SetTexture(0, texMap);
    matMap.AddLight();
    matMap.SetLightPosition(0, 50.0, 500.0, 50.0);
    matMap.SetLightColor(0, 1.0, 0.95, 0.85);
    matMap.SetLightIntensity(0, 5000.0);
    matMap.SetLightFalloff(0, 8000.0);

    matCynder.SetTexture(0, texCynder);
    matCynder.AddLight();
    matCynder.SetLightPosition(0, 50.0, 500.0, 50.0);
    matCynder.SetLightColor(0, 1.0, 0.95, 0.85);
    matCynder.SetLightIntensity(0, 5000.0);
    matCynder.SetLightFalloff(0, 8000.0);

    var cynderTf = Transform(Vector3D(0.0, 0.0, 0.0), Vector3D(0.0, 0.0, 0.0), Vector3D(20.0, 20.0, 20.0));
    cynderMesh.SetMaterial(matCynder);
    cynderMesh.SetTransform(cynderTf);
    cynderMesh.ResetVertices();
    cynderMesh.UpdateTransform();

    var mapTf = Transform(Vector3D(0.0, 0.0, 0.0), Vector3D(0.0, -150.0, 0.0), Vector3D(10.0, 10.0, 10.0));
    mapMesh.SetMaterial(matMap);
    mapMesh.SetTransform(mapTf);
    mapMesh.ResetVertices();
    mapMesh.UpdateTransform();

    scene.AddMesh(mapMesh);
    scene.AddMesh(cynderMesh);

    cam.SetPerspective(60.0, 0.1, 10000.0);
    cam.SetBackfaceCulling(true);

    sky.SetMaterial(matSky);
    sky.Enable();
    sky.SetTimeOfDay(10.0);
    sky.SetTimeSpeed(0.3);

    print("=== LED Volume Demo ===");
    print("Camera orbiting Cynder at origin");
    print("Framebuffer: 128x128 (matched to 64x64 LED panel)");
}

fn Update(dt) {
    time_acc = time_acc + dt;

    // Gentle orbit around Cynder
    var orbit_radius = 400.0;
    var orbit_speed = 0.15;
    camX = orbit_radius * sin(time_acc * orbit_speed);
    camZ = -orbit_radius * cos(time_acc * orbit_speed);
    camY = -50.0 + 30.0 * sin(time_acc * 0.3);

    yaw = -(time_acc * orbit_speed) * 57.2958 + 180.0;

    var camPos = Vector3D(camX, camY, camZ);
    cam.GetTransform().SetPosition(camPos);

    var yawTf = Transform(Vector3D(0.0, yaw, 0.0), Vector3D(0.0, 0.0, 0.0), Vector3D(1.0, 1.0, 1.0));
    var yawQ = yawTf.GetRotation();
    var pitchTf = Transform(Vector3D(pitch, 0.0, 0.0), Vector3D(0.0, 0.0, 0.0), Vector3D(1.0, 1.0, 1.0));
    var pitchQ = pitchTf.GetRotation();
    cam.GetTransform().SetRotation(yawQ.Multiply(pitchQ));

    matMap.SetLightPosition(0, camX, camY + 300.0, camZ);
    matMap.SetCameraPosition(camPos);
    matCynder.SetLightPosition(0, camX, camY + 300.0, camZ);
    matCynder.SetCameraPosition(camPos);

    canvas.Clear();
    canvas.DrawText(2, 2, "HUB75 Demo", 100, 255, 100);
    canvas.DrawText(2, 10, str(floor(fps)) + " fps", 180, 180, 180);
}
