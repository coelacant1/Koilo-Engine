// =============================================================================
// KoiloEngine OBJ Scene - Artisans Hub + Cynder
// Keyframe camera track with free-fly override
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

// --- Textures ---
var texMap = Texture();
var texCynder = Texture();

// --- Materials ---
var matMap = KSLMaterial("texture");
var matCynder = KSLMaterial("texture");
var matSky = KSLMaterial("sky");

// --- Mesh loaders ---
var mapLoader = MorphableMesh();
var cynderLoader = MorphableMesh();

var mapMesh = 0;
var cynderMesh = 0;

// Camera state (shared between modes)
var camX = 200.6;
var camY = -3.0;
var camZ = -1384.2;
var yaw = 181.6;
var pitch = -7.0;
var showHud = true;

// Camera mode: 0 = keyframe track, 1 = free-fly
var cameraMode = 0;

// Keyframe track state
var pathTime = 0.0;
var pathSpeed = 0.3;
var pathPaused = false;

// Spline paths (built in Setup)
var posSpline = SplinePath3D();
var yawSpline = SplinePath1D();
var pitchSpline = SplinePath1D();

// Free-fly state
var moveSpeed = 400.0;
var lookSpeed = 60.0;
var moveForward = false;
var moveBack = false;
var strafeLeft = false;
var strafeRight = false;
var moveUp = false;
var moveDown = false;
var lookUp = false;
var lookDown = false;
var lookLeft = false;
var lookRight = false;

// Capture state
var captureCount = 0;
var lastHitX = 0.0;
var lastHitY = 0.0;
var lastHitZ = 0.0;
var lastHitDist = 0.0;
var hasHit = false;
var mfps = 0.0;

// --- Interpolation helpers ---

fn AddWaypoint(x, y, z, yaw, pitch) {
    posSpline.AddPoint(x, y, z);
    yawSpline.AddPoint(yaw);
    pitchSpline.AddPoint(pitch);
}

fn Setup() {
    if (not texMap.LoadFile("assets/textures/artisans.ktex")) {
        print("WARNING: Failed to load artisans texture");
    } else {
        print("Loaded artisans texture: " + str(texMap.GetWidth()) + "x" + str(texMap.GetHeight()));
    }

    if (not texCynder.LoadFile("assets/textures/cynder.ktex")) {
        print("WARNING: Failed to load cynder texture");
    } else {
        print("Loaded cynder texture: " + str(texCynder.GetWidth()) + "x" + str(texCynder.GetHeight()));
    }

    if (not mapLoader.Load("assets/kmesh/artisans.kmesh")) {
        print("ERROR: Failed to load artisans map: " + mapLoader.GetError());
        return;
    }
    print("Loaded artisans map: " + str(mapLoader.GetVertexCount()) + " vertices");

    if (not cynderLoader.Load("assets/kmesh/cynder.kmesh")) {
        print("ERROR: Failed to load cynder: " + cynderLoader.GetError());
        return;
    }
    print("Loaded cynder: " + str(cynderLoader.GetVertexCount()) + " vertices");

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

    mapMesh.SetMaterial(matMap);
    cynderMesh.SetMaterial(matCynder);

    var cynderTf = Transform(Vector3D(0.0, 0.0, 0.0), Vector3D(0.0, 0.0, 0.0), Vector3D(20.0, 20.0, 20.0));
    cynderMesh.SetTransform(cynderTf);
    cynderMesh.ResetVertices();
    cynderMesh.UpdateTransform();

    var mapTf = Transform(Vector3D(0.0, 0.0, 0.0), Vector3D(0.0, -150.0, 0.0), Vector3D(10.0, 10.0, 10.0));
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

    debug.Enable();

    // Build camera spline from captured waypoints
    posSpline.SetLooping(true);
    yawSpline.SetLooping(true);
    pitchSpline.SetLooping(true);
    yawSpline.SetAngleMode(true);

    AddWaypoint( 200.6,   -3.0, -1384.2, 181.6,  -7.0);
    AddWaypoint( 672.2, -131.6,  -881.8, 159.3,  -0.9);
    AddWaypoint( 375.6, -131.6,  -122.2, 201.9,  -0.9);
    AddWaypoint( 406.9, -131.6,   367.6, 259.6,  -3.9);
    AddWaypoint( 775.9, -131.6,   389.3, 178.5,  -8.0);
    AddWaypoint( 758.2, -205.9,   865.0, 230.3,  -8.0);
    AddWaypoint( 774.9, -205.9,  1100.2, 122.8,  -8.0);
    AddWaypoint( 776.2, -205.9,   871.4,   2.0,   2.1);
    AddWaypoint( 781.6, -138.4,   429.9,  82.1,  -1.0);
    AddWaypoint( 409.9, -138.4,   391.0,   5.1,  -1.0);
    AddWaypoint( 350.4,  -91.1,   -18.6,  61.8,  -8.0);
    AddWaypoint(-193.3,  -91.1,  -245.1, 133.8,  -8.0);
    AddWaypoint(-637.3,  -91.1,   224.6, 102.3,  -8.0);
    AddWaypoint(-992.1,  -91.1,   472.0, 257.4,  -2.0);
    AddWaypoint(-458.1,  -91.1,   625.9,  13.0,  -2.0);
    AddWaypoint(-532.4, -131.7,   166.5, 325.3,   0.1);
    AddWaypoint(-310.0, -131.7,  -475.8, 318.2,   3.1);
    AddWaypoint(-128.0, -131.7,  -623.9,   4.8,   3.1);
    AddWaypoint(-398.7, -131.7,  -875.6,  28.2,  -5.0);
    AddWaypoint(-195.0,  -84.4, -1264.8, 295.0,   0.1);

    print("=== Keyframe Camera Track (" + str(posSpline.GetPointCount()) + " waypoints) ===");
    print("F=free-fly  P=pause  T=hud  +/-=speed");
}

fn Update(dt) {
    // --- Keyframe track mode ---
    if (cameraMode == 0) {
        var numPts = posSpline.GetPointCount();
        if (not pathPaused) {
            pathTime = pathTime + pathSpeed * dt;
            if (pathTime >= numPts) { pathTime = pathTime - numPts; }
        }

        // Evaluate Catmull-Rom splines
        var pos = posSpline.Evaluate(pathTime);
        camX = pos.X;
        camY = pos.Y;
        camZ = pos.Z;
        yaw = yawSpline.Evaluate(pathTime);
        pitch = pitchSpline.Evaluate(pathTime);
    }

    // --- Free-fly mode ---
    if (cameraMode == 1) {
        if (lookLeft)  { yaw = yaw + lookSpeed * dt; }
        if (lookRight) { yaw = yaw - lookSpeed * dt; }
        if (lookUp)    { pitch = pitch + lookSpeed * dt; }
        if (lookDown)  { pitch = pitch - lookSpeed * dt; }
        if (pitch > 89.0) { pitch = 89.0; }
        if (pitch < -89.0) { pitch = -89.0; }
        if (yaw >= 360.0) { yaw = yaw - 360.0; }
        if (yaw < 0.0) { yaw = yaw + 360.0; }

        var yawTfFly = Transform(Vector3D(0.0, yaw, 0.0), Vector3D(0.0, 0.0, 0.0), Vector3D(1.0, 1.0, 1.0));
        var yawQFly = yawTfFly.GetRotation();
        var flatForward = yawQFly.RotateVector(Vector3D(0.0, 0.0, -1.0));
        var flatRight = yawQFly.RotateVector(Vector3D(1.0, 0.0, 0.0));

        var speed = moveSpeed * dt;
        if (moveForward) { camX = camX + flatForward.X * speed; camZ = camZ + flatForward.Z * speed; }
        if (moveBack)    { camX = camX - flatForward.X * speed; camZ = camZ - flatForward.Z * speed; }
        if (strafeLeft)  { camX = camX - flatRight.X * speed;   camZ = camZ - flatRight.Z * speed; }
        if (strafeRight) { camX = camX + flatRight.X * speed;   camZ = camZ + flatRight.Z * speed; }
        if (moveUp)   { camY = camY + speed; }
        if (moveDown) { camY = camY - speed; }
    }

    // --- Common: apply camera, lighting, debug, HUD ---
    var camPos = Vector3D(camX, camY, camZ);
    cam.GetTransform().SetPosition(camPos);

    var yawTf = Transform(Vector3D(0.0, yaw, 0.0), Vector3D(0.0, 0.0, 0.0), Vector3D(1.0, 1.0, 1.0));
    var yawQ = yawTf.GetRotation();
    var pitchTf = Transform(Vector3D(pitch, 0.0, 0.0), Vector3D(0.0, 0.0, 0.0), Vector3D(1.0, 1.0, 1.0));
    var pitchQ = pitchTf.GetRotation();
    var camQ = yawQ.Multiply(pitchQ);
    cam.GetTransform().SetRotation(camQ);

    // Raycast for look-at point
    var lookDir = camQ.RotateVector(Vector3D(0.0, 0.0, -1.0));
    var ray = Ray(camPos, lookDir);
    var hit = RaycastHit();

    hasHit = false;
    if (MeshRaycast.Raycast(ray, mapMesh, hit, 10000.0, false)) {
        lastHitX = hit.point.X;
        lastHitY = hit.point.Y;
        lastHitZ = hit.point.Z;
        lastHitDist = hit.distance;
        hasHit = true;
    }
    if (MeshRaycast.Raycast(ray, cynderMesh, hit, 10000.0, false)) {
        if (hit.distance < lastHitDist or not hasHit) {
            lastHitX = hit.point.X;
            lastHitY = hit.point.Y;
            lastHitZ = hit.point.Z;
            lastHitDist = hit.distance;
            hasHit = true;
        }
    }

    // Light follows camera
    matMap.SetLightPosition(0, camX, camY + 300.0, camZ);
    matMap.SetCameraPosition(camPos);
    matCynder.SetLightPosition(0, camX, camY + 300.0, camZ);
    matCynder.SetCameraPosition(camPos);

    debug.DrawGrid(Vector3D(0.0, -150.0, 0.0), 2000.0, 40, Color(0.15, 0.15, 0.15, 1.0), 0.0, true);

    // HUD
    canvas.Clear();
    if (showHud) {
        if (cameraMode == 0) {
            var segLabel = floor(pathTime);
            if (pathPaused) {
                canvas.DrawText(2, 2, "Keyframe Track (paused)", 255, 200, 100);
            }
            if (not pathPaused) {
                canvas.DrawText(2, 2, "Keyframe Track", 100, 200, 255);
            }
            canvas.DrawText(2, 10, "seg " + str(segLabel) + "/" + str(posSpline.GetPointCount()) + "  spd " + str(pathSpeed), 180, 180, 180);
        }
        if (cameraMode == 1) {
            canvas.DrawText(2, 2, "Free-Fly Camera", 255, 255, 255);
        }
        canvas.DrawText(2, 18, "pos " + str(floor(camX)) + " " + str(floor(camY)) + " " + str(floor(camZ)), 180, 220, 140);
        canvas.DrawText(2, 26, "yaw " + str(floor(yaw)) + "  pitch " + str(floor(pitch)), 180, 180, 180);
        if (hasHit) {
            canvas.DrawText(2, 34, "hit " + str(floor(lastHitX)) + " " + str(floor(lastHitY)) + " " + str(floor(lastHitZ)), 255, 200, 100);
            canvas.DrawText(2, 42, "dist " + str(floor(lastHitDist)), 200, 200, 200);
        }
        canvas.DrawText(2, 91, str(floor(fps)) + " fps  " + str(floor(mfps)) + " mfps", 180, 220, 140);
        if (cameraMode == 0) {
            canvas.DrawText(2, 98, "f=free-fly p=pause <>=skip -/+=speed", 140, 140, 140);
        }
        if (cameraMode == 1) {
            canvas.DrawText(2, 98, "f=track wasd=move arrows=look c=cap", 140, 140, 140);
        }
        canvas.DrawText(95, 52, "+", 255, 255, 0);
    }
}

fn OnKeyPress(key) {
    // Toggle camera mode
    if (key == "f") {
        if (cameraMode == 0) {
            cameraMode = 1;
            print("=== Free-Fly Camera ===");
        } else {
            cameraMode = 0;
            print("=== Keyframe Track ===");
        }
    }

    // Keyframe track controls
    if (key == "p") { pathPaused = not pathPaused; }
    if (key == "equals") { pathSpeed = pathSpeed + 0.1; print("Track speed: " + str(pathSpeed)); }
    if (key == "minus")  { pathSpeed = pathSpeed - 0.1; if (pathSpeed < 0.05) { pathSpeed = 0.05; } print("Track speed: " + str(pathSpeed)); }

    // Skip to next/previous keyframe (keyframe track mode only)
    if (cameraMode == 0) {
        if (key == "right") {
            pathTime = floor(pathTime) + 1.0;
            var numPts = posSpline.GetPointCount();
            if (pathTime >= numPts) { pathTime = pathTime - numPts; }
            print("Keyframe " + str(floor(pathTime)) + "/" + str(numPts));
        }
        if (key == "left") {
            pathTime = floor(pathTime) - 1.0;
            var numPts = posSpline.GetPointCount();
            if (pathTime < 0.0) { pathTime = pathTime + numPts; }
            print("Keyframe " + str(floor(pathTime)) + "/" + str(numPts));
        }
    }

    if (key == "t") { showHud = not showHud; }

    // Free-fly controls (only active in free-fly mode)
    if (cameraMode == 1) {
        if (key == "w") { moveForward = true; }
        if (key == "s") { moveBack = true; }
        if (key == "a") { strafeLeft = true; }
        if (key == "d") { strafeRight = true; }
        if (key == "space") { moveUp = true; }
        if (key == "shift") { moveDown = true; }
        if (key == "up") { lookUp = true; }
        if (key == "down") { lookDown = true; }
        if (key == "left") { lookLeft = true; }
        if (key == "right") { lookRight = true; }

        if (key == "c") {
            captureCount = captureCount + 1;
            print("=== CAPTURE #" + str(captureCount) + " ===");
            print("  Camera: " + str(camX) + ", " + str(camY) + ", " + str(camZ));
            print("  Yaw: " + str(yaw) + "  Pitch: " + str(pitch));
            if (hasHit) {
                print("  LookAt: " + str(lastHitX) + ", " + str(lastHitY) + ", " + str(lastHitZ));
                print("  Distance: " + str(lastHitDist));
            }
            print("========================");
        }

        if (key == "1") { moveSpeed = 100.0; print("Speed: slow"); }
        if (key == "2") { moveSpeed = 400.0; print("Speed: normal"); }
        if (key == "3") { moveSpeed = 1200.0; print("Speed: fast"); }
    }
}

fn OnKeyRelease(key) {
    if (key == "w") { moveForward = false; }
    if (key == "s") { moveBack = false; }
    if (key == "a") { strafeLeft = false; }
    if (key == "d") { strafeRight = false; }
    if (key == "space") { moveUp = false; }
    if (key == "shift") { moveDown = false; }
    if (key == "up") { lookUp = false; }
    if (key == "down") { lookDown = false; }
    if (key == "left") { lookLeft = false; }
    if (key == "right") { lookRight = false; }
}
