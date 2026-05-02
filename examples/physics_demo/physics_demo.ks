// =============================================================================
// KoiloEngine Physics + Aerodynamics Demo
// Demonstrates: PhysicsWorld step loop, RigidBody / SphereCollider / BoxCollider
//               binding, AerodynamicsWorld + ConstantWind, ThrustEngine on a
//               dynamic body, BodyPose readback for visualization, and live
//               PhysicsDiagnostics for the HUD.
//
// Controls:
//   space   pause / resume
//   r       reset bodies to start positions
//   w       toggle wind (none / east-bound / shear)
//   t       toggle thruster on rocket
//   h       toggle HUD
//   up/dn   raise / lower thrust magnitude
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

// ----------------------------------------------------------------------------
// Static ground plane is now authored in physics_demo_world.kscene.
// The .kscene file declares groundHalfExtents, groundSize, groundCol,
// groundBody - they're all visible in this scope after the import.
// See docs/kscene-format.md for the format spec.
// ----------------------------------------------------------------------------
import "physics_demo_world.kscene"

// ----------------------------------------------------------------------------
// Static obstacle blocks - give the spheres something to bounce/roll off of
// instead of just hitting a flat plane. A flat ground produces no sideways
// impulse, so spheres landing on it just bounce vertically and never roll.
// Each obstacle is a static BoxCollider; the half-extents pair is for DrawBox.
// ----------------------------------------------------------------------------
var obstacleCount = 4;
var obstacleCenter = [
    Vector3D(0.0 -  8.0, 1.5, 0.0 -  4.0),
    Vector3D( 4.0,        2.5,  3.0),
    Vector3D(10.0,        1.0, 0.0 -  6.0),
    Vector3D(0.0 - 14.0, 3.0,  6.0)
];
var obstacleHalf = [
    Vector3D(2.5, 1.5, 2.5),
    Vector3D(3.5, 2.5, 1.5),
    Vector3D(2.0, 1.0, 4.0),
    Vector3D(1.5, 3.0, 2.0)
];
var obstacleSize = [
    Vector3D(5.0, 3.0, 5.0),
    Vector3D(7.0, 5.0, 3.0),
    Vector3D(4.0, 2.0, 8.0),
    Vector3D(3.0, 6.0, 4.0)
];
var obstacleBodies = [];
var obstacleCols   = [];

fn MakeObstacle(idx) {
    var col = BoxCollider(obstacleCenter[idx], obstacleSize[idx]);
    var body = RigidBody();
    body.MakeStatic();
    body.SetCollider(col);
    body.SetRestitution(0.55);
    body.SetFriction(0.6);
    obstacleCols[idx]   = col;
    obstacleBodies[idx] = body;
}

// ----------------------------------------------------------------------------
// Dynamic spheres - varying restitution + initial heights
// ----------------------------------------------------------------------------
var sphereCount = 6;
var sphereStart = [
    Vector3D(0.0 - 12.0, 14.0, 0.0 - 4.0),
    Vector3D(0.0 -  6.0, 18.0, 0.0 -  2.0),
    Vector3D( 0.0,        22.0,  0.0),
    Vector3D( 6.0,        26.0,  2.0),
    Vector3D(12.0,        30.0,  4.0),
    Vector3D( 3.0,        34.0, 0.0 - 5.0)
];
var sphereRadius = [1.4, 1.1, 1.6, 1.0, 1.3, 1.2];
var sphereRest   = [0.20, 0.55, 0.80, 0.95, 0.35, 0.65];

var sphereCols = [];
var sphereBodies = [];

fn MakeSphere(idx) {
    var p = sphereStart[idx];
    var r = sphereRadius[idx];
    var col = SphereCollider(p, r);
    var body = RigidBody();      // Dynamic, mass 1
    body.SetMass(r * r * r);     // crude mass ∝ volume
    body.SetCollider(col);
    body.SetRestitution(sphereRest[idx]);
    body.SetFriction(0.4);
    body.SetLinearDamping(0.02);
    body.SetAngularDamping(0.05);
    body.SetInertiaSphere(r);
    sphereCols[idx] = col;
    sphereBodies[idx] = body;
}

// ----------------------------------------------------------------------------
// "Rocket" - dynamic box with a body-fixed thrust engine pushing upward.
// rocketHalfExtents = half-extents (used by DrawBox and SetInertiaBox).
// rocketSize        = full size (used by BoxCollider, which divides by 2).
// ----------------------------------------------------------------------------
// Rectangular cross-section (X=1.6, Z=1.0) so the resting orientation on
// the long (Y) axis is unambiguous: the box prefers to lie on its widest
// face. A square cross-section can balance on an edge at 45°, which makes
// it impossible to tell visualization bugs apart from real physics.
var rocketHalfExtents = Vector3D(0.8, 1.6, 0.5);
var rocketSize        = Vector3D(1.6, 3.2, 1.0);
var rocketStart   = Vector3D(0.0 - 18.0, 8.0, 8.0);
var rocketCol     = BoxCollider(rocketStart, rocketSize);
var rocketBody    = RigidBody();
rocketBody.SetMass(2.5);
rocketBody.SetCollider(rocketCol);
rocketBody.SetRestitution(0.0);
rocketBody.SetFriction(0.6);
rocketBody.SetLinearDamping(0.05);
rocketBody.SetAngularDamping(0.30);
rocketBody.SetInertiaBox(rocketHalfExtents);

var rocketEngine = ThrustEngine();
rocketEngine.directionLocal = Vector3D(0.0, 1.0, 0.0);
rocketEngine.applicationPointLocal = Vector3D(0.0, 0.0 - 1.4, 0.0);
rocketEngine.maxThrustN = 75.0;
rocketEngine.throttle   = 0.0;

// Aerodynamic surfaces - cruciform fin pack on the rocket tail. Four fins
// radially offset so they damp roll as well as pitch/yaw. Each fin's chord
// plane is in the body's long-axis (+Y forward) direction; the fin's "up"
// is the chord-normal axis. Without surfaces the AerodynamicsWorld step
// would only see the engine and wind would have zero effect (engines
// don't read wind, surfaces do).
var rocketFinLift = AeroCurve();
rocketFinLift.InitFlatPlateLift();
var rocketFinDrag = AeroCurve();
rocketFinDrag.InitFlatPlateDrag();

// Fin geometry: tail is at body y=-1.2, fins extend ~0.6m radially out
// from the body center so roll spin produces velocity at each fin's COP.
var finRadius = 0.6;
var finTailY  = 0.0 - 1.2;
var finArea   = 0.5;

// +X fin (right side of rocket)
var rocketFinXp = AerodynamicSurface();
rocketFinXp.centerOfPressureLocal = Vector3D(finRadius, finTailY, 0.0);
rocketFinXp.forwardLocal = Vector3D(0.0, 1.0, 0.0);
rocketFinXp.upLocal      = Vector3D(0.0, 0.0, 1.0);
rocketFinXp.area = finArea;
rocketFinXp.cl = rocketFinLift;
rocketFinXp.cd = rocketFinDrag;

// -X fin (left side)
var rocketFinXn = AerodynamicSurface();
rocketFinXn.centerOfPressureLocal = Vector3D(0.0 - finRadius, finTailY, 0.0);
rocketFinXn.forwardLocal = Vector3D(0.0, 1.0, 0.0);
rocketFinXn.upLocal      = Vector3D(0.0, 0.0, 1.0);
rocketFinXn.area = finArea;
rocketFinXn.cl = rocketFinLift;
rocketFinXn.cd = rocketFinDrag;

// +Z fin (front side)
var rocketFinZp = AerodynamicSurface();
rocketFinZp.centerOfPressureLocal = Vector3D(0.0, finTailY, finRadius);
rocketFinZp.forwardLocal = Vector3D(0.0, 1.0, 0.0);
rocketFinZp.upLocal      = Vector3D(1.0, 0.0, 0.0);
rocketFinZp.area = finArea;
rocketFinZp.cl = rocketFinLift;
rocketFinZp.cd = rocketFinDrag;

// -Z fin (back side)
var rocketFinZn = AerodynamicSurface();
rocketFinZn.centerOfPressureLocal = Vector3D(0.0, finTailY, 0.0 - finRadius);
rocketFinZn.forwardLocal = Vector3D(0.0, 1.0, 0.0);
rocketFinZn.upLocal      = Vector3D(1.0, 0.0, 0.0);
rocketFinZn.area = finArea;
rocketFinZn.cl = rocketFinLift;
rocketFinZn.cd = rocketFinDrag;

// Precomputed locals for visualization (avoid passing field-reads of complex
// objects into DrawFin, which the script marshaller doesn't always snapshot
// cleanly for nested Vector3D fields on user-defined object instances).
var finCopXp = Vector3D(finRadius, finTailY, 0.0);
var finCopXn = Vector3D(0.0 - finRadius, finTailY, 0.0);
var finCopZp = Vector3D(0.0, finTailY, finRadius);
var finCopZn = Vector3D(0.0, finTailY, 0.0 - finRadius);
var finUpXX  = Vector3D(0.0, 0.0, 1.0);
var finUpZZ  = Vector3D(1.0, 0.0, 0.0);

// ----------------------------------------------------------------------------
// Wind fields (we hold both, swap between them with W)
// ----------------------------------------------------------------------------
var windNone     = ConstantWind(Vector3D(0.0, 0.0, 0.0));
var windEast     = ConstantWind(Vector3D(8.0, 0.0, 0.0));
var windShear    = ShearWind(Vector3D(2.0, 0.0, 0.0), Vector3D(0.4, 0.0, 0.0));
var windMode     = 0;          // 0=none 1=east 2=shear
var windLabel    = "off";

// ----------------------------------------------------------------------------
// State
// ----------------------------------------------------------------------------
var paused      = false;
var showHud     = true;
var thrustOn    = false;
var thrustLevel = 0.6;
var orbitAngle  = 0.0;
var orbitSpeed  = 12.0;
var orbitDist   = 60.0;

fn ResetBodies() {
    var idx = 0;
    while (idx < sphereCount) {
        var p = sphereStart[idx];
        var pose = BodyPose(p);
        sphereBodies[idx].SetPose(pose);
        sphereBodies[idx].SetVelocity(Vector3D(0.0, 0.0, 0.0));
        sphereBodies[idx].SetAngularVelocity(Vector3D(0.0, 0.0, 0.0));
        sphereBodies[idx].Wake();
        idx = idx + 1;
    }
    var rpose = BodyPose(rocketStart);
    rocketBody.SetPose(rpose);
    rocketBody.SetVelocity(Vector3D(0.0, 0.0, 0.0));
    rocketBody.SetAngularVelocity(Vector3D(0.0, 0.0, 0.0));
    rocketBody.Wake();
    rocketEngine.throttle = 0.0;
    thrustOn = false;
}

fn ApplyWindMode() {
    if (windMode == 0) {
        aero.SetWindField(windNone);
        windLabel = "off";
    }
    if (windMode == 1) {
        aero.SetWindField(windEast);
        windLabel = "east 8m/s";
    }
    if (windMode == 2) {
        aero.SetWindField(windShear);
        windLabel = "shear";
    }
}

fn Setup() {
    // Build spheres
    var i = 0;
    while (i < sphereCount) {
        MakeSphere(i);
        i = i + 1;
    }

    // Build static obstacles
    var oi = 0;
    while (oi < obstacleCount) {
        MakeObstacle(oi);
        oi = oi + 1;
    }

    // Tune physics world
    physics.SetGravity(Vector3D(0.0, 0.0 - 9.81, 0.0));
    physics.SetFixedTimestep(1.0 / 120.0);
    physics.SetMaxSubSteps(4);

    // Add bodies (ground first so it lives in a known slot)
    physics.AddBody(groundBody);
    var ob = 0;
    while (ob < obstacleCount) {
        physics.AddBody(obstacleBodies[ob]);
        ob = ob + 1;
    }
    var k = 0;
    while (k < sphereCount) {
        physics.AddBody(sphereBodies[k]);
        k = k + 1;
    }
    physics.AddBody(rocketBody);

    // Aero registrations
    aero.RegisterEngine(rocketBody, rocketEngine);
    aero.RegisterSurface(rocketBody, rocketFinXp);
    aero.RegisterSurface(rocketBody, rocketFinXn);
    aero.RegisterSurface(rocketBody, rocketFinZp);
    aero.RegisterSurface(rocketBody, rocketFinZn);
    ApplyWindMode();

    // Camera
    cam.SetPerspective(60.0, 0.1, 500.0);
    cam.SetBackfaceCulling(true);

    // Visuals
    debug.Enable();
    print("=== KoiloEngine Physics + Aero Demo ===");
    print("space=pause  r=reset  w=wind  t=thrust  up/dn=throttle  h=HUD");
}

fn DrawSphereBody(body, radius, col) {
    var pose = body.GetPose();
    var px = pose.position.X;
    var py = pose.position.Y;
    var pz = pose.position.Z;
    var p = Vector3D(px, py, pz);
    var c = Color(col.X, col.Y, col.Z, 1.0);
    debug.DrawSphere(p, radius, c, 0.0, true, 0);

    // Spin indicator: draw a 3-axis tripod inside the sphere so rotation is visible.
    var rot = pose.orientation;
    var axisLen = radius * 0.95;
    var ax = rot.RotateVector(Vector3D(axisLen, 0.0, 0.0));
    var ay = rot.RotateVector(Vector3D(0.0, axisLen, 0.0));
    var az = rot.RotateVector(Vector3D(0.0, 0.0, axisLen));
    debug.DrawLine(p, p.Add(ax), Color(1.0, 0.2, 0.2, 1.0), 0.0, true);
    debug.DrawLine(p, p.Add(ay), Color(0.2, 1.0, 0.2, 1.0), 0.0, true);
    debug.DrawLine(p, p.Add(az), Color(0.2, 0.4, 1.0, 1.0), 0.0, true);
}

fn DrawBoxBody(body, extents, col) {
    var pose = body.GetPose();
    var px = pose.position.X;
    var py = pose.position.Y;
    var pz = pose.position.Z;
    var p = Vector3D(px, py, pz);
    var c = Color(col.X, col.Y, col.Z, 1.0);
    var rotMat = Matrix4x4.Rotation(pose.orientation);
    debug.DrawOrientedBox(p, extents, rotMat, c, 0.0, true, 0);
}

// Visualize an aerodynamic fin attached to body: draws (1) a magenta line
// from body COM to the fin's COP showing the radial offset, (2) a small
// cyan stub along the fin's "up" (chord-normal) axis showing fin
// orientation, and (3) a yellow arrow at the COP showing the force the
// engine applied last substep (read from surface.lastForceWorld). Force
// is scaled down so it stays visible without dwarfing the scene.
fn DrawFin(body, surf, copLocal, upLocal, span) {
    var pose = body.GetPose();
    var bx = pose.position.X;
    var by = pose.position.Y;
    var bz = pose.position.Z;
    var bodyP = Vector3D(bx, by, bz);
    var rot   = pose.orientation;
    var copWorld = bodyP.Add(rot.RotateVector(copLocal));
    var upWorld  = rot.RotateVector(upLocal);
    var stubEnd  = copWorld.Add(Vector3D(upWorld.X * span,
                                         upWorld.Y * span,
                                         upWorld.Z * span));
    debug.DrawLine(bodyP, copWorld,
                   Color(1.0, 0.2, 0.9, 1.0), 0.0, true);
    debug.DrawLine(copWorld, stubEnd,
                   Color(0.2, 0.9, 1.0, 1.0), 0.0, true);

    // Force arrow (yellow). Scale: 1 N -> 0.05 m so a typical 20 N fin
    // force draws ~1 m long. lastForceWorld is updated each substep by
    // AerodynamicsWorld::ApplySurface.
    var fScale = 0.05;
    var fx = surf.lastForceWorld.X * fScale;
    var fy = surf.lastForceWorld.Y * fScale;
    var fz = surf.lastForceWorld.Z * fScale;
    var fEnd = Vector3D(copWorld.X + fx, copWorld.Y + fy, copWorld.Z + fz);
    debug.DrawLine(copWorld, fEnd, Color(1.0, 1.0, 0.2, 1.0), 0.0, false);
}

fn Update(dt) {
    if (not paused) {
        orbitAngle = orbitAngle + orbitSpeed * dt;
    }

    // Camera orbit around the action area
    if (orbitAngle >= 360.0) { orbitAngle = orbitAngle - 360.0; }
    if (orbitAngle <  0.0)   { orbitAngle = orbitAngle + 360.0; }
    var camRotTf = Transform(Vector3D(0.0, orbitAngle, 0.0),
                             Vector3D(0.0, 0.0, 0.0),
                             Vector3D(1.0, 1.0, 1.0));
    var camQ   = camRotTf.GetRotation();
    var camPos = camQ.RotateVector(Vector3D(0.0, 18.0, orbitDist));
    cam.GetTransform().SetPosition(camPos);
    var lookDir  = Vector3D(0.0, 4.0, 0.0).Subtract(camPos);
    var lookQuat = Quaternion.LookAt(lookDir, Vector3D(0.0, 1.0, 0.0));
    cam.GetTransform().SetRotation(lookQuat);

    // Rocket thrust
    if (thrustOn) {
        rocketEngine.throttle = thrustLevel;
    } else {
        rocketEngine.throttle = 0.0;
    }

    // Visualize ground (only the visible top slab so the wireframe stays useful)
    debug.DrawBox(Vector3D(0.0, 0.0 - 1.0, 0.0), Vector3D(40.0, 1.0, 40.0),
                  Color(0.30, 0.30, 0.30, 1.0), 0.0, true, 0);

    // Visualize obstacle blocks
    var oi = 0;
    while (oi < obstacleCount) {
        debug.DrawBox(obstacleCenter[oi], obstacleHalf[oi],
                      Color(0.55, 0.45, 0.20, 1.0), 0.0, true, 0);
        oi = oi + 1;
    }
    debug.DrawGrid(Vector3D(0.0, 0.0, 0.0), 80.0, 40,
                   Color(0.18, 0.18, 0.20, 1.0), 0.0, true);
    debug.DrawAxes(Vector3D(0.0, 0.0, 0.0), 5.0, 0.0, true);

    // Visualize each sphere body (color hue per slot)
    var idx = 0;
    while (idx < sphereCount) {
        var hueR = 0.35 + 0.65 * (idx % 2);
        var hueG = 0.40 + 0.40 * ((idx + 1) % 3) / 2.0;
        var hueB = 0.95 - 0.45 * (idx % 3) / 2.0;
        DrawSphereBody(sphereBodies[idx], sphereRadius[idx],
                       Vector3D(hueR, hueG, hueB));
        // Sleep marker
        if (sphereBodies[idx].IsSleeping()) {
            var sp = sphereBodies[idx].GetPose();
            var ppx = sp.position.X;
            var ppy = sp.position.Y;
            var ppz = sp.position.Z;
            var pp = Vector3D(ppx, ppy, ppz);
            debug.DrawSphere(pp, sphereRadius[idx] * 1.1,
                             Color(0.6, 0.6, 0.2, 1.0), 0.0, false, 0);
        }
        idx = idx + 1;
    }

    // Rocket
    var rocketColor = Vector3D(1.0, 0.45, 0.10);
    if (thrustOn) {
        rocketColor = Vector3D(1.0, 0.85, 0.20);
    }
    DrawBoxBody(rocketBody, rocketHalfExtents, rocketColor);
    // Fin visualization: each fin's radial offset (magenta) + chord-normal (cyan)
    DrawFin(rocketBody, rocketFinXp, finCopXp, finUpXX, 0.5);
    DrawFin(rocketBody, rocketFinXn, finCopXn, finUpXX, 0.5);
    DrawFin(rocketBody, rocketFinZp, finCopZp, finUpZZ, 0.5);
    DrawFin(rocketBody, rocketFinZn, finCopZn, finUpZZ, 0.5);

    // Wind arrow at scene origin (only when active)
    if (windMode != 0) {
        var sample = Vector3D(0.0, 0.0, 0.0);
        if (windMode == 1) { sample = Vector3D(8.0, 0.0, 0.0); }
        if (windMode == 2) { sample = Vector3D(2.0 + 0.4 * 6.0, 0.0, 0.0); }
        var arrowEnd = Vector3D(sample.X, sample.Y + 6.0, sample.Z);
        debug.DrawLine(Vector3D(0.0, 6.0, 0.0), arrowEnd,
                       Color(0.4, 0.7, 1.0, 1.0), 0.0, false);
    }

    // HUD
    canvas.Clear();
    if (showHud) { DrawHUD(); }
}

fn DrawHUD() {
    var diag = physics.ComputeDiagnostics();
    var bodies = physics.GetBodyCount();
    var contacts = physics.GetDebugContactCount();
    var lm = diag.linearMomentum;
    var ke = diag.kineticEnergy;

    canvas.DrawText(2,  2,  "KoiloEngine Physics + Aero", 255, 255, 255);
    canvas.DrawText(2, 10,  "bodies=" + str(bodies) + " contacts=" + str(contacts), 200, 200, 200);
    canvas.DrawText(2, 18,  "KE=" + str(floor(ke * 10.0) / 10.0), 180, 220, 180);
    canvas.DrawText(2, 26,  "p=(" + str(floor(lm.X * 10.0) / 10.0) + "," +
                                     str(floor(lm.Y * 10.0) / 10.0) + "," +
                                     str(floor(lm.Z * 10.0) / 10.0) + ")", 180, 200, 220);
    canvas.DrawText(2, 34,  "wind=" + windLabel + " thrust=" + str(floor(thrustLevel * 100.0)) + "%", 220, 200, 160);
    canvas.DrawText(2, 91,  str(floor(fps)) + " fps", 180, 220, 140);
    canvas.DrawText(2, 98,  "spc=pause r=rst w=wind t=thr up/dn=lvl h=hud", 140, 140, 140);
    if (paused) {
        canvas.DrawText(80, 50, "PAUSED", 255, 60, 60);
    }
}

fn OnKeyPress(key) {
    if (key == "space") { paused = not paused; }
    if (key == "r")     { ResetBodies(); }
    if (key == "h")     { showHud = not showHud; }
    if (key == "t")     { thrustOn = not thrustOn; }
    if (key == "w") {
        windMode = (windMode + 1) % 3;
        ApplyWindMode();
    }
    if (key == "up") {
        thrustLevel = thrustLevel + 0.1;
        if (thrustLevel > 1.0) { thrustLevel = 1.0; }
    }
    if (key == "down") {
        thrustLevel = thrustLevel - 0.1;
        if (thrustLevel < 0.0) { thrustLevel = 0.0; }
    }
    if (key == "left")  { orbitSpeed = orbitSpeed - 4.0; }
    if (key == "right") { orbitSpeed = orbitSpeed + 4.0; }
}
