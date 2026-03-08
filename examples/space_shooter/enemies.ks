// enemies.ks  Enemy and asteroid classes with wave spawning
// Demonstrates: classes, arrays, random(), coroutines for wave timing
// Backend classes: Vector2D (pos/vel), AABB2D (bounds), FunctionGenerator (sine wave),
//                  BouncePhysics (asteroid wall bounces)

// Screen bounds for off-screen detection
var screenBounds = AABB2D(Vector2D(-3, -3), Vector2D(display.GetPixelWidth() + 3, display.GetPixelHeight() + 3));
var screenW = display.GetPixelWidth();

class Enemy {
    var pos = Vector2D(0, 0);
    var vel = Vector2D(0, 0);
    var alive = true;
    var kind = 0;
    var size = 1.5;
    var hp = 1;

    // Fighter sine-wave state
    var waveGen = 0;
    var baseX = 0.0;

    // Asteroid bounce physics
    var bouncer = 0;

    fn Update(dt) {
        var px = self.pos.X;
        var py = self.pos.Y;

        if self.kind == 1 and self.waveGen != 0 {
            // Fighter: use FunctionGenerator for sine-wave X oscillation
            var waveVal = self.waveGen.Update();
            px = self.baseX + waveVal;
            self.baseX = self.baseX + self.vel.X * dt;
        } else if self.bouncer != 0 {
            // Asteroid: use BouncePhysics for wall bounce behavior
            px = px + self.vel.X * dt;

            // Bounce off screen edges
            if px < 1.0 or px > screenW - 1.0 {
                var newVx = self.bouncer.Calculate(-self.vel.X, dt);
                self.vel = Vector2D(newVx, self.vel.Y);
                px = clamp(px, 1.0, screenW - 1.0);
            }
        } else {
            px = px + self.vel.X * dt;
        }

        py = py + self.vel.Y * dt;
        self.pos = Vector2D(px, py);

        // Off-screen detection via AABB2D.Contains()
        if not screenBounds.Contains(self.pos) {
            if py > display.GetPixelHeight() + 2.0 {
                self.alive = false;
            } else {
                // Wrap horizontally
                if px < -2.0 { px = screenW + 2.0; self.baseX = px; }
                if px > screenW + 2.0 { px = -2.0; self.baseX = px; }
                self.pos = Vector2D(px, py);
            }
        }
    }

    fn TakeHit() {
        self.hp = self.hp - 1;
        if self.hp <= 0 {
            self.alive = false;
            return true;
        }
        return false;
    }
}

fn MakeAsteroid(x, speed) {
    var e = new Enemy();
    e.pos = Vector2D(x, -3.0);
    e.vel = Vector2D((random() - 0.5) * 8.0, speed + random() * 5.0);
    e.kind = 0;
    e.size = 1.5 + random() * 2.0;
    e.hp = 1;
    // BouncePhysics: gravity=0 (no vertical pull), velocityRatio=0.85 (damped bounces)
    e.bouncer = BouncePhysics(0.0, 0.85);
    return e;
}

fn MakeFighter(x, speed) {
    var e = new Enemy();
    e.pos = Vector2D(x, -3.0);
    e.vel = Vector2D(0.0, speed);
    e.baseX = x;
    e.kind = 1;
    e.size = 1.2;
    e.hp = 2;
    // FunctionGenerator: Sine wave, amplitude ±(6-14), period 1-2.5 seconds
    var amp = 6.0 + random() * 8.0;
    e.waveGen = FunctionGenerator(Sine, -amp, amp, 1.0 + random() * 1.5);
    return e;
}
