// player.ks  Player ship class and movement logic
// Demonstrates: classes, self, methods, constructor patterns
// Backend classes: Vector2D (pos/vel), Mathematics (Lerp), Triangle2D (collision shape)

class Player {
    var pos = Vector2D(0, 0);
    var vel = Vector2D(0, 0);
    var speed = 40.0;
    var accel = 12.0;
    var shootCooldown = 0.0;
    var shootRate = 0.2;
    var lives = 3;
    var invincibleTimer = 0.0;
    var visible = true;
    var thrustActive = false;

    fn MoveLeft(dt) {
        var vx = Mathematics.Lerp(self.vel.X, -self.speed, self.accel * dt);
        self.vel = Vector2D(vx, self.vel.Y);
    }

    fn MoveRight(dt) {
        var vx = Mathematics.Lerp(self.vel.X, self.speed, self.accel * dt);
        self.vel = Vector2D(vx, self.vel.Y);
    }

    fn MoveUp(dt) {
        var vy = Mathematics.Lerp(self.vel.Y, -self.speed * 0.5, self.accel * dt);
        self.vel = Vector2D(self.vel.X, vy);
    }

    fn MoveDown(dt) {
        var vy = Mathematics.Lerp(self.vel.Y, self.speed * 0.5, self.accel * dt);
        self.vel = Vector2D(self.vel.X, vy);
    }

    fn UpdatePhysics(dt) {
        // Apply velocity
        self.pos = Vector2D(self.pos.X + self.vel.X * dt, self.pos.Y + self.vel.Y * dt);

        // Decelerate when not moving
        var dvx = Mathematics.Lerp(self.vel.X, 0.0, 6.0 * dt);
        var dvy = Mathematics.Lerp(self.vel.Y, 0.0, 6.0 * dt);
        self.vel = Vector2D(dvx, dvy);

        // Constrain to screen bounds
        var cx = clamp(self.pos.X, 1.0, display.GetPixelWidth() - 2.0);
        var cy = clamp(self.pos.Y, 10.0, display.GetPixelHeight() - 6.0);
        self.pos = Vector2D(cx, cy);
    }

    fn GetCollisionShape() {
        var px = self.pos.X;
        var py = self.pos.Y;
        var nose = Vector2D(px, py - 3);
        var left = Vector2D(px - 2, py + 2);
        var right = Vector2D(px + 2, py + 2);
        return Triangle2D(nose, left, right);
    }

    fn CanShoot() {
        return self.shootCooldown <= 0.0;
    }

    fn Shoot() {
        self.shootCooldown = self.shootRate;
    }

    fn UpdateCooldown(dt) {
        if self.shootCooldown > 0.0 {
            self.shootCooldown = self.shootCooldown - dt;
        }
        if self.invincibleTimer > 0.0 {
            self.invincibleTimer = self.invincibleTimer - dt;
            self.visible = floor(self.invincibleTimer * 10) % 2 == 0;
        } else {
            self.visible = true;
        }
    }

    fn TakeDamage() {
        if self.invincibleTimer <= 0.0 {
            self.lives = self.lives - 1;
            self.invincibleTimer = 2.0;
            return true;
        }
        return false;
    }

    fn IsAlive() {
        return self.lives > 0;
    }

    fn Reset() {
        self.pos = Vector2D(floor(display.GetPixelWidth() / 2), display.GetPixelHeight() - 8.0);
        self.vel = Vector2D(0, 0);
        self.lives = 3;
        self.invincibleTimer = 0.0;
        self.shootCooldown = 0.0;
    }
}
