// syntax_demo.ks - KoiloScript Language Feature Showcase
// A runnable demo exercising every language construct and core API.

// -- Display Configuration --------------------------------------------

display.SetType("desktop");
display.SetWidth(960);
display.SetHeight(540);
display.SetPixelWidth(960);
display.SetPixelHeight(540);
display.SetTargetFPS(60);
display.SetCapFPS(true);

// -- Canvas for 2D drawing --------------------------------------------

var canvas = Canvas2D();
canvas.Resize(192, 108);
canvas.Scale(960, 540);
canvas.Attach();

// -- Constructor Expressions ------------------------------------------

var white   = Color888(255, 255, 255);
var red     = Color888(255, 30, 30);
var green   = Color888(30, 255, 80);
var blue    = Color888(30, 100, 255);
var yellow  = Color888(255, 220, 40);
var origin  = Vector3D(0, 0, 0);
var up      = Vector3D(0, 0, 1);
var fwd     = Vector3D(0, -1, 0);

// -- Materials (KSLMaterial) ------------------------------------------

var matPBR   = KSLMaterial("pbr");
var matNoise = KSLMaterial("procedural_noise");
var matFloor = KSLMaterial("uniform_color");
var matSky   = KSLMaterial("sky");

// -- Geometry ---------------------------------------------------------

var primFloor = PrimitiveMesh();
var primCube  = PrimitiveMesh();
var meshFloor = null;
var meshCube  = null;

// -- Tables and Arrays -----------------------------------------------

var config = {
    speed: 0.08,
    gravity: -9.81,
    maxHealth: 100,
    name: "Player One",
    isAlive: true,
    debug: false
};

var palette = [red, green, blue, yellow, white];
var scores  = [0, 0, 0, 0];

// -- Global Variables ------------------------------------------------

var timer      = 0;
var frameCount = 0;
var isRunning  = true;
var camAngle   = 0;

// -- User Functions --------------------------------------------------

fn lerp(a, b, t) {
    return a + (b - a) * t;
}

fn clamp(value, lo, hi) {
    if value < lo { return lo; }
    if value > hi { return hi; }
    return value;
}

fn easeInOut(t) {
    if t < 0.5 {
        return 2 * t * t;
    } else {
        return 1 - (-2 * t + 2) * (-2 * t + 2) / 2;
    }
}

fn applyDamage(amount) {
    var newHealth = config.maxHealth - amount;
    config.maxHealth = clamp(newHealth, 0, 100);

    if config.maxHealth == 0 {
        config.isAlive = false;
        print("Game Over!");
    }

    return config.maxHealth;
}

fn formatScore(idx) {
    return "P" + str(idx + 1) + ": " + str(scores[idx]);
}

// -- Setup -----------------------------------------------------------

fn Setup() {
    // Floor quad
    primFloor.CreateTexturedQuad(20.0, 20.0);
    meshFloor = primFloor.GetMesh();
    matFloor.SetColor("albedo", Color888(50, 55, 60));
    meshFloor.SetMaterial(matFloor);
    meshFloor.SetTransform(Transform(Vector3D(0, 0, 0), Vector3D(0, 0, -1), Vector3D(1, 1, 1)));
    scene.AddMesh(meshFloor);

    // Spinning cube
    primCube.CreateTexturedQuad(2.0, 2.0);
    meshCube = primCube.GetMesh();
    matPBR.SetColor("albedo", Color888(200, 60, 60));
    matPBR.SetFloat("roughness", 0.4);
    matPBR.SetFloat("metallic", 0.8);
    matPBR.SetFloat("ao", 1.0);
    matPBR.AddLight();
    matPBR.SetLightPosition(0, 5.0, -5.0, 8.0);
    matPBR.SetLightColor(0, 1.0, 0.94, 0.86);
    matPBR.SetLightIntensity(0, 2.0);
    meshCube.SetMaterial(matPBR);
    scene.AddMesh(meshCube);

    // Camera
    cam.SetPerspective(60.0, 0.1, 500.0);
    cam.SetBackfaceCulling(true);

    // Sky
    sky.SetMaterial(matSky);
    sky.Enable();
    sky.SetTimeOfDay(10.0);
    sky.SetTimeSpeed(0.5);

    // Debug overlays
    debug.Enable();
    debug.DrawGrid(origin, 20.0, 10, Color(0.4, 0.4, 0.4, 1.0), 0.0, true);
    debug.DrawAxes(origin, 2.0, 0.0, true);

    print("Syntax demo: all systems initialized");
}

// -- Input Handling --------------------------------------------------

fn OnKeyPress(key) {
    // State switching
    if key == "1" { config.debug = not config.debug; }
    if key == "2" { config.isAlive = true; config.maxHealth = 100; }
    if key == "space" {
        applyDamage(25);
    }

    // Builtin math functions
    var angle = random(0, 360);
    var rad = angle * 3.14159 / 180;
    var x = cos(rad) * 10;
    var y = sin(rad) * 10;
    var dist = sqrt(x * x + y * y);
    print("Random: angle=" + str(round(angle)) + " dist=" + str(round(dist)));
}

// -- Update ----------------------------------------------------------

fn Update(dt) {
    timer = timer + dt;
    frameCount = frameCount + 1;

    // -- Table property access ------------------------------------
    var t = easeInOut(clamp(timer / 4.0, 0, 1));
    config.speed = lerp(0.01, 0.15, t);

    // -- Transform animation --------------------------------------
    var bobHeight = sin(timer * 2.0) * 0.3;
    meshCube.SetTransform(Transform(
        Vector3D(timer * 30, timer * 45, 0),
        Vector3D(0, 0, 1.5 + bobHeight),
        Vector3D(1, 1, 1)
    ));

    // -- Material animation ---------------------------------------
    var r = clamp(abs(sin(timer)) * 255, 0, 255);
    var g = clamp(abs(cos(timer * 0.7)) * 255, 0, 255);
    var b = clamp(abs(sin(timer * 1.3)) * 200, 0, 255);
    matPBR.SetColor("albedo", Color888(r, g, b));
    matPBR.SetCameraPosition(cam.GetTransform().GetPosition());

    // -- Camera orbit ---------------------------------------------
    camAngle = camAngle + dt * 15;
    var camDist = 10;
    var camX = sin(camAngle * 3.14159 / 180) * camDist;
    var camY = cos(camAngle * 3.14159 / 180) * camDist;
    var camPos = Vector3D(camX, camY, 5);
    var lookDir = origin.Subtract(camPos);
    var camQuat = Quaternion.LookAt(lookDir, up);
    cam.GetTransform().SetPosition(camPos);
    cam.GetTransform().SetRotation(camQuat);

    // -- Conditional logic with logical operators -----------------
    if isRunning and config.isAlive {
        scores[0] = scores[0] + 1;
    }

    if not config.isAlive or timer > 120 {
        isRunning = false;
    }

    // -- While loop -----------------------------------------------
    var i = 0;
    while i < 4 {
        scores[i] = min(scores[i], 99999);
        i = i + 1;
    }

    // -- For-each loop --------------------------------------------
    var colorIdx = 0;
    for c in palette {
        colorIdx = colorIdx + 1;
    }

    // -- Break and continue ---------------------------------------
    var sum = 0;
    var j = 0;
    while j < 100 {
        j = j + 1;
        if j % 2 == 0 { continue; }
        sum = sum + j;
        if sum > 500 { break; }
    }

    // -- Null safety ----------------------------------------------
    var nothing = null;
    if nothing == null {
        // Handled
    }

    // -- Ternary expression ---------------------------------------
    var status = config.isAlive ? "ALIVE" : "DEAD";

    // -- Boolean constants ----------------------------------------
    var enabled = true;
    var paused = false;

    // -- Canvas 2D HUD --------------------------------------------
    canvas.Clear();
    var fps = 1.0 / dt;
    canvas.DrawText(2, 2, "FPS: " + str(round(fps)), 255, 255, 255);
    canvas.DrawText(2, 12, "HP: " + str(config.maxHealth) + " [" + status + "]", 30, 255, 80);
    canvas.DrawText(2, 22, formatScore(0) + "  " + formatScore(1), 255, 220, 40);
    canvas.DrawText(2, 32, "Timer: " + str(round(timer)) + "s", 255, 255, 255);
    canvas.DrawText(2, 42, "Sky: " + str(round(sky.GetTimeOfDay())) + "h", 30, 100, 255);

    if config.debug {
        canvas.DrawText(2, 98, "[DEBUG] Angle: " + str(round(camAngle)), 150, 150, 150);
    }
}
