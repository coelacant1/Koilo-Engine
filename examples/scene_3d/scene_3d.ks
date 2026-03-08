// =============================================================================
// KoiloEngine 3D Material Exhibition - Spinning Cube
// Demonstrates: perspective projection, orbiting camera, 6 material faces,
//               backend engine math (Transform, Quaternion), HUD overlay
// =============================================================================

display.SetType("desktop");
display.SetWidth(1920);
display.SetHeight(1080);
display.SetPixelWidth(1920);
display.SetPixelHeight(1080);
display.SetTargetFPS(60);
display.SetCapFPS(false);

var canvas = Canvas2D();
canvas.Resize(192, 108);
canvas.Scale(1920, 1080);
canvas.Attach();

// --- Materials (one per cube face, all KSLMaterial) ---
var matPhong    = KSLMaterial("phong");
var matNoise    = KSLMaterial("procedural_noise");
matNoise.SetRainbowPalette("colors", 4);
var matSpiral   = KSLMaterial("spiral");
matSpiral.SetRainbowPalette("colors", 6);
matSpiral.SetFloat("width", 0.15);
matSpiral.SetFloat("bend", 1.5);
var matRainbow  = KSLMaterial("horizontal_rainbow");
matRainbow.SetRainbowPalette("colors", 6);
var matGradient = KSLMaterial("gradient");
matGradient.SetRainbowPalette("colors", 6);
var matSolid    = KSLMaterial("uniform_color");
matSolid.SetColor("color", Color888(40, 180, 255));

// Sky material (time-of-day auto-updates topColor/bottomColor)
var matSky = KSLMaterial("sky");

// --- 6 quads - cube faces ---
var prim0 = PrimitiveMesh();
var prim1 = PrimitiveMesh();
var prim2 = PrimitiveMesh();
var prim3 = PrimitiveMesh();
var prim4 = PrimitiveMesh();
var prim5 = PrimitiveMesh();

var m0 = 0; var m1 = 0; var m2 = 0;
var m3 = 0; var m4 = 0; var m5 = 0;

// State
var cubeSize   = 18.0;
var halfSize   = 9.0;
var spinAngle  = 0.0;
var orbitAngle = 0.0;
var orbitDist  = 65.0;
var orbitSpeed = 25.0;
var spinSpeed  = 40.0;
var hueOffset  = 0.0;
var paused     = false;
var showHud    = true;

// HSL hue rotation: shift base RGB color by hueDeg degrees
fn HueRotate(r, g, b, hueDeg) {
    var rr = r / 255.0;
    var gg = g / 255.0;
    var bb = b / 255.0;
    var angle = hueDeg * 3.14159265 / 180.0;
    var cosA = cos(angle);
    var sinA = sin(angle);
    var k = 0.57735026919;
    var oneMinusCos = 1.0 - cosA;
    var rOut = rr * (cosA + k * k * oneMinusCos) + gg * (k * k * oneMinusCos - k * sinA) + bb * (k * k * oneMinusCos + k * sinA);
    var gOut = rr * (k * k * oneMinusCos + k * sinA) + gg * (cosA + k * k * oneMinusCos) + bb * (k * k * oneMinusCos - k * sinA);
    var bOut = rr * (k * k * oneMinusCos - k * sinA) + gg * (k * k * oneMinusCos + k * sinA) + bb * (cosA + k * k * oneMinusCos);
    if (rOut < 0.0) { rOut = 0.0; }
    if (rOut > 1.0) { rOut = 1.0; }
    if (gOut < 0.0) { gOut = 0.0; }
    if (gOut > 1.0) { gOut = 1.0; }
    if (bOut < 0.0) { bOut = 0.0; }
    if (bOut > 1.0) { bOut = 1.0; }
    return Color888(floor(rOut * 255.0), floor(gOut * 255.0), floor(bOut * 255.0));
}

fn Setup() {
    // Create 6 quads
    prim0.CreateQuad(cubeSize, cubeSize);
    prim1.CreateQuad(cubeSize, cubeSize);
    prim2.CreateQuad(cubeSize, cubeSize);
    prim3.CreateQuad(cubeSize, cubeSize);
    prim4.CreateQuad(cubeSize, cubeSize);
    prim5.CreateQuad(cubeSize, cubeSize);

    m0 = prim0.GetMesh(); m1 = prim1.GetMesh(); m2 = prim2.GetMesh();
    m3 = prim3.GetMesh(); m4 = prim4.GetMesh(); m5 = prim5.GetMesh();

    // Assign materials
    m0.SetMaterial(matPhong);
    m1.SetMaterial(matNoise);
    m2.SetMaterial(matSpiral);
    m3.SetMaterial(matRainbow);
    m4.SetMaterial(matGradient);
    m5.SetMaterial(matSolid);

    // Add all to scene
    scene.AddMesh(m0); scene.AddMesh(m1); scene.AddMesh(m2);
    scene.AddMesh(m3); scene.AddMesh(m4); scene.AddMesh(m5);

    // Camera
    cam.SetPerspective(60.0, 0.1, 500.0);
    cam.SetBackfaceCulling(true);

    // Phong light setup
    matPhong.SetColor("ambientColor", Color888(30, 30, 40));
    matPhong.SetColor("diffuseColor", Color888(180, 160, 220));
    matPhong.SetColor("specularColor", Color888(255, 255, 255));
    matPhong.SetFloat("shininess", 32.0);

    // Position light above the scene
    matPhong.AddLight();
    matPhong.SetLightPosition(0, 50.0, 80.0, 50.0);
    matPhong.SetLightColor(0, 1.0, 1.0, 1.0);
    matPhong.SetLightIntensity(0, 255.0);
    matPhong.SetLightFalloff(0, 200.0);

    // Noise material setup
    matNoise.SetVec3("noiseScale", 4.0, 4.0, 4.0);
    matNoise.SetFloat("simplexDepth", 3.0);

    // Rainbow scroll
    matRainbow.SetFloat("gradientPeriod", 2.0);

    print("=== KoiloEngine 3D Spinning Cube ===");
    print("6 materials · orbiting camera · euler transforms");

    // Enable debug drawing (grid + origin gizmo)
    debug.Enable();

    // Enable sky with time-of-day cycle via KSL sky material
    sky.SetMaterial(matSky);
    sky.Enable();
    sky.SetTimeOfDay(8.0);
    sky.SetTimeSpeed(0.5);
}

fn ApplyFace(mesh, spinQ, faceEulerX, faceEulerY, faceEulerZ, offX, offY, offZ) {
    var unit = Vector3D(1.0, 1.0, 1.0);
    var tmpTf = Transform(Vector3D(faceEulerX, faceEulerY, faceEulerZ), Vector3D(0.0, 0.0, 0.0), Vector3D(1.0, 1.0, 1.0));
    var faceQ = tmpTf.GetRotation();
    var totalQ = spinQ.Multiply(faceQ);
    var offset = spinQ.RotateVector(Vector3D(offX, offY, offZ));
    var t = Transform(Vector3D(0.0, 0.0, 0.0), offset, unit);
    t.SetRotation(totalQ);
    mesh.SetTransform(t);
    mesh.ResetVertices();
    mesh.UpdateTransform();
}

fn Update(dt) {
    if (not paused) {
        spinAngle  = spinAngle  + spinSpeed  * dt;
        orbitAngle = orbitAngle + orbitSpeed  * dt;
        hueOffset  = hueOffset + 15.0 * dt;
    }

    // Cube spin quaternion from euler angles
    var tmpTf = Transform(Vector3D(spinAngle * 0.3, spinAngle, 0.0), Vector3D(0.0, 0.0, 0.0), Vector3D(1.0, 1.0, 1.0));
    var spinQ = tmpTf.GetRotation();

    // Position 6 cube faces: (mesh, spinQ, faceEulerXYZ, offsetXYZ)
    ApplyFace(m0, spinQ,    0.0,    0.0, 0.0,  0.0,  0.0,  halfSize);
    ApplyFace(m1, spinQ,    0.0,  180.0, 0.0,  0.0,  0.0, 0.0 - halfSize);
    ApplyFace(m2, spinQ,    0.0,   90.0, 0.0,  halfSize,  0.0,  0.0);
    ApplyFace(m3, spinQ,    0.0,  -90.0, 0.0, 0.0 - halfSize,  0.0,  0.0);
    ApplyFace(m4, spinQ,  -90.0,    0.0, 0.0,  0.0,  halfSize,  0.0);
    ApplyFace(m5, spinQ,   90.0,    0.0, 0.0,  0.0, 0.0 - halfSize,  0.0);

    // Camera orbit
    if (orbitAngle >= 360.0) { orbitAngle = orbitAngle - 360.0; }
    if (orbitAngle < 0.0) { orbitAngle = orbitAngle + 360.0; }
    var tmpTf = Transform(Vector3D(0.0, orbitAngle, 0.0), Vector3D(0.0, 0.0, 0.0), Vector3D(1.0, 1.0, 1.0));
    var camOrbitQ = tmpTf.GetRotation();
    var startPos = Vector3D(0.0, 15.0, orbitDist);
    var camPos = camOrbitQ.RotateVector(startPos);
    cam.GetTransform().SetPosition(camPos);

    // Look at origin using Quaternion.LookAt (avoids gimbal lock)
    var lookDir = Vector3D(0.0, 0.0, 0.0).Subtract(camPos);
    var lookQuat = Quaternion.LookAt(lookDir, Vector3D(0.0, 1.0, 0.0));
    cam.GetTransform().SetRotation(lookQuat);

    // Animate materials
    matSolid.SetColor("color", HueRotate(40, 180, 255, hueOffset));
    matPhong.SetCameraPosition(camPos);
    matRainbow.SetVec2("positionOffset", hueOffset * 0.02, 0.0);

    // Move light to follow camera for consistent illumination
    matPhong.SetLightPosition(0, camPos.X + 20.0, camPos.Y + 30.0, camPos.Z);

    // Debug: world grid and origin axes
    debug.DrawGrid(Vector3D(0.0, 0.0, 0.0), 120.0, 60, Color(0.2, 0.2, 0.2, 1.0), 0.0, true);
    debug.DrawAxes(Vector3D(0.0, 0.0, 0.0), 15.0, 0.0, true);

    // HUD
    canvas.Clear();
    if (showHud) {
        DrawHUD();
    }
}

fn DrawHUD() {
    canvas.DrawText(2, 2, "KoiloEngine 3D Cube", 255, 255, 255);
    canvas.DrawText(2, 10, "orbit " + str(floor(orbitAngle)) + " spin " + str(floor(spinAngle)), 200, 200, 200);
    canvas.DrawText(2, 18, "hue " + str(floor(hueOffset)) + " sky " + str(floor(sky.GetTimeOfDay())), 200, 200, 200);
    canvas.DrawText(2, 98, "spc=pause r=rst t=hud", 140, 140, 140);
    canvas.DrawText(2, 91, str(floor(fps)) + " fps", 180, 220, 140);
}

fn OnKeyPress(key) {
    if (key == "space") {
        paused = not paused;
    }
    if (key == "r") {
        spinAngle  = 0.0;
        orbitAngle = 0.0;
        hueOffset  = 0.0;
    }
    if (key == "t") {
        showHud = not showHud;
    }
    if (key == "up") {
        spinSpeed = spinSpeed + 10.0;
    }
    if (key == "down") {
        spinSpeed = spinSpeed - 10.0;
    }
    if (key == "left") {
        orbitSpeed = orbitSpeed - 5.0;
    }
    if (key == "right") {
        orbitSpeed = orbitSpeed + 5.0;
    }
}
