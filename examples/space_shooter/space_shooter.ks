// space_shooter.ks - KoiloEngine Space Shooter Example
// =====================================================
// A feature showcase game exercising the engine's major systems:
//
//   Imports      - multi-file project (player, enemies, weapons, effects)
//   Classes      - Player, Enemy, Bullet, Explosion (script-defined)
//   Signals      - on_enemy_killed, on_player_hit, on_wave_complete
//   Coroutines   - wave spawning sequences with yield
//   Modules      - has_module() / list_modules() runtime checks
//   Math         - sin, cos, random, abs, floor, clamp
//   Input        - OnKeyPress / OnKeyRelease for controls
//   Debug draw   - debug.DrawBox for collision visualization
//   Profiler     - frame time display
//   Backend:
//       Vector2D         - all entity positions & velocities
//       Circle2D         - collision detection (OverlapsCircle)
//       Triangle2D       - player collision shape (IsInShape)
//       AABB2D           - screen bounds checking (Contains)
//       Color888         - explosion colors (Lerp), star tints
//       Mathematics      - Lerp (movement), PingPong (flame flicker)
//       FunctionGenerator - fighter sine-wave movement
//       BouncePhysics    - asteroid wall bounce behavior
//       DampedSpring     - screen shake decay
//       SimplexNoise     - procedural star brightness variation
//
// Controls: A/D or Left/Right = move, Space = shoot, R = restart, P = pause
// =====================================================

// -- Imports ------------------------------------------
import "player.ks";
import "enemies.ks";
import "weapons.ks";
import "effects.ks";

// -- Display Configuration ----------------------------
display.SetType("desktop");
display.SetWidth(960);
display.SetHeight(720);
display.SetPixelWidth(64);
display.SetPixelHeight(48);
display.SetTargetFPS(60);
display.SetCapFPS(true);

// -- Canvas (user-created 2D drawing surface) ---------
var canvas = Canvas2D();
canvas.Resize(64, 48);
canvas.Attach();

// -- Screen Dimensions (cached from display config) ---
var SW = display.GetPixelWidth();
var SH = display.GetPixelHeight();

// -- Procedural Noise (SimplexNoise for star variation) -
var noise = SimplexNoise(42);

// -- Signals ------------------------------------------
signal on_enemy_killed(x, y, kind);
signal on_player_hit();
signal on_wave_complete(waveNum);

// -- Game State ---------------------------------------
var player = new Player();
var bullets = [];
var enemies = [];
var score = 0;
var wave = 0;
var gameOver = false;
var paused = false;
var gameTime = 0.0;
var waveSpawning = false;

var displayScore = 0;
var moveLeft = false;
var moveRight = false;
var moveUp = false;
var moveDown = false;
var shooting = false;

// -- Config Table -------------------------------------
var config = {
    starCount: 30,
    waveDelay: 2.0,
    baseEnemySpeed: 8.0,
    maxBullets: 20,
    debugColliders: false
};

// -- Star Field (simple parallax dots) ----------------
var stars = [];
var starColors = [];

fn InitStars() {
    var i = 0;
    while i < config.starCount {
        var sx = random() * SW;
        var sy = random() * SH;
        // First half = background (dim, slow), second half = foreground (bright, faster)
        var isBg = i < floor(config.starCount / 2);
        var spd = 0.0;
        var bright = 0;
        if isBg {
            spd = 0.25 + random() * 0.75;
            bright = 60 + floor(random() * 80);
        } else {
            spd = 0.5 + random() * 1.5;
            bright = 140 + floor(random() * 115);
        }
        var star = {
            pos: Vector2D(sx, sy),
            speed: spd,
            brightness: bright
        };
        stars[i] = star;
        // Use SimplexNoise for subtle color variation per star
        var nv = noise.Noise(sx * 0.1, sy * 0.1);
        var tint = floor(abs(nv) * 30);
        var b = star.brightness;
        starColors[i] = Color888(b, b - tint, floor(b * 0.9 + tint));
        i = i + 1;
    }
}

fn UpdateStars(dt) {
    var i = 0;
    while i < length(stars) {
        var sy = stars[i].pos.Y + stars[i].speed * dt;
        var sx = stars[i].pos.X;
        if sy > SH {
            sy = 0.0;
            sx = random() * SW;
        }
        stars[i].pos = Vector2D(sx, sy);
        i = i + 1;
    }
}

// -- Collision Detection -----------------------------
fn CheckCircleCollision(posA, radiusA, posB, radiusB) {
    var ca = Circle2D(posA, radiusA);
    var cb = Circle2D(posB, radiusB);
    return ca.OverlapsCircle(cb);
}

fn CheckTriangleCollision(tri, point) {
    // Triangle2D.IsInShape tests if a point is inside the triangle
    return tri.IsInShape(point);
}

// -- Signal Handlers ----------------------------------
fn OnEnemyKilled(x, y, kind) {
    if kind == 0 {
        score = score + 10;
    } else {
        score = score + 25;
    }
    displayScore = score;
    SpawnExplosion(x, y);
    TriggerScreenShake(3.0);
}

fn OnPlayerHit() {
    if player.TakeDamage() {
        SpawnExplosion(player.pos.X, player.pos.Y);
        TriggerScreenShake(6.0);
        if not player.IsAlive() {
            gameOver = true;
        }
    }
}

fn OnWaveComplete(waveNum) {
    print("Wave " + str(waveNum) + " complete! Score: " + str(score));
    start_coroutine("SpawnNextWave");
}

// -- Wave Spawning Coroutine --------------------------
fn SpawnNextWave() {
    waveSpawning = true;
    wave = wave + 1;
    // Pause between waves (120 frames = 2 seconds at 60fps)
    var waitFrames = 120;
    var f = 0;
    while f < waitFrames {
        f = f + 1;
        yield;
    }

    // Spawn enemies for this wave
    var count = 3 + wave * 2;
    if count > 20 { count = 20; }

    var speed = config.baseEnemySpeed + wave * 1.5;
    var i = 0;
    while i < count {
        var x = 5.0 + random() * (SW - 10.0);

        // Alternate between asteroids and fighters
        if i % 3 == 0 and wave > 1 {
            enemies[length(enemies)] = MakeFighter(x, speed);
        } else {
            enemies[length(enemies)] = MakeAsteroid(x, speed);
        }
        // Stagger spawns: wait a few frames between each enemy
        var delay = 0;
        while delay < 8 {
            delay = delay + 1;
            yield;
        }

        i = i + 1;
    }

    waveSpawning = false;
}

// -- Module Detection ---------------------------------
fn CheckModules() {
    if has_module("physics") {
        print("[modules] Physics module loaded");
    }
    if has_module("audio") {
        print("[modules] Audio module loaded");
    }
    if has_module("ai") {
        print("[modules] AI module loaded");
    }
    var mods = list_modules();
    print("[modules] " + str(length(mods)) + " modules active");
}

// -- Setup --------------------------------------------
fn Setup() {
    print("=== KoiloEngine Space Shooter ===");
    print("Controls: A/D = move, Space = shoot, R = restart, P = pause");

    // Check available modules
    CheckModules();

    // Connect signals to handlers
    connect("on_enemy_killed", "OnEnemyKilled");
    connect("on_player_hit", "OnPlayerHit");
    connect("on_wave_complete", "OnWaveComplete");

    // Initialize player
    player.Reset();

    // Initialize starfield
    InitStars();

    // Initialize HUD widgets
    InitHUD();

    // Enable profiler
    profiler_enable();

    // Start first wave
    start_coroutine("SpawnNextWave");

    print("Setup complete. Wave 1 starting...");
}

// -- Update Loop --------------------------------------
fn Update(dt) {
    if not gameOver and not paused {

    gameTime = gameTime + dt;

    // -- Player Movement --
    player.thrustActive = false;
    if moveLeft { player.MoveLeft(dt); player.thrustActive = true; }
    if moveRight { player.MoveRight(dt); player.thrustActive = true; }
    if moveUp { player.MoveUp(dt); }
    if moveDown { player.MoveDown(dt); }
    player.UpdatePhysics(dt);
    player.UpdateCooldown(dt);

    // -- Shooting --
    if shooting and player.CanShoot() {
        player.Shoot();
        var b = SpawnBullet(player.pos.X, player.pos.Y);
        bullets[length(bullets)] = b;
    }

    // -- Update Bullets --
    var bi = 0;
    while bi < length(bullets) {
        if bullets[bi].alive {
            bullets[bi].Update(dt);
        }
        bi = bi + 1;
    }

    // -- Update Enemies --
    var ei = 0;
    while ei < length(enemies) {
        if enemies[ei].alive {
            enemies[ei].Update(dt);
        }
        ei = ei + 1;
    }

    // -- Bullet-Enemy Collisions (Circle2D vs Circle2D) --
    bi = 0;
    while bi < length(bullets) {
        if bullets[bi].alive {
            ei = 0;
            while ei < length(enemies) {
                if enemies[ei].alive {
                    if CheckCircleCollision(
                        bullets[bi].pos, bullets[bi].radius,
                        enemies[ei].pos, enemies[ei].size
                    ) {
                        bullets[bi].alive = false;
                        if enemies[ei].TakeHit() {
                            emit on_enemy_killed(enemies[ei].pos.X, enemies[ei].pos.Y, enemies[ei].kind);
                        }
                    }
                }
                ei = ei + 1;
            }
        }
        bi = bi + 1;
    }

    // -- Enemy-Player Collisions (Circle2D vs Circle2D) --
    ei = 0;
    while ei < length(enemies) {
        if enemies[ei].alive {
            if CheckCircleCollision(player.pos, 2.5, enemies[ei].pos, enemies[ei].size) {
                enemies[ei].alive = false;
                emit on_player_hit();
                SpawnExplosion(enemies[ei].pos.X, enemies[ei].pos.Y);
            }
        }
        ei = ei + 1;
    }

    // -- Clean Dead Entities --
    var newBullets = [];
    bi = 0;
    while bi < length(bullets) {
        if bullets[bi].alive {
            newBullets[length(newBullets)] = bullets[bi];
        }
        bi = bi + 1;
    }
    bullets = newBullets;

    var newEnemies = [];
    ei = 0;
    while ei < length(enemies) {
        if enemies[ei].alive {
            newEnemies[length(newEnemies)] = enemies[ei];
        }
        ei = ei + 1;
    }
    enemies = newEnemies;

    // -- Check Wave Complete --
    if length(enemies) == 0 and not waveSpawning and wave > 0 {
        emit on_wave_complete(wave);
    }

    } // end gameplay guard

    // -- Effects & rendering (always active) --
    UpdateScreenShake(dt);
    UpdateStars(dt);

    // Update explosions
    var newExplosions = [];
    var xi = 0;
    while xi < length(explosions) {
        explosions[xi].Update(dt);
        if explosions[xi].alive {
            newExplosions[length(newExplosions)] = explosions[xi];
        }
        xi = xi + 1;
    }
    explosions = newExplosions;

    DrawGame();
}

// -- Input Handlers -----------------------------------
fn OnKeyPress(key) {
    if key == "a" or key == "left" { moveLeft = true; }
    if key == "d" or key == "right" { moveRight = true; }
    if key == "w" or key == "up" { moveUp = true; }
    if key == "s" or key == "down" { moveDown = true; }
    if key == "space" { shooting = true; }
    if key == "p" { paused = not paused; }
    if key == "r" { RestartGame(); }
    if key == "f1" { config.debugColliders = not config.debugColliders; }
}

fn OnKeyRelease(key) {
    if key == "a" or key == "left" { moveLeft = false; }
    if key == "d" or key == "right" { moveRight = false; }
    if key == "w" or key == "up" { moveUp = false; }
    if key == "s" or key == "down" { moveDown = false; }
    if key == "space" { shooting = false; }
}

fn RestartGame() {
    stop_all_coroutines();
    player.Reset();
    bullets = [];
    enemies = [];
    explosions = [];
    score = 0;
    wave = 0;
    gameOver = false;
    paused = false;
    gameTime = 0.0;
    waveSpawning = false;
    displayScore = 0;
    start_coroutine("SpawnNextWave");
    print("Game restarted!");
}

// -- HUD Drawing (3x5 canvas text) --------------------

fn InitHUD() {
    // No widgets needed - HUD is drawn directly on canvas
}

fn UpdateHUD() {
    // Score (top-left, white)
    canvas.DrawText(1, 2, str(floor(displayScore)), 200, 220, 255);

    // Wave number (top-right area)
    canvas.DrawText(44, 2, "W" + str(wave), 180, 180, 100);

    // Game Over / Paused overlays
    if gameOver {
        canvas.DrawText(14, 18, "GAME OVER", 255, 80, 80);
        canvas.DrawText(18, 26, "PRESS R", 180, 180, 180);
    }
    if paused {
        canvas.DrawText(16, 20, "PAUSED", 255, 255, 255);
    }
}

// -- Render -------------------------------------------
fn DrawGame() {
    // Clear to deep space blue
    canvas.ClearWithColor(4, 4, 16);

    // -- Screen shake offset --
    var shakeOx = floor(screenShakeX);
    var shakeOy = floor(screenShakeY);

    // -- Stars -- (using pre-built Color888 objects)
    var si = 0;
    while si < length(stars) {
        var sx = floor(stars[si].pos.X) + shakeOx;
        var sy = floor(stars[si].pos.Y) + shakeOy;
        var sc = starColors[si];
        canvas.SetPixel(sx, sy, sc.r, sc.g, sc.b);
        si = si + 1;
    }

    // -- Bullets -- (bright yellow dots)
    var bi = 0;
    while bi < length(bullets) {
        if bullets[bi].alive {
            var bx = floor(bullets[bi].pos.X) + shakeOx;
            var by = floor(bullets[bi].pos.Y) + shakeOy;
            canvas.FillRect(bx, by, 1, 2, 255, 255, 80);
        }
        bi = bi + 1;
    }

    // -- Enemies --
    var ei = 0;
    while ei < length(enemies) {
        if enemies[ei].alive {
            var ex = floor(enemies[ei].pos.X) + shakeOx;
            var ey = floor(enemies[ei].pos.Y) + shakeOy;
            var sz = floor(enemies[ei].size);
            if enemies[ei].kind == 0 {
                // Asteroid - gray/brown
                canvas.FillRect(ex - sz, ey - sz, sz * 2, sz * 2, 140, 120, 90);
                canvas.DrawRect(ex - sz, ey - sz, sz * 2, sz * 2, 180, 160, 120);
            } else {
                // Fighter - red cross shape
                canvas.FillRect(ex - sz, ey - 1, sz * 2, 3, 220, 40, 40);
                canvas.FillRect(ex - 1, ey - sz, 2, sz * 2, 220, 40, 40);
            }
        }
        ei = ei + 1;
    }

    // -- Explosions -- (Color888.Lerp blended colors)
    var xi = 0;
    while xi < length(explosions) {
        if explosions[xi].alive {
            var prog = explosions[xi].GetProgress();
            var radius = floor(1 + prog * 4);
            var ex2 = floor(explosions[xi].pos.X) + shakeOx;
            var ey2 = floor(explosions[xi].pos.Y) + shakeOy;
            var col = explosions[xi].GetColor();
            canvas.DrawCircle(ex2, ey2, radius, col.r, col.g, col.b);
            if radius > 1 {
                // Inner ring brighter
                var inner = Color888.Lerp(col, explosionColorInner, 0.5);
                canvas.DrawCircle(ex2, ey2, radius - 1, inner.r, inner.g, inner.b);
            }
        }
        xi = xi + 1;
    }

    // -- Player -- (draw last so it's on top)
    if not gameOver {
        var px = floor(player.pos.X) + shakeOx;
        var py = floor(player.pos.Y) + shakeOy;

        // Only draw if visible (handles invincibility blink)
        if player.visible {
            // Ship body - cyan/teal triangle shape
            canvas.FillRect(px - 1, py - 2, 3, 4, 60, 200, 220);
            // Nose
            canvas.SetPixel(px, py - 3, 120, 255, 255);
            // Wings
            canvas.SetPixel(px - 2, py + 1, 40, 150, 180);
            canvas.SetPixel(px + 2, py + 1, 40, 150, 180);

            // Thrust flame (uses Mathematics.PingPong for flicker)
            if player.thrustActive {
                var flicker = Mathematics.PingPong(gameTime * 8.0);
                var flameR = 255;
                var flameG = floor(140 + flicker * 115);
                canvas.SetPixel(px, py + 2, flameR, flameG, 0);
                canvas.SetPixel(px, py + 3, 255, 100, 0);
            }
        }
    }

    // -- HUD background bar + lives dots --
    canvas.FillRect(0, 0, SW, 7, 8, 8, 24);
    var li = 0;
    while li < player.lives {
        canvas.FillRect(SW - 4 - li * 3, 2, 2, 2, 60, 220, 60);
        li = li + 1;
    }

    // -- HUD text (canvas DrawText) --
    UpdateHUD();
}
