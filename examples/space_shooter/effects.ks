// effects.ks  Visual effect helpers
// Demonstrates: tables, math functions, signal handlers
// Backend classes: Color888 (Lerp for fading), DampedSpring (screen shake),
//                  Vector2D (explosion pos), Mathematics (SmoothStep)

// Screen shake via DampedSpring (realistic spring decay)
var shakeSpringX = DampedSpring(25.0, 0.6);
var shakeSpringY = DampedSpring(25.0, 0.6);
var screenShakeX = 0.0;
var screenShakeY = 0.0;

fn TriggerScreenShake(intensity) {
    // Kick the springs with random offsets
    screenShakeX = (random() - 0.5) * intensity;
    screenShakeY = (random() - 0.5) * intensity;
}

fn UpdateScreenShake(dt) {
    // DampedSpring returns toward target=0, giving natural decay
    screenShakeX = shakeSpringX.Calculate(screenShakeX, dt);
    screenShakeY = shakeSpringY.Calculate(screenShakeY, dt);
}

// Explosion particle data
var explosions = [];

// Explosion colors  backend Color888 objects for Lerp blending
var explosionColorInner = Color888(255, 255, 200);
var explosionColorMid   = Color888(255, 160, 40);
var explosionColorOuter = Color888(180, 40, 0);

class Explosion {
    var pos = Vector2D(0, 0);
    var timer = 0.5;
    var maxTimer = 0.5;
    var alive = true;

    fn Update(dt) {
        self.timer = self.timer - dt;
        if self.timer <= 0.0 {
            self.alive = false;
        }
    }

    fn GetProgress() {
        return 1.0 - (self.timer / self.maxTimer);
    }

    fn GetColor() {
        var prog = self.GetProgress();
        // Blend from inner (white-yellow) to outer (dark red) via Color888.Lerp
        if prog < 0.5 {
            return Color888.Lerp(explosionColorInner, explosionColorMid, prog * 2.0);
        }
        return Color888.Lerp(explosionColorMid, explosionColorOuter, (prog - 0.5) * 2.0);
    }
}

fn SpawnExplosion(x, y) {
    var exp = new Explosion();
    exp.pos = Vector2D(x, y);
    explosions[length(explosions)] = exp;
}
