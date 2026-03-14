// ui_showcase.ks - Koilo Engine UI Widget Showcase
// =================================================
// Loads showcase.kml + showcase.kss and demonstrates
// every widget type for visual debugging.
//
// Layout loaded from:
//   assets/ui/showcase.kml  - widget hierarchy (26 sections)
//   assets/ui/showcase.kss  - dark theme stylesheet
// =================================================

// -- Display Configuration ----------------------------
display.SetType("desktop");
display.SetWidth(1280);
display.SetHeight(800);
display.SetPixelWidth(1280);
display.SetPixelHeight(800);
display.SetTargetFPS(60);
display.SetCapFPS(true);

// -- State Variables ----------------------------------
var frameCount  = 0;
var fpsDisplay  = 0;
var fpsAccum    = 0.0;

// Widget indices (resolved after KML load)
var fpsLabel       = -1;
var slVolumeVal    = -1;
var slOpacityVal   = -1;
var slRangeVal     = -1;

// Tab widgets
var tabContent     = -1;
var tabDetail      = -1;

// Color hex preview widget
var cfHexPreview   = -1;
var tfHex          = -1;

// Nav button widgets
var navBasic       = -1;
var navInput       = -1;
var navLayout      = -1;
var navTree        = -1;
var navAdvanced    = -1;
var navStates      = -1;
var navText        = -1;

// Tab button widgets
var tabGeneral     = -1;
var tabRendering   = -1;
var tabPhysics     = -1;
var tabAudio       = -1;
var tabScripting   = -1;

// Page group widgets
var pageBasic      = -1;
var pageInput      = -1;
var pageLayout     = -1;
var pageTree       = -1;
var pageAdvanced   = -1;
var pageStates     = -1;
var pageText       = -1;


// =================================================================
// SETUP - load layout from KML + KSS
// =================================================================
fn Setup() {
    var result = ui.LoadMarkup("assets/ui/showcase.kml", "assets/ui/showcase.kss");
    if (result < 0) {
        var lbl = ui.CreateLabel("err", "Failed to load showcase.kml - check file paths");
        ui.SetParent(lbl, ui.GetRoot());
        return;
    }

    // Cache widget references for per-frame updates
    fpsLabel    = ui.FindWidget("fps-label");
    slVolumeVal = ui.FindWidget("sl-volume-val");
    slOpacityVal= ui.FindWidget("sl-opacity-val");
    slRangeVal  = ui.FindWidget("sl-range-val");

    // Cache tab widgets
    tabContent   = ui.FindWidget("tab-content");
    tabDetail    = ui.FindWidget("tab-detail");

    // Cache color hex widgets
    cfHexPreview = ui.FindWidget("cf-hex-preview");
    tfHex        = ui.FindWidget("tf-hex");

    // Cache nav button widgets
    navBasic     = ui.FindWidget("nav-basic");
    navInput     = ui.FindWidget("nav-input");
    navLayout    = ui.FindWidget("nav-layout");
    navTree      = ui.FindWidget("nav-tree");
    navAdvanced  = ui.FindWidget("nav-advanced");
    navStates    = ui.FindWidget("nav-states");
    navText      = ui.FindWidget("nav-text");

    // Cache tab button widgets
    tabGeneral   = ui.FindWidget("tab-general");
    tabRendering = ui.FindWidget("tab-rendering");
    tabPhysics   = ui.FindWidget("tab-physics");
    tabAudio     = ui.FindWidget("tab-audio");
    tabScripting = ui.FindWidget("tab-scripting");

    // Set initial active states
    ui.SetSelected(navBasic, true);
    SelectTab(tabGeneral);

    // Cache page group widgets
    pageBasic    = ui.FindWidget("page-basic");
    pageInput    = ui.FindWidget("page-input");
    pageLayout   = ui.FindWidget("page-layout");
    pageTree     = ui.FindWidget("page-tree");
    pageAdvanced = ui.FindWidget("page-advanced");
    pageStates   = ui.FindWidget("page-states");
    pageText     = ui.FindWidget("page-text");

    SetupShortcuts();
}

// =================================================================
// HELPER: show one page, hide all others + update nav selection
// =================================================================
fn ShowPage(activePageIdx, activeNavIdx) {
    ui.SetVisible(pageBasic,    pageBasic    == activePageIdx);
    ui.SetVisible(pageInput,    pageInput    == activePageIdx);
    ui.SetVisible(pageLayout,   pageLayout   == activePageIdx);
    ui.SetVisible(pageTree,     pageTree     == activePageIdx);
    ui.SetVisible(pageAdvanced, pageAdvanced == activePageIdx);
    ui.SetVisible(pageStates,   pageStates   == activePageIdx);
    ui.SetVisible(pageText,     pageText     == activePageIdx);

    // Update nav button selection
    ui.SetSelected(navBasic,    navBasic    == activeNavIdx);
    ui.SetSelected(navInput,    navInput    == activeNavIdx);
    ui.SetSelected(navLayout,   navLayout   == activeNavIdx);
    ui.SetSelected(navTree,     navTree     == activeNavIdx);
    ui.SetSelected(navAdvanced, navAdvanced == activeNavIdx);
    ui.SetSelected(navStates,   navStates   == activeNavIdx);
    ui.SetSelected(navText,     navText     == activeNavIdx);
}

fn SelectTab(activeTabIdx) {
    ui.SetSelected(tabGeneral,   tabGeneral   == activeTabIdx);
    ui.SetSelected(tabRendering, tabRendering == activeTabIdx);
    ui.SetSelected(tabPhysics,   tabPhysics   == activeTabIdx);
    ui.SetSelected(tabAudio,     tabAudio     == activeTabIdx);
    ui.SetSelected(tabScripting, tabScripting == activeTabIdx);
}

// =================================================================
// EVENT HANDLERS
// =================================================================

// Shortcut status label
var shortcutStatus = -1;

fn SetupShortcuts() {
    shortcutStatus = ui.FindWidget("shortcut-status");
}

fn OnMenuNew()   { if (shortcutStatus >= 0) { ui.SetText(shortcutStatus, "Action: New Scene (Ctrl+N)"); } }
fn OnMenuOpen()  { if (shortcutStatus >= 0) { ui.SetText(shortcutStatus, "Action: Open... (Ctrl+O)"); } }
fn OnMenuSave()  { if (shortcutStatus >= 0) { ui.SetText(shortcutStatus, "Action: Save (Ctrl+S)"); } }
fn OnMenuUndo()  { if (shortcutStatus >= 0) { ui.SetText(shortcutStatus, "Action: Undo (Ctrl+Z)"); } }
fn OnMenuRedo()  { if (shortcutStatus >= 0) { ui.SetText(shortcutStatus, "Action: Redo (Ctrl+Y)"); } }
fn OnMenuPrefs() { if (shortcutStatus >= 0) { ui.SetText(shortcutStatus, "Action: Preferences (Ctrl+,)"); } }

// Color hex input handler
fn OnHexChange() {
    if (tfHex >= 0 and cfHexPreview >= 0) {
        var hex = ui.GetText(tfHex);
        ui.SetColorHex(cfHexPreview, hex);
    }
}

// Navigation sidebar buttons
fn OnNavBasic()    { ShowPage(pageBasic,    navBasic);    }
fn OnNavInput()    { ShowPage(pageInput,    navInput);    }
fn OnNavLayout()   { ShowPage(pageLayout,   navLayout);   }
fn OnNavTree()     { ShowPage(pageTree,     navTree);     }
fn OnNavAdvanced() { ShowPage(pageAdvanced, navAdvanced); }
fn OnNavStates()   { ShowPage(pageStates,   navStates);   }
fn OnNavText()     { ShowPage(pageText,     navText);     }

// Tab bar switching
fn OnTabGeneral() {
    ui.SetText(tabContent, "General settings - resolution, vsync, UI scale.");
    ui.SetText(tabDetail, "Configure basic application parameters.");
    SelectTab(tabGeneral);
}

fn OnTabRendering() {
    ui.SetText(tabContent, "Rendering - shadows, AA, LOD, draw distance.");
    ui.SetText(tabDetail, "Fine-tune graphics quality and performance.");
    SelectTab(tabRendering);
}

fn OnTabPhysics() {
    ui.SetText(tabContent, "Physics - gravity, timestep, collision layers.");
    ui.SetText(tabDetail, "Adjust simulation parameters.");
    SelectTab(tabPhysics);
}

fn OnTabAudio() {
    ui.SetText(tabContent, "Audio - master volume, spatial audio, reverb.");
    ui.SetText(tabDetail, "Control sound system settings.");
    SelectTab(tabAudio);
}

fn OnTabScripting() {
    ui.SetText(tabContent, "Scripting - hot reload, debug output, profiling.");
    ui.SetText(tabDetail, "Developer scripting options.");
    SelectTab(tabScripting);
}

// =================================================================
// PER-FRAME UPDATE
// =================================================================
fn Update(dt) {
    frameCount = frameCount + 1;
    fpsAccum   = fpsAccum + dt;

    // Update FPS counter every 30 frames
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
