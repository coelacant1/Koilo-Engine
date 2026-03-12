// ui_demo.ks - Koilo Engine Editor UI Showcase
// ==============================================
// Full editor layout demonstrating every widget type.
// Built entirely from KoiloScript using the runtime UI API.
//
// Layout:
//   Menu bar (File | Edit | View | Tools | Help)
//   Dock area:
//     Left:   Hierarchy panel (scene tree)
//     Center: Viewport + Console
//     Right:  Inspector panel (property editor)
//   Status bar (FPS | Widgets | Tool)
//
// Features demonstrated:
//   Panel, Label, Button, TextField, Slider, Checkbox,
//   ScrollView, Separator, TreeNode, DockContainer,
//   SplitPane, TabBar, MenuItem,
//   Localization, Accessibility, Animation, Theming
// ==============================================

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
var logCount = 0;

// Widget indices (set during Setup)
var fpsLabel = -1;
var widgetLabel = -1;
var statusTool = -1;
var consoleLog = -1;
var inspScroll = -1;

// Inspector property widgets
var inspNameField = -1;
var inspPosX = -1;
var inspPosY = -1;
var inspPosZ = -1;
var inspRotX = -1;
var inspRotY = -1;
var inspRotZ = -1;
var inspScaleX = -1;
var inspScaleY = -1;
var inspScaleZ = -1;
var inspVisible = -1;
var inspStatic = -1;
var inspCastShadow = -1;
var inspColorR = -1;
var inspColorG = -1;
var inspColorB = -1;
var inspIntensity = -1;
var inspNoSel = -1;
var inspNameLabel = -1;

// =================================================================
// SETUP
// =================================================================
fn Setup() {
    // -- Theme (dark editor) ------------------------------
    ui.SetThemeColor("panel", 38, 38, 42, 255);
    ui.SetThemeColor("button", 60, 60, 70, 255);
    ui.SetThemeColor("label", 38, 38, 42, 0);
    ui.SetThemeColor("textfield", 28, 28, 32, 255);
    ui.SetThemeColor("slider", 50, 50, 58, 255);
    ui.SetThemeColor("separator", 55, 55, 60, 255);
    ui.SetThemeColor("dock", 30, 30, 34, 255);
    ui.SetThemeColor("split", 34, 34, 38, 255);
    ui.SetThemeColor("tab", 44, 44, 50, 255);
    ui.SetThemeColor("popup", 48, 48, 55, 255);
    ui.SetThemeColor("menuitem", 48, 48, 55, 255);
    ui.SetThemeColor("checkbox", 50, 50, 58, 255);
    ui.SetThemeColor("treenode", 38, 38, 42, 0);
    ui.SetThemeColor("scrollview", 32, 32, 36, 255);

    // -- Localization -------------------------------------
    ui.SetLocale("en");
    ui.SetLocalizedString("menu.file", "File");
    ui.SetLocalizedString("menu.edit", "Edit");
    ui.SetLocalizedString("menu.view", "View");
    ui.SetLocalizedString("menu.tools", "Tools");
    ui.SetLocalizedString("menu.help", "Help");
    ui.SetLocalizedString("panel.hierarchy", "Hierarchy");
    ui.SetLocalizedString("panel.inspector", "Inspector");
    ui.SetLocalizedString("panel.console", "Console");
    ui.SetLocalizedString("panel.viewport", "Viewport");
    ui.SetLocalizedString("tool.select", "Select");
    ui.SetLocalizedString("tool.move", "Move");
    ui.SetLocalizedString("tool.rotate", "Rotate");
    ui.SetLocalizedString("tool.scale", "Scale");

    var root = ui.GetRoot();
    ui.SetLayoutColumn(root, 0);

    // ═══════════════════════════════════════════════════════
    //  MENU BAR
    // ═══════════════════════════════════════════════════════
    var menuBar = ui.CreatePanel("menuBar");
    ui.SetParent(menuBar, root);
    ui.SetLayoutRow(menuBar, 4);
    ui.SetSize(menuBar, 0, 32);
    ui.SetFillWidth(menuBar);
    ui.SetPadding(menuBar, 4, 8, 4, 8);

    var btnFile = ui.CreateButton("btn_file", ui.L("menu.file"));
    ui.SetParent(btnFile, menuBar);
    ui.SetSize(btnFile, 60, 24);
    ui.SetOnClick(btnFile, "OnMenuFile");

    var btnEdit = ui.CreateButton("btn_edit", ui.L("menu.edit"));
    ui.SetParent(btnEdit, menuBar);
    ui.SetSize(btnEdit, 60, 24);
    ui.SetOnClick(btnEdit, "OnMenuEdit");

    var btnView = ui.CreateButton("btn_view", ui.L("menu.view"));
    ui.SetParent(btnView, menuBar);
    ui.SetSize(btnView, 60, 24);
    ui.SetOnClick(btnView, "OnMenuView");

    var btnTools = ui.CreateButton("btn_tools", ui.L("menu.tools"));
    ui.SetParent(btnTools, menuBar);
    ui.SetSize(btnTools, 60, 24);
    ui.SetOnClick(btnTools, "OnMenuTools");

    var btnHelp = ui.CreateButton("btn_help", ui.L("menu.help"));
    ui.SetParent(btnHelp, menuBar);
    ui.SetSize(btnHelp, 60, 24);
    ui.SetOnClick(btnHelp, "OnMenuHelp");

    // Spacer then play controls
    var playPanel = ui.CreatePanel("play_panel");
    ui.SetParent(playPanel, menuBar);
    ui.SetLayoutRow(playPanel, 2);
    ui.SetSize(playPanel, 200, 24);

    var btnPlay = ui.CreateButton("btn_play", "Play");
    ui.SetParent(btnPlay, playPanel);
    ui.SetSize(btnPlay, 56, 22);
    ui.SetOnClick(btnPlay, "OnPlay");

    var btnPause = ui.CreateButton("btn_pause", "Pause");
    ui.SetParent(btnPause, playPanel);
    ui.SetSize(btnPause, 56, 22);
    ui.SetOnClick(btnPause, "OnPause");

    var btnStop = ui.CreateButton("btn_stop", "Stop");
    ui.SetParent(btnStop, playPanel);
    ui.SetSize(btnStop, 56, 22);
    ui.SetOnClick(btnStop, "OnStop");

    // ═══════════════════════════════════════════════════════
    //  DOCK AREA
    // ═══════════════════════════════════════════════════════
    var dockArea = ui.CreateDockContainer("dock");
    ui.SetParent(dockArea, root);
    ui.SetLayoutRow(dockArea, 2);
    ui.SetFillWidth(dockArea);
    ui.SetFillHeight(dockArea);
    ui.SetPadding(dockArea, 2, 2, 2, 2);

    // --- LEFT PANE: Hierarchy ---------------------------
    var leftPane = ui.CreateSplitPane("left_pane", false);
    ui.SetParent(leftPane, dockArea);
    ui.SetLayoutColumn(leftPane, 4);
    ui.SetSize(leftPane, 220, 0);
    ui.SetFillHeight(leftPane);
    ui.SetPadding(leftPane, 6, 6, 6, 6);

    var hierTitle = ui.CreateLabel("hier_title", ui.L("panel.hierarchy"));
    ui.SetParent(hierTitle, leftPane);
    ui.SetAriaLabel(hierTitle, "Hierarchy panel title");

    var hierSearch = ui.CreateTextField("hier_search", "Search...");
    ui.SetParent(hierSearch, leftPane);
    ui.SetSize(hierSearch, 208, 22);
    ui.SetFillWidth(hierSearch);

    ui.CreateSeparator("hier_sep");
    ui.SetParent(ui.FindWidget("hier_sep"), leftPane);

    var hierScroll = ui.CreateScrollView("hier_scroll");
    ui.SetParent(hierScroll, leftPane);
    ui.SetFillWidth(hierScroll);
    ui.SetFillHeight(hierScroll);

    // Scene tree - nested objects
    var nScene = ui.CreateTreeNode("n_scene", "Scene");
    ui.SetParent(nScene, hierScroll);
    ui.SetOnClick(nScene, "OnSelectScene");

    var nCamera = ui.CreateTreeNode("n_camera", "  MainCamera");
    ui.SetParent(nCamera, hierScroll);
    ui.SetOnClick(nCamera, "OnSelectCamera");

    var nDirLight = ui.CreateTreeNode("n_dirlight", "  DirectionalLight");
    ui.SetParent(nDirLight, hierScroll);
    ui.SetOnClick(nDirLight, "OnSelectLight");

    var nPlayer = ui.CreateTreeNode("n_player", "  Player");
    ui.SetParent(nPlayer, hierScroll);
    ui.SetOnClick(nPlayer, "OnSelectPlayer");

    var nWeapon = ui.CreateTreeNode("n_weapon", "    Weapon");
    ui.SetParent(nWeapon, hierScroll);
    ui.SetOnClick(nWeapon, "OnSelectWeapon");

    var nShield = ui.CreateTreeNode("n_shield", "    Shield");
    ui.SetParent(nShield, hierScroll);
    ui.SetOnClick(nShield, "OnSelectShield");

    var nEnemy1 = ui.CreateTreeNode("n_enemy1", "  Enemy_01");
    ui.SetParent(nEnemy1, hierScroll);
    ui.SetOnClick(nEnemy1, "OnSelectEnemy");

    var nEnemy2 = ui.CreateTreeNode("n_enemy2", "  Enemy_02");
    ui.SetParent(nEnemy2, hierScroll);
    ui.SetOnClick(nEnemy2, "OnSelectEnemy");

    var nGround = ui.CreateTreeNode("n_ground", "  GroundPlane");
    ui.SetParent(nGround, hierScroll);
    ui.SetOnClick(nGround, "OnSelectGround");

    var nSkybox = ui.CreateTreeNode("n_skybox", "  Skybox");
    ui.SetParent(nSkybox, hierScroll);
    ui.SetOnClick(nSkybox, "OnSelectSkybox");

    // --- CENTER PANE: Viewport + Console ----------------
    var centerPane = ui.CreateSplitPane("center_pane", false);
    ui.SetParent(centerPane, dockArea);
    ui.SetLayoutColumn(centerPane, 2);
    ui.SetFillWidth(centerPane);
    ui.SetFillHeight(centerPane);

    // Viewport toolbar
    var vpToolbar = ui.CreatePanel("vp_toolbar");
    ui.SetParent(vpToolbar, centerPane);
    ui.SetLayoutRow(vpToolbar, 4);
    ui.SetSize(vpToolbar, 0, 28);
    ui.SetFillWidth(vpToolbar);
    ui.SetPadding(vpToolbar, 3, 6, 3, 6);

    var vpLabel = ui.CreateLabel("vp_label", ui.L("panel.viewport"));
    ui.SetParent(vpLabel, vpToolbar);

    ui.CreateSeparator("vp_sep1");
    ui.SetParent(ui.FindWidget("vp_sep1"), vpToolbar);

    var btnSelect = ui.CreateButton("btn_select", ui.L("tool.select"));
    ui.SetParent(btnSelect, vpToolbar);
    ui.SetSize(btnSelect, 60, 22);
    ui.SetOnClick(btnSelect, "OnToolSelect");

    var btnMove = ui.CreateButton("btn_move", ui.L("tool.move"));
    ui.SetParent(btnMove, vpToolbar);
    ui.SetSize(btnMove, 60, 22);
    ui.SetOnClick(btnMove, "OnToolMove");

    var btnRotate = ui.CreateButton("btn_rotate", ui.L("tool.rotate"));
    ui.SetParent(btnRotate, vpToolbar);
    ui.SetSize(btnRotate, 60, 22);
    ui.SetOnClick(btnRotate, "OnToolRotate");

    var btnScale = ui.CreateButton("btn_scale", ui.L("tool.scale"));
    ui.SetParent(btnScale, vpToolbar);
    ui.SetSize(btnScale, 60, 22);
    ui.SetOnClick(btnScale, "OnToolScale");

    ui.CreateSeparator("vp_sep2");
    ui.SetParent(ui.FindWidget("vp_sep2"), vpToolbar);

    var btnGrid = ui.CreateCheckbox("btn_grid", true);
    ui.SetParent(btnGrid, vpToolbar);
    ui.SetSize(btnGrid, 18, 18);

    var gridLabel = ui.CreateLabel("grid_lbl", "Grid");
    ui.SetParent(gridLabel, vpToolbar);

    var btnSnap = ui.CreateCheckbox("btn_snap", false);
    ui.SetParent(btnSnap, vpToolbar);
    ui.SetSize(btnSnap, 18, 18);

    var snapLabel = ui.CreateLabel("snap_lbl", "Snap");
    ui.SetParent(snapLabel, vpToolbar);

    // Viewport area (game render target)
    var vpArea = ui.CreatePanel("vp_area");
    ui.SetParent(vpArea, centerPane);
    ui.SetFillWidth(vpArea);
    ui.SetFillHeight(vpArea);

    var vpPlaceholder = ui.CreateLabel("vp_placeholder", "[Game Viewport - 3D Scene Renders Here]");
    ui.SetParent(vpPlaceholder, vpArea);
    ui.SetPadding(vpPlaceholder, 8, 8, 8, 8);

    // Console panel
    var consPanel = ui.CreatePanel("cons_panel");
    ui.SetParent(consPanel, centerPane);
    ui.SetLayoutColumn(consPanel, 2);
    ui.SetSize(consPanel, 0, 150);
    ui.SetFillWidth(consPanel);
    ui.SetPadding(consPanel, 4, 6, 4, 6);

    // Console header with filter tabs
    var consHeader = ui.CreatePanel("cons_header");
    ui.SetParent(consHeader, consPanel);
    ui.SetLayoutRow(consHeader, 4);
    ui.SetSize(consHeader, 0, 22);
    ui.SetFillWidth(consHeader);

    var consTitle = ui.CreateLabel("cons_title", ui.L("panel.console"));
    ui.SetParent(consTitle, consHeader);

    ui.CreateSeparator("cons_sep1");
    ui.SetParent(ui.FindWidget("cons_sep1"), consHeader);

    var btnClear = ui.CreateButton("btn_clear", "Clear");
    ui.SetParent(btnClear, consHeader);
    ui.SetSize(btnClear, 44, 18);
    ui.SetOnClick(btnClear, "OnConsoleClear");

    var btnInfo = ui.CreateButton("btn_info", "Info");
    ui.SetParent(btnInfo, consHeader);
    ui.SetSize(btnInfo, 36, 18);

    var btnWarn = ui.CreateButton("btn_warn", "Warn");
    ui.SetParent(btnWarn, consHeader);
    ui.SetSize(btnWarn, 40, 18);

    var btnErr = ui.CreateButton("btn_error", "Error");
    ui.SetParent(btnErr, consHeader);
    ui.SetSize(btnErr, 40, 18);

    // Console log area
    consoleLog = ui.CreateScrollView("cons_log");
    ui.SetParent(consoleLog, consPanel);
    ui.SetFillWidth(consoleLog);
    ui.SetFillHeight(consoleLog);

    // Initial log messages
    var l1 = ui.CreateLabel("log_1", "[Info] Koilo Editor initialized");
    ui.SetParent(l1, consoleLog);

    var l2 = ui.CreateLabel("log_2", "[Info] Scene loaded: 10 objects");
    ui.SetParent(l2, consoleLog);

    var l3 = ui.CreateLabel("log_3", "[Info] UI system: 57 methods, 18 widget types");
    ui.SetParent(l3, consoleLog);

    var l4 = ui.CreateLabel("log_4", "[Info] Render backend: GPU active");
    ui.SetParent(l4, consoleLog);

    logCount = 4;

    // Console command input
    var consInput = ui.CreateTextField("cons_input", "Enter command...");
    ui.SetParent(consInput, consPanel);
    ui.SetSize(consInput, 0, 22);
    ui.SetFillWidth(consInput);

    // --- RIGHT PANE: Inspector --------------------------
    inspRightPane = ui.CreateSplitPane("right_pane", false);
    ui.SetParent(inspRightPane, dockArea);
    ui.SetLayoutColumn(inspRightPane, 4);
    ui.SetSize(inspRightPane, 280, 0);
    ui.SetFillHeight(inspRightPane);
    ui.SetPadding(inspRightPane, 6, 6, 6, 6);

    var inspTitleLabel = ui.CreateLabel("insp_title", ui.L("panel.inspector"));
    ui.SetParent(inspTitleLabel, inspRightPane);
    ui.SetAriaLabel(inspTitleLabel, "Inspector panel title");

    ui.CreateSeparator("insp_sep1");
    ui.SetParent(ui.FindWidget("insp_sep1"), inspRightPane);

    inspScroll = ui.CreateScrollView("insp_scroll");
    ui.SetParent(inspScroll, inspRightPane);
    ui.SetFillWidth(inspScroll);
    ui.SetFillHeight(inspScroll);

    // Default state: no selection
    inspNoSel = ui.CreateLabel("insp_nosel", "No object selected");
    ui.SetParent(inspNoSel, inspScroll);

    // ═══════════════════════════════════════════════════════
    //  STATUS BAR
    // ═══════════════════════════════════════════════════════
    var statusBar = ui.CreatePanel("statusBar");
    ui.SetParent(statusBar, root);
    ui.SetLayoutRow(statusBar, 16);
    ui.SetSize(statusBar, 0, 22);
    ui.SetFillWidth(statusBar);
    ui.SetPadding(statusBar, 2, 12, 2, 12);

    fpsLabel = ui.CreateLabel("fps_label", "FPS: --");
    ui.SetParent(fpsLabel, statusBar);

    ui.CreateSeparator("st_sep1");
    ui.SetParent(ui.FindWidget("st_sep1"), statusBar);

    widgetLabel = ui.CreateLabel("widget_label", "Widgets: 0");
    ui.SetParent(widgetLabel, statusBar);

    ui.CreateSeparator("st_sep2");
    ui.SetParent(ui.FindWidget("st_sep2"), statusBar);

    statusTool = ui.CreateLabel("tool_label", "Tool: Select");
    ui.SetParent(statusTool, statusBar);

    ui.CreateSeparator("st_sep3");
    ui.SetParent(ui.FindWidget("st_sep3"), statusBar);

    var statusReady = ui.CreateLabel("status_ready", "Ready");
    ui.SetParent(statusReady, statusBar);

    // Log setup complete
    LogMessage("[Info] Editor UI built successfully");
}

// =================================================================
// INSPECTOR BUILDER
// =================================================================

var inspRightPane = -1;

fn RebuildInspectorScroll() {
    // Destroy old scroll view (and all its children)
    if (inspScroll != -1) {
        ui.DestroyWidget(inspScroll);
    }
    // Create a fresh scroll view
    inspScroll = ui.CreateScrollView("insp_scroll");
    ui.SetParent(inspScroll, inspRightPane);
    ui.SetFillWidth(inspScroll);
    ui.SetFillHeight(inspScroll);
}

fn BuildInspector(name) {
    RebuildInspectorScroll();

    // Object name header
    inspNameLabel = ui.CreateLabel("insp_name", name);
    ui.SetParent(inspNameLabel, inspScroll);

    ui.CreateSeparator("insp_sep_name");
    ui.SetParent(ui.FindWidget("insp_sep_name"), inspScroll);

    // --- Transform section ------------------------------
    var transformLabel = ui.CreateLabel("insp_transform", "Transform");
    ui.SetParent(transformLabel, inspScroll);

    // Position
    var posPanel = ui.CreatePanel("insp_pos_panel");
    ui.SetParent(posPanel, inspScroll);
    ui.SetLayoutRow(posPanel, 2);
    ui.SetSize(posPanel, 0, 22);
    ui.SetFillWidth(posPanel);

    var posLbl = ui.CreateLabel("insp_pos_lbl", "Pos");
    ui.SetParent(posLbl, posPanel);
    ui.SetSize(posLbl, 32, 20);

    inspPosX = ui.CreateTextField("insp_pos_x", "0.0");
    ui.SetParent(inspPosX, posPanel);
    ui.SetSize(inspPosX, 64, 20);

    inspPosY = ui.CreateTextField("insp_pos_y", "0.0");
    ui.SetParent(inspPosY, posPanel);
    ui.SetSize(inspPosY, 64, 20);

    inspPosZ = ui.CreateTextField("insp_pos_z", "0.0");
    ui.SetParent(inspPosZ, posPanel);
    ui.SetSize(inspPosZ, 64, 20);

    // Rotation
    var rotPanel = ui.CreatePanel("insp_rot_panel");
    ui.SetParent(rotPanel, inspScroll);
    ui.SetLayoutRow(rotPanel, 2);
    ui.SetSize(rotPanel, 0, 22);
    ui.SetFillWidth(rotPanel);

    var rotLbl = ui.CreateLabel("insp_rot_lbl", "Rot");
    ui.SetParent(rotLbl, rotPanel);
    ui.SetSize(rotLbl, 32, 20);

    inspRotX = ui.CreateTextField("insp_rot_x", "0.0");
    ui.SetParent(inspRotX, rotPanel);
    ui.SetSize(inspRotX, 64, 20);

    inspRotY = ui.CreateTextField("insp_rot_y", "0.0");
    ui.SetParent(inspRotY, rotPanel);
    ui.SetSize(inspRotY, 64, 20);

    inspRotZ = ui.CreateTextField("insp_rot_z", "0.0");
    ui.SetParent(inspRotZ, rotPanel);
    ui.SetSize(inspRotZ, 64, 20);

    // Scale
    var sclPanel = ui.CreatePanel("insp_scl_panel");
    ui.SetParent(sclPanel, inspScroll);
    ui.SetLayoutRow(sclPanel, 2);
    ui.SetSize(sclPanel, 0, 22);
    ui.SetFillWidth(sclPanel);

    var sclLbl = ui.CreateLabel("insp_scl_lbl", "Scl");
    ui.SetParent(sclLbl, sclPanel);
    ui.SetSize(sclLbl, 32, 20);

    inspScaleX = ui.CreateTextField("insp_scl_x", "1.0");
    ui.SetParent(inspScaleX, sclPanel);
    ui.SetSize(inspScaleX, 64, 20);

    inspScaleY = ui.CreateTextField("insp_scl_y", "1.0");
    ui.SetParent(inspScaleY, sclPanel);
    ui.SetSize(inspScaleY, 64, 20);

    inspScaleZ = ui.CreateTextField("insp_scl_z", "1.0");
    ui.SetParent(inspScaleZ, sclPanel);
    ui.SetSize(inspScaleZ, 64, 20);

    ui.CreateSeparator("insp_sep_xform");
    ui.SetParent(ui.FindWidget("insp_sep_xform"), inspScroll);

    // --- Properties section -----------------------------
    var propsLabel = ui.CreateLabel("insp_props", "Properties");
    ui.SetParent(propsLabel, inspScroll);

    // Visible checkbox
    var visPanel = ui.CreatePanel("insp_vis_panel");
    ui.SetParent(visPanel, inspScroll);
    ui.SetLayoutRow(visPanel, 4);
    ui.SetSize(visPanel, 0, 22);
    ui.SetFillWidth(visPanel);

    inspVisible = ui.CreateCheckbox("insp_visible", true);
    ui.SetParent(inspVisible, visPanel);
    ui.SetSize(inspVisible, 18, 18);

    var visLbl = ui.CreateLabel("insp_vis_lbl", "Visible");
    ui.SetParent(visLbl, visPanel);

    // Static checkbox
    var staticPanel = ui.CreatePanel("insp_static_panel");
    ui.SetParent(staticPanel, inspScroll);
    ui.SetLayoutRow(staticPanel, 4);
    ui.SetSize(staticPanel, 0, 22);
    ui.SetFillWidth(staticPanel);

    inspStatic = ui.CreateCheckbox("insp_static", false);
    ui.SetParent(inspStatic, staticPanel);
    ui.SetSize(inspStatic, 18, 18);

    var staticLbl = ui.CreateLabel("insp_static_lbl", "Static");
    ui.SetParent(staticLbl, staticPanel);

    // Cast Shadow checkbox
    var shadowPanel = ui.CreatePanel("insp_shadow_panel");
    ui.SetParent(shadowPanel, inspScroll);
    ui.SetLayoutRow(shadowPanel, 4);
    ui.SetSize(shadowPanel, 0, 22);
    ui.SetFillWidth(shadowPanel);

    inspCastShadow = ui.CreateCheckbox("insp_shadow", true);
    ui.SetParent(inspCastShadow, shadowPanel);
    ui.SetSize(inspCastShadow, 18, 18);

    var shadowLbl = ui.CreateLabel("insp_shadow_lbl", "Cast Shadow");
    ui.SetParent(shadowLbl, shadowPanel);

    ui.CreateSeparator("insp_sep_props");
    ui.SetParent(ui.FindWidget("insp_sep_props"), inspScroll);

    // --- Tag / Layer ------------------------------------
    var tagPanel = ui.CreatePanel("insp_tag_panel");
    ui.SetParent(tagPanel, inspScroll);
    ui.SetLayoutRow(tagPanel, 4);
    ui.SetSize(tagPanel, 0, 22);
    ui.SetFillWidth(tagPanel);

    var tagLbl = ui.CreateLabel("insp_tag_lbl", "Tag");
    ui.SetParent(tagLbl, tagPanel);
    ui.SetSize(tagLbl, 40, 20);

    var tagField = ui.CreateTextField("insp_tag", "Default");
    ui.SetParent(tagField, tagPanel);
    ui.SetSize(tagField, 120, 20);

    var layerPanel = ui.CreatePanel("insp_layer_panel");
    ui.SetParent(layerPanel, inspScroll);
    ui.SetLayoutRow(layerPanel, 4);
    ui.SetSize(layerPanel, 0, 24);
    ui.SetFillWidth(layerPanel);

    var layerLbl = ui.CreateLabel("insp_layer_lbl", "Layer");
    ui.SetParent(layerLbl, layerPanel);
    ui.SetSize(layerLbl, 40, 20);

    var layerField = ui.CreateTextField("insp_layer", "0");
    ui.SetParent(layerField, layerPanel);
    ui.SetSize(layerField, 120, 20);

    LogMessage("[Info] Inspector: " + name);
}

fn BuildLightInspector(name) {
    BuildInspector(name);

    ui.CreateSeparator("insp_sep_light");
    ui.SetParent(ui.FindWidget("insp_sep_light"), inspScroll);

    var lightLabel = ui.CreateLabel("insp_light", "Light Component");
    ui.SetParent(lightLabel, inspScroll);

    // Color RGB sliders
    var colPanel = ui.CreatePanel("insp_col_panel");
    ui.SetParent(colPanel, inspScroll);
    ui.SetLayoutRow(colPanel, 4);
    ui.SetSize(colPanel, 0, 24);
    ui.SetFillWidth(colPanel);

    var colLbl = ui.CreateLabel("insp_col_lbl", "Color");
    ui.SetParent(colLbl, colPanel);
    ui.SetSize(colLbl, 44, 20);

    inspColorR = ui.CreateSlider("insp_col_r", 0, 1, 1);
    ui.SetParent(inspColorR, colPanel);
    ui.SetSize(inspColorR, 50, 20);

    inspColorG = ui.CreateSlider("insp_col_g", 0, 1, 0.95);
    ui.SetParent(inspColorG, colPanel);
    ui.SetSize(inspColorG, 50, 20);

    inspColorB = ui.CreateSlider("insp_col_b", 0, 1, 0.85);
    ui.SetParent(inspColorB, colPanel);
    ui.SetSize(inspColorB, 50, 20);

    // Intensity
    var intPanel = ui.CreatePanel("insp_int_panel");
    ui.SetParent(intPanel, inspScroll);
    ui.SetLayoutRow(intPanel, 4);
    ui.SetSize(intPanel, 0, 24);
    ui.SetFillWidth(intPanel);

    var intLbl = ui.CreateLabel("insp_int_lbl", "Intensity");
    ui.SetParent(intLbl, intPanel);
    ui.SetSize(intLbl, 60, 20);

    inspIntensity = ui.CreateSlider("insp_intensity", 0, 10, 1);
    ui.SetParent(inspIntensity, intPanel);
    ui.SetSize(inspIntensity, 100, 20);
}

fn LogMessage(msg) {
    logCount = logCount + 1;
    var entry = ui.CreateLabel("log_" + logCount, msg);
    ui.SetParent(entry, consoleLog);
}

// =================================================================
// EVENT HANDLERS
// =================================================================

fn OnMenuFile() {
    LogMessage("[Info] File menu opened");
}

fn OnMenuEdit() {
    LogMessage("[Info] Edit menu opened");
}

fn OnMenuView() {
    LogMessage("[Info] View menu opened");
}

fn OnMenuTools() {
    LogMessage("[Info] Tools menu opened");
}

fn OnMenuHelp() {
    LogMessage("[Info] Help menu opened");
}

fn OnPlay() {
    LogMessage("[Info] Play mode started");
}

fn OnPause() {
    LogMessage("[Info] Play mode paused");
}

fn OnStop() {
    LogMessage("[Info] Play mode stopped");
}

fn OnSelectScene() {
    selectedNode = "Scene";
    BuildInspector("Scene");
}

fn OnSelectCamera() {
    selectedNode = "MainCamera";
    BuildInspector("MainCamera");
}

fn OnSelectLight() {
    selectedNode = "DirectionalLight";
    BuildLightInspector("DirectionalLight");
}

fn OnSelectPlayer() {
    selectedNode = "Player";
    BuildInspector("Player");
}

fn OnSelectWeapon() {
    selectedNode = "Weapon";
    BuildInspector("Weapon");
}

fn OnSelectShield() {
    selectedNode = "Shield";
    BuildInspector("Shield");
}

fn OnSelectEnemy() {
    selectedNode = "Enemy_01";
    BuildInspector("Enemy_01");
}

fn OnSelectGround() {
    selectedNode = "GroundPlane";
    BuildInspector("GroundPlane");
}

fn OnSelectSkybox() {
    selectedNode = "Skybox";
    BuildInspector("Skybox");
}

fn OnToolSelect() {
    activeTool = "Select";
    ui.SetText(statusTool, "Tool: Select");
    LogMessage("[Info] Tool: Select");
}

fn OnToolMove() {
    activeTool = "Move";
    ui.SetText(statusTool, "Tool: Move");
    LogMessage("[Info] Tool: Move");
}

fn OnToolRotate() {
    activeTool = "Rotate";
    ui.SetText(statusTool, "Tool: Rotate");
    LogMessage("[Info] Tool: Rotate");
}

fn OnToolScale() {
    activeTool = "Scale";
    ui.SetText(statusTool, "Tool: Scale");
    LogMessage("[Info] Tool: Scale");
}

fn OnConsoleClear() {
    LogMessage("--- Console cleared ---");
}

// -- Per-frame update ---------------------------------
fn Update(dt) {
    frameCount = frameCount + 1;
    fpsAccum = fpsAccum + dt;

    // Update FPS every 30 frames
    if (frameCount % 30 == 0) {
        if (fpsAccum > 0) {
            fpsDisplay = 30.0 / fpsAccum;
        }
        fpsAccum = 0.0;
        ui.SetText(fpsLabel, "FPS: " + fpsDisplay);
    }

    // Update widget count
    var count = ui.GetWidgetCount();
    ui.SetText(widgetLabel, "Widgets: " + count);
}
