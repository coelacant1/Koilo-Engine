// demo_all_modules.ks - Combined demo exercising all three ELF modules
// Loaded by the modules example to verify sensor, game_mode, and fx modules.

display.SetType("desktop");
display.SetWidth(640);
display.SetHeight(360);
display.SetPixelWidth(192);
display.SetPixelHeight(108);
display.SetTargetFPS(30);
display.SetCapFPS(true);

var canvas = Canvas2D();
canvas.Resize(192, 108);
canvas.Scale(640, 360);
canvas.Attach();

var frameCount = 0;
var fxTimer = 0;

fn Setup() {
    fx_mode = 0;
    print("[all_modules] Loaded - testing sensor + game_mode + fx");
}

fn Update(dt) {
    frameCount = frameCount + 1;
    fxTimer = fxTimer + dt;

    // -- Sensor module: read simulated hardware data --------------
    var temp = sensor_temperature;
    var dist = sensor_distance;
    var lux  = sensor_light;

    // -- Game mode module: score/lives management -----------------
    if frameCount % 60 == 0 {
        game_add_score = 50;
    }
    if frameCount % 300 == 0 {
        game_lose_life = 1;
    }
    if game_over > 0 {
        game_reset = 1;
    }

    // -- FX module: cycle post-processing effects -----------------
    if fxTimer > 2.0 {
        fx_mode = fx_mode + 1;
        if fx_mode > 3 {
            fx_mode = 0;
        }
        fxTimer = 0;
    }

    // -- HUD ------------------------------------------------------
    canvas.Clear();
    canvas.DrawText(2, 2,  "=== Module Demo ===", 255, 255, 255);
    canvas.DrawText(2, 14, "Temp:" + str(round(temp)) + "C Dist:" + str(round(dist)) + "cm", 100, 220, 255);
    canvas.DrawText(2, 24, "Light: " + str(round(lux * 100)) + "%", 100, 220, 255);
    canvas.DrawText(2, 36, "Score:" + str(game_score) + " Lives:" + str(game_lives), 255, 220, 40);
    canvas.DrawText(2, 46, "Timer:" + str(round(game_timer)) + "s", 255, 220, 40);
    canvas.DrawText(2, 58, "FX mode: " + str(fx_mode), 180, 255, 180);

    // Periodic console log
    if frameCount % 90 == 0 {
        print("Sensor t=" + str(round(temp)) + " d=" + str(round(dist)) + " | Score=" + str(game_score) + " Lives=" + str(game_lives) + " | FX=" + str(fx_mode));
    }
}
