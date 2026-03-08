// =============================================================================
// KoiloEngine 3D Stress Test - Complex Multi-Object Scene
// Demonstrates: many meshes, diverse KSL materials (PBR, Phong, procedural),
//               per-object animation, orbiting camera, sky system, debug overlays
// All materials use KSLMaterial - runtime-loaded KSL shaders.
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

// -- Materials (all KSLMaterial) ----------------------------------------------

// Floor: PBR metallic ground plane
var matFloor = KSLMaterial("pbr");
matFloor.SetColor("albedo", Color888(60, 65, 70));
matFloor.SetFloat("roughness", 0.85);
matFloor.SetFloat("metallic", 0.1);
matFloor.SetFloat("ao", 0.9);

// Walls: Phong-lit stone walls (left/right only)

// Pillars: shiny PBR columns
var matPillar = KSLMaterial("pbr");
matPillar.SetColor("albedo", Color888(180, 160, 130));
matPillar.SetFloat("roughness", 0.25);
matPillar.SetFloat("metallic", 0.7);

// Central orb: emissive PBR
var matOrb = KSLMaterial("pbr");
matOrb.SetColor("albedo", Color888(40, 200, 255));
matOrb.SetFloat("roughness", 0.1);
matOrb.SetFloat("metallic", 0.9);
matOrb.SetVec3("emissive", 0.31, 1.57, 2.0);

// Floating cubes: procedural noise
var matNoise1 = KSLMaterial("procedural_noise");
matNoise1.SetRainbowPalette("colors", 4);
var matNoise2 = KSLMaterial("procedural_noise");
matNoise2.SetRainbowPalette("colors", 4);

// Ring objects: spiral pattern
var matSpiral1 = KSLMaterial("spiral");
matSpiral1.SetRainbowPalette("colors", 8);
matSpiral1.SetFloat("width", 0.1);
matSpiral1.SetFloat("bend", 2.0);
var matSpiral2 = KSLMaterial("spiral");
matSpiral2.SetRainbowPalette("colors", 5);
matSpiral2.SetFloat("width", 0.2);
matSpiral2.SetFloat("bend", 1.0);

// Accent cubes: solid colors
var matRed    = KSLMaterial("uniform_color");
matRed.SetColor("color", Color888(220, 50, 50));
var matGreen  = KSLMaterial("uniform_color");
matGreen.SetColor("color", Color888(50, 200, 80));
var matBlue   = KSLMaterial("uniform_color");
matBlue.SetColor("color", Color888(50, 80, 220));
var matYellow = KSLMaterial("uniform_color");
matYellow.SetColor("color", Color888(220, 200, 50));
var matPurple = KSLMaterial("uniform_color");
matPurple.SetColor("color", Color888(160, 50, 200));
var matOrange = KSLMaterial("uniform_color");
matOrange.SetColor("color", Color888(230, 140, 30));

// Rainbow band on one wall
var matRainbow = KSLMaterial("horizontal_rainbow");
matRainbow.SetRainbowPalette("colors", 8);
matRainbow.SetFloat("gradientPeriod", 14.0);

// Sky material (time-of-day auto-updates topColor/bottomColor)
var matSky = KSLMaterial("sky");

// Depth visualization panel
var matDepth = KSLMaterial("depth");
matDepth.SetInt("axis", 0);
matDepth.SetFloat("depth", 80.0);
matDepth.SetFloat("offset", 10.0);
matDepth.SetInt("colorCount", 4);
matDepth.SetColorAt("colors", 0, Color888(0, 255, 0));
matDepth.SetColorAt("colors", 1, Color888(255, 0, 0));
matDepth.SetColorAt("colors", 2, Color888(0, 255, 0));
matDepth.SetColorAt("colors", 3, Color888(0, 0, 255));

// -- Geometry (PrimitiveMesh objects created in global, meshes built in Setup) -

var primFloor    = PrimitiveMesh();
var primLeftWall = PrimitiveMesh();
var primRightWall = PrimitiveMesh();
var meshFloor = 0;
var meshLeftWall = 0;
var meshRightWall = 0;

var pillarQuads = [];
var pillarMeshes = [];
var pillarCount = 4;
var pillarPositions = [
    Vector3D(-30.0, 0.0, -30.0),
    Vector3D( 30.0, 0.0, -30.0),
    Vector3D(-30.0, 0.0,  30.0),
    Vector3D( 30.0, 0.0,  30.0)
];

var orbQuads = [];
var orbMeshes = [];

var cubeColors = [matRed, matGreen, matBlue, matYellow, matPurple, matOrange, matNoise1, matSpiral1];
var cubeQuads = [];
var cubeMeshes = [];
var cubePositions = [
    Vector3D(-50.0,  6.0, -48.0),
    Vector3D( 50.0, 20.0, -48.0),
    Vector3D(-50.0, 14.0,  48.0),
    Vector3D( 50.0,  8.0,  48.0),
    Vector3D(-50.0, 24.0,   0.0),
    Vector3D( 50.0, 12.0,   0.0),
    Vector3D(  0.0, 28.0, -50.0),
    Vector3D(  0.0, 10.0,  50.0)
];
var cubeSize = 5.0;
var cubeHalf = 2.5;

var panelQuads = [];
var panelMeshes = [];
var panelMats = [matNoise2, matSpiral2, matRainbow, matDepth];
var panelPositions = [
    Vector3D(-40.0, 15.0, -55.0),
    Vector3D( 40.0, 15.0, -55.0),
    Vector3D(-40.0, 15.0,  55.0),
    Vector3D( 40.0, 15.0,  55.0)
];

// -- State --------------------------------------------------------------------

var time = 0.0;
var orbitAngle = 0.0;
var orbitDist = 80.0;
var orbitHeight = 25.0;
var orbitSpeed = 15.0;
var showHud = true;

// HSL hue rotation: shift base RGB color by hueDeg degrees
fn HueRotate(r, g, b, hueDeg) {
    // Convert RGB to HSL-like rotation via Rodrigues' rotation in RGB space
    var rr = r / 255.0;
    var gg = g / 255.0;
    var bb = b / 255.0;
    var angle = hueDeg * 3.14159265 / 180.0;
    var cosA = cos(angle);
    var sinA = sin(angle);
    // Rotation around (1,1,1) axis in RGB space
    var k = 0.57735026919; // 1/sqrt(3)
    var oneMinusCos = 1.0 - cosA;
    var rOut = rr * (cosA + k * k * oneMinusCos) + gg * (k * k * oneMinusCos - k * sinA) + bb * (k * k * oneMinusCos + k * sinA);
    var gOut = rr * (k * k * oneMinusCos + k * sinA) + gg * (cosA + k * k * oneMinusCos) + bb * (k * k * oneMinusCos - k * sinA);
    var bOut = rr * (k * k * oneMinusCos - k * sinA) + gg * (k * k * oneMinusCos + k * sinA) + bb * (cosA + k * k * oneMinusCos);
    // Clamp to [0,255]
    if (rOut < 0.0) { rOut = 0.0; }
    if (rOut > 1.0) { rOut = 1.0; }
    if (gOut < 0.0) { gOut = 0.0; }
    if (gOut > 1.0) { gOut = 1.0; }
    if (bOut < 0.0) { bOut = 0.0; }
    if (bOut > 1.0) { bOut = 1.0; }
    return Color888(floor(rOut * 255.0), floor(gOut * 255.0), floor(bOut * 255.0));
}

fn ApplyFace(mesh, centerQ, faceEulerX, faceEulerY, faceEulerZ, offX, offY, offZ, basePos) {
    var tmpTf = Transform(Vector3D(faceEulerX, faceEulerY, faceEulerZ), Vector3D(0.0, 0.0, 0.0), Vector3D(1.0, 1.0, 1.0));
    var faceQ = tmpTf.GetRotation();
    var totalQ = centerQ.Multiply(faceQ);
    var offset = centerQ.RotateVector(Vector3D(offX, offY, offZ));
    var pos = Vector3D(basePos.X + offset.X, basePos.Y + offset.Y, basePos.Z + offset.Z);
    var t = Transform(Vector3D(0.0, 0.0, 0.0), pos, Vector3D(1.0, 1.0, 1.0));
    t.SetRotation(totalQ);
    mesh.SetTransform(t);
    mesh.ResetVertices();
    mesh.UpdateTransform();
}

fn PlaceCube(meshes, baseIdx, spinQ, half, basePos) {
    ApplyFace(meshes[baseIdx + 0], spinQ,    0.0,    0.0, 0.0,  0.0,  0.0,  half, basePos);
    ApplyFace(meshes[baseIdx + 1], spinQ,    0.0,  180.0, 0.0,  0.0,  0.0, 0.0 - half, basePos);
    ApplyFace(meshes[baseIdx + 2], spinQ,    0.0,   90.0, 0.0,  half,  0.0,  0.0, basePos);
    ApplyFace(meshes[baseIdx + 3], spinQ,    0.0,  -90.0, 0.0, 0.0 - half,  0.0,  0.0, basePos);
    ApplyFace(meshes[baseIdx + 4], spinQ,  -90.0,    0.0, 0.0,  0.0,  half,  0.0, basePos);
    ApplyFace(meshes[baseIdx + 5], spinQ,   90.0,    0.0, 0.0,  0.0, 0.0 - half,  0.0, basePos);
}

fn PlaceStatic(mesh, euler_x, euler_y, euler_z, px, py, pz) {
    var tmpTf = Transform(Vector3D(euler_x, euler_y, euler_z), Vector3D(0.0, 0.0, 0.0), Vector3D(1.0, 1.0, 1.0));
    var q = tmpTf.GetRotation();
    var t = Transform(Vector3D(0.0, 0.0, 0.0), Vector3D(px, py, pz), Vector3D(1.0, 1.0, 1.0));
    t.SetRotation(q);
    mesh.SetTransform(t);
    mesh.ResetVertices();
    mesh.UpdateTransform();
}

fn PlacePillarFaces(meshes, baseIdx, pos) {
    var pillarH = 15.0;
    var pillarW = 3.0;
    var tmpTf = Transform(Vector3D(0.0, 0.0, 0.0), Vector3D(0.0, 0.0, 0.0), Vector3D(1.0, 1.0, 1.0));
    var identQ = tmpTf.GetRotation();
    ApplyFace(meshes[baseIdx + 0], identQ,   0.0,   0.0, 0.0,  0.0, pillarH,  pillarW, pos);
    ApplyFace(meshes[baseIdx + 1], identQ,   0.0, 180.0, 0.0,  0.0, pillarH, 0.0 - pillarW, pos);
    ApplyFace(meshes[baseIdx + 2], identQ,   0.0,  90.0, 0.0,  pillarW, pillarH, 0.0, pos);
    ApplyFace(meshes[baseIdx + 3], identQ,   0.0, -90.0, 0.0, 0.0 - pillarW, pillarH, 0.0, pos);
}

// -- Setup --------------------------------------------------------------------

fn Setup() {
    // Build geometry and add to scene
    primFloor.CreateTexturedQuad(120.0, 120.0);
    meshFloor = primFloor.GetMesh();
    meshFloor.SetMaterial(matFloor);
    scene.AddMesh(meshFloor);

    primLeftWall.CreateTexturedQuad(120.0, 40.0);
    meshLeftWall = primLeftWall.GetMesh();
    meshLeftWall.SetMaterial(matRainbow);
    scene.AddMesh(meshLeftWall);

    primRightWall.CreateTexturedQuad(120.0, 40.0);
    meshRightWall = primRightWall.GetMesh();
    meshRightWall.SetMaterial(matDepth);
    scene.AddMesh(meshRightWall);

    // 4 Pillars (4 side faces each)
    var pi = 0;
    while (pi < pillarCount) {
        var fi = 0;
        while (fi < 4) {
            var pq = PrimitiveMesh();
            pq.CreateTexturedQuad(6.0, 30.0);
            var pm = pq.GetMesh();
            pm.SetMaterial(matPillar);
            scene.AddMesh(pm);
            pillarQuads[pi * 4 + fi] = pq;
            pillarMeshes[pi * 4 + fi] = pm;
            fi = fi + 1;
        }
        pi = pi + 1;
    }

    // Central orb: 6-face cube
    var oi = 0;
    while (oi < 6) {
        var oq = PrimitiveMesh();
        oq.CreateTexturedQuad(8.0, 8.0);
        var om = oq.GetMesh();
        om.SetMaterial(matOrb);
        scene.AddMesh(om);
        orbQuads[oi] = oq;
        orbMeshes[oi] = om;
        oi = oi + 1;
    }

    // 8 floating accent cubes (6 faces each = 48 meshes)
    var ci = 0;
    while (ci < 8) {
        var fi = 0;
        while (fi < 6) {
            var cq = PrimitiveMesh();
            cq.CreateTexturedQuad(cubeSize, cubeSize);
            var cm = cq.GetMesh();
            cm.SetMaterial(cubeColors[ci]);
            scene.AddMesh(cm);
            cubeQuads[ci * 6 + fi] = cq;
            cubeMeshes[ci * 6 + fi] = cm;
            fi = fi + 1;
        }
        ci = ci + 1;
    }

    // 4 display panels
    var pni = 0;
    while (pni < 4) {
        var ppq = PrimitiveMesh();
        ppq.CreateTexturedQuad(15.0, 10.0);
        var ppm = ppq.GetMesh();
        ppm.SetMaterial(panelMats[pni]);
        scene.AddMesh(ppm);
        panelQuads[pni] = ppq;
        panelMeshes[pni] = ppm;
        pni = pni + 1;
    }

    // Camera & lighting
    cam.SetPerspective(60.0, 0.1, 500.0);
    cam.SetBackfaceCulling(true);

    // PBR lighting (one light each)
    matFloor.AddLight();
    matFloor.SetLightPosition(0, 0.0, 35.0, 0.0);
    matFloor.SetLightColor(0, 1.0, 0.98, 0.94);
    matFloor.SetLightIntensity(0, 255.0);
    matFloor.SetLightFalloff(0, 180.0);

    matPillar.AddLight();
    matPillar.SetLightPosition(0, 0.0, 35.0, 0.0);
    matPillar.SetLightColor(0, 1.0, 0.98, 0.94);
    matPillar.SetLightIntensity(0, 255.0);
    matPillar.SetLightFalloff(0, 180.0);

    matOrb.AddLight();
    matOrb.SetLightPosition(0, 0.0, 35.0, 0.0);
    matOrb.SetLightColor(0, 0.78, 0.86, 1.0);
    matOrb.SetLightIntensity(0, 200.0);
    matOrb.SetLightFalloff(0, 120.0);

    // Noise material setup - low scale for large smooth blobs
    matNoise1.SetVec3("noiseScale", 0.25, 0.25, 0.25);
    matNoise1.SetFloat("simplexDepth", 0.0);
    matNoise1.SetFloat("gradientPeriod", 0.1);
    matNoise2.SetVec3("noiseScale", 0.1, 0.1, 0.1);
    matNoise2.SetFloat("simplexDepth", 0.0);
    matNoise2.SetFloat("gradientPeriod", 0.2);

    // Place static geometry
    PlaceStatic(meshFloor,    -90.0, 0.0, 0.0,   0.0, 0.0, 0.0);
    PlaceStatic(meshLeftWall,   0.0, 90.0, 0.0, -60.0, 20.0,   0.0);
    PlaceStatic(meshRightWall,  0.0, -90.0, 0.0, 60.0, 20.0,   0.0);

    // Place pillars
    pi = 0;
    while (pi < pillarCount) {
        PlacePillarFaces(pillarMeshes, pi * 4, pillarPositions[pi]);
        pi = pi + 1;
    }

    // Place display panels
    pni = 0;
    while (pni < 4) {
        var panelPos = panelPositions[pni];
        var panelAngle = 0.0;
        if (panelPos.Z > 0.0) { panelAngle = 180.0; }
        PlaceStatic(panelMeshes[pni], 0.0, panelAngle, 0.0, panelPos.X, panelPos.Y, panelPos.Z);
        pni = pni + 1;
    }

    // Sky - use KSL sky material (time-of-day auto-updates colors)
    sky.SetMaterial(matSky);
    sky.Enable();
    sky.SetTimeOfDay(10.0);
    sky.SetTimeSpeed(0.3);

    // Debug grid (smaller for less overhead)
    debug.Enable();

    print("=== KoiloEngine 3D Stress Test ===");
    print("Room with pillars, floating cubes, orb, panels (open top/back)");
    print("Meshes: " + str(scene.GetMeshCount()));
}

// -- Update -------------------------------------------------------------------

fn Update(dt) {
    time = time + dt;
    orbitAngle = orbitAngle + orbitSpeed * dt;
    // Keep orbit angle in [0, 360) range
    if (orbitAngle >= 360.0) { orbitAngle = orbitAngle - 360.0; }
    if (orbitAngle < 0.0) { orbitAngle = orbitAngle + 360.0; }

    // Camera orbit position
    var tmpTf = Transform(Vector3D(0.0, orbitAngle, 0.0), Vector3D(0.0, 0.0, 0.0), Vector3D(1.0, 1.0, 1.0));
    var camQ = tmpTf.GetRotation();
    var camStart = Vector3D(0.0, orbitHeight, orbitDist);
    var camPos = camQ.RotateVector(camStart);
    cam.GetTransform().SetPosition(camPos);

    // Look at center using Quaternion.LookAt (avoids gimbal lock)
    var lookDir = Vector3D(0.0, 10.0, 0.0).Subtract(camPos);
    var lookQuat = Quaternion.LookAt(lookDir, Vector3D(0.0, 1.0, 0.0));
    cam.GetTransform().SetRotation(lookQuat);

    // Update PBR camera positions
    matFloor.SetCameraPosition(camPos);
    matPillar.SetCameraPosition(camPos);
    matOrb.SetCameraPosition(camPos);

    // Move lights to follow camera for consistent illumination
    var lightPos = Vector3D(camPos.X * 0.3, 35.0, camPos.Z * 0.3);
    matFloor.SetLightPosition(0, lightPos.X, lightPos.Y, lightPos.Z);
    matPillar.SetLightPosition(0, lightPos.X, lightPos.Y, lightPos.Z);
    matOrb.SetLightPosition(0, lightPos.X, lightPos.Y, lightPos.Z);

    // Animate orb: slow rotation + hover
    var orbY = 15.0 + sin(time * 1.5) * 3.0;
    var tmpTf = Transform(Vector3D(time * 20.0, time * 35.0, time * 10.0), Vector3D(0.0, 0.0, 0.0), Vector3D(1.0, 1.0, 1.0));
    var orbSpin = tmpTf.GetRotation();
    var orbPos = Vector3D(0.0, orbY, 0.0);
    PlaceCube(orbMeshes, 0, orbSpin, 4.0, orbPos);

    // Animate emissive pulse
    var pulse = (sin(time * 3.0) + 1.0) * 0.5;
    var eR = (40.0 + pulse * 60.0) / 255.0;
    var eG = (180.0 + pulse * 75.0) / 255.0;
    var eB = 1.0;
    var eI = 1.5 + pulse * 1.5;
    matOrb.SetVec3("emissive", eR * eI, eG * eI, eB * eI);

    // Animate floating cubes: each spins at a different rate and bobs
    var ci = 0;
    while (ci < 8) {
        var basePos = cubePositions[ci];
        var bobY = sin(time * (1.0 + ci * 0.3) + ci * 0.8) * 2.0;
        var animPos = Vector3D(basePos.X, basePos.Y + bobY, basePos.Z);
        var speed1 = 15.0 + ci * 8.0;
        var speed2 = 25.0 + ci * 5.0;
        var tmpTf = Transform(Vector3D(time * speed1, time * speed2, 0.0), Vector3D(0.0, 0.0, 0.0), Vector3D(1.0, 1.0, 1.0));
        var spinQ = tmpTf.GetRotation();
        PlaceCube(cubeMeshes, ci * 6, spinQ, cubeHalf, animPos);
        ci = ci + 1;
    }

    // Animate color shifts on solid materials (hue rotation via SetColor)
    matRed.SetColor("color", HueRotate(220, 50, 50, time * 10.0));
    matGreen.SetColor("color", HueRotate(50, 200, 80, time * 15.0));
    matBlue.SetColor("color", HueRotate(50, 80, 220, time * 20.0));
    matYellow.SetColor("color", HueRotate(220, 200, 50, time * 8.0));
    matPurple.SetColor("color", HueRotate(160, 50, 200, time * 12.0));
    matOrange.SetColor("color", HueRotate(230, 140, 30, time * 18.0));

    // Animate rainbow: scroll for hue shift + slow rotation
    matRainbow.SetVec2("positionOffset", time * 2.0, 0.0);
    matRainbow.SetFloat("rotationDeg", 90.0 + time * 8.0);

    // Animate simplex noise: slow rolling Z-slice, gentle scale drift
    var nScale1 = 0.25 + sin(time * 0.08) * 0.05;
    var nScale2 = 0.2 + sin(time * 0.06) * 0.04;
    matNoise1.SetVec3("noiseScale", nScale1, nScale1, nScale1);
    matNoise1.SetFloat("simplexDepth", time * 0.1);
    matNoise2.SetVec3("noiseScale", nScale2, nScale2, nScale2);
    matNoise2.SetFloat("simplexDepth", time * 0.08);

    // Debug overlays (small grid, less overhead)
    debug.DrawGrid(Vector3D(0.0, 0.01, 0.0), 60.0, 15, Color(0.15, 0.15, 0.15, 1.0), 0.0, true);
    debug.DrawAxes(Vector3D(0.0, 0.01, 0.0), 10.0, 0.0, true);

    // HUD
    canvas.Clear();
    if (showHud) {
        DrawHUD();
    }
}

fn DrawHUD() {
    canvas.DrawText(2, 2, "Koilo Stress Test", 255, 255, 255);
    canvas.DrawText(2, 10, "meshes: " + str(scene.GetMeshCount()) + "  tris: " + str(scene.GetTotalTriangleCount()), 200, 200, 200);
    canvas.DrawText(2, 18, "orbit: " + str(floor(orbitAngle)) + "  sky: " + str(floor(sky.GetTimeOfDay())), 200, 200, 200);
    canvas.DrawText(2, 91, str(floor(fps)) + " fps", 180, 220, 140);
    canvas.DrawText(2, 98, "spc=pause t=hud", 140, 140, 140);
}

fn OnKeyPress(key) {
    if (key == "space") {
        orbitSpeed = 0.0 - orbitSpeed;
    }
    if (key == "t") {
        showHud = not showHud;
    }
    if (key == "up") {
        orbitDist = orbitDist - 5.0;
    }
    if (key == "down") {
        orbitDist = orbitDist + 5.0;
    }
}
