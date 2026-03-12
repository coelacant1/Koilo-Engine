// ui_demo_kml.ks - Koilo Engine Editor UI (KML Version)
// ======================================================
// Full editor layout built declaratively from KML markup
// + KSS stylesheet. Showcases all UI system features:
//
//   Rendering:  SDF rounded rects, z-index, scrollbars, tree arrows
//   Layout:     Flex-grow, space-between, position absolute, overlays
//   Styling:    CSS variables, gradients, box-shadow, transitions
//   Selectors:  :first-child, :last-child, :not(), :checked, .class
//   Text:       font-weight, text-overflow, letter-spacing
//   Responsive: @media queries, relative units, calc()
//   Pseudo:     ::before/::after content generation
//
// Layout loaded from:
//   assets/ui/editor.kml  - widget hierarchy
//   assets/ui/editor.kss  - dark theme styling
// ======================================================

// -- Display Configuration ----------------------------
display.SetType("desktop");
display.SetWidth(1280);
display.SetHeight(720);
display.SetPixelWidth(1280);
display.SetPixelHeight(720);
display.SetTargetFPS(60);
display.SetCapFPS(true);

// -- State Variables ----------------------------------
var frameCount = 0;
var fpsDisplay = 0;
var fpsAccum = 0.0;
var selectedNode = "None";
var activeTool = "Select";

// Widget indices (resolved after KML load)
var fpsLabel = -1;
var statusText = -1;

// =================================================================
// SETUP - load layout from KML files
// =================================================================
fn Setup() {
    var result = ui.LoadMarkup("assets/ui/editor.kml", "assets/ui/editor.kss");
    if (result < 0) {
        var lbl = ui.CreateLabel("err", "Failed to load KML layout");
        ui.SetParent(lbl, ui.GetRoot());
        return;
    }

    // Cache widget references for per-frame updates
    fpsLabel = ui.FindWidget("fps-counter");
    statusText = ui.FindWidget("status-text");
}

// =================================================================
// EVENT HANDLERS
// =================================================================

fn OnMenuFile()    { ui.SetText(statusText, "File menu opened"); }
fn OnMenuEdit()    { ui.SetText(statusText, "Edit menu opened"); }
fn OnMenuView()    { ui.SetText(statusText, "View menu opened"); }
fn OnMenuTools()   { ui.SetText(statusText, "Tools menu opened"); }
fn OnMenuHelp()    { ui.SetText(statusText, "Help - Koilo Engine v0.1"); }

fn OnPlay()        { ui.SetText(statusText, "▶ Play mode started"); }
fn OnPause()       { ui.SetText(statusText, "⏸ Play mode paused"); }
fn OnStop()        { ui.SetText(statusText, "⏹ Play mode stopped"); }

fn OnToolSelect()  { activeTool = "Select";  ui.SetText(statusText, "Tool: Select"); }
fn OnToolMove()    { activeTool = "Move";    ui.SetText(statusText, "Tool: Move"); }
fn OnToolRotate()  { activeTool = "Rotate";  ui.SetText(statusText, "Tool: Rotate"); }
fn OnToolScale()   { activeTool = "Scale";   ui.SetText(statusText, "Tool: Scale"); }
fn OnToolTerrain() { activeTool = "Terrain"; ui.SetText(statusText, "Tool: Terrain"); }
fn OnToolPaint()   { activeTool = "Paint";   ui.SetText(statusText, "Tool: Paint"); }

fn OnAddNode()     { ui.SetText(statusText, "Add node dialog"); }
fn OnAddComponent(){ ui.SetText(statusText, "Add component dialog"); }

// -- Per-frame update ---------------------------------
fn Update(dt) {
    frameCount = frameCount + 1;
    fpsAccum = fpsAccum + dt;

    if (frameCount % 30 == 0) {
        if (fpsAccum > 0) {
            fpsDisplay = 30.0 / fpsAccum;
        }
        fpsAccum = 0.0;
        if (fpsLabel >= 0) {
            ui.SetText(fpsLabel, "" + fpsDisplay + " FPS");
        }
    }
}
