// weapons.ks  Bullet class
// Demonstrates: classes, simple physics-like movement
// Backend classes: Vector2D (pos/vel)

class Bullet {
    var pos = Vector2D(0, 0);
    var vel = Vector2D(0, -60);
    var alive = true;
    var radius = 1.0;

    fn Update(dt) {
        self.pos = Vector2D(self.pos.X + self.vel.X * dt, self.pos.Y + self.vel.Y * dt);
        if self.pos.Y < -2.0 {
            self.alive = false;
        }
    }
}

fn SpawnBullet(x, y) {
    var b = new Bullet();
    b.pos = Vector2D(x, y - 1.0);
    return b;
}
