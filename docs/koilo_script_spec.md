# KoiloScript Language Specification

**Version:** 3.0  
**Date:** 2026-03-08

---

## 1. Overview

KoiloScript is a scripting language for KoiloEngine. It replaces compiled C++ animation files with runtime-loaded `.ks` scripts that configure display hardware, construct objects, compose scenes, and run per-frame logic without recompilation.

Imperative, C++-familiar syntax with semicolons, constructor calls, method chains, and `Setup()`/`Update()` lifecycle functions.

### Design Goals
- Edit script, restart, done - no recompilation
- C++-familiar syntax (`fn`/`var`, constructor calls, method chains)
- Same script runs on Teensy 4.1 (HUB75) and Desktop (SDL2)
- Any C++ class with reflection macros is scriptable
- Parser targets 3-5 KB on microcontrollers

### File Extension
`.ks`

---

## 2. Script Structure

Scripts consist of **top-level code** (imports, class declarations, variable declarations, constructor calls, method calls) and **function definitions**.

### Structure Pattern (Arduino-like)

```ks
// -- Top-level init code ----------------------------------------------
// Runs once during initialization. Configure display, declare variables,
// create objects. The `display` object is implicitly available.

display.SetType("desktop");
display.SetWidth(1280);
display.SetHeight(720);
display.SetPixelWidth(192);
display.SetPixelHeight(94);

var faceMat = UniformColorMaterial(Color888(255, 255, 255));
var face = MorphableMesh();

// -- Setup (runs once after scene is created) -------------------------
fn Setup() {
    face.Load("models/face.kmesh");
    face.GetMesh().SetMaterial(faceMat);
    scene.AddMesh(face.GetMesh());
}

// -- User functions ---------------------------------------------------
fn setAngry() {
    faceMat.SetColor(255, 0, 0);
}

// -- Input handler ----------------------------------------------------
fn OnKeyPress(key) {
    if key == "1" { setAngry(); }
}

// -- Per-frame update -------------------------------------------------
fn Update(dt) {
    face.Update();
}
```

### Execution Order
1. `LoadScript()` - File is read, lexed, parsed, and compiled to bytecode
2. `BuildScene()`:
   a. Register `display` as implicit global (reflected DisplayConfig object)
   b. Register all `fn` declarations as callable functions
   c. Execute top-level init statements (var decls, display config, constructors)
   d. Read display config -> create Camera, PixelGroup, Scene
   e. Register `scene` as implicit global
   f. Call `fn Setup()` if defined (loads assets, composes scene)
3. `ExecuteUpdate(dt)` - Called every frame; calls `fn Update(dt)` if defined
4. `HandleInput(type, id)` - Calls `fn OnKeyPress(key)`, `fn OnKeyRelease(key)`, or `fn OnPinInput(pin)` if defined
5. `CallFunction(name, args)` - Host can call any script function by name

### Semicolons
All simple statements **require a semicolon** at the end:
```ks
var x = 10;                          // variable declaration
face.SetMorphWeight("Anger", 1.0);   // method call
faceMat.SetColor(255, 0, 0);         // method call
return x + 1;                        // return statement
```

Block statements (if/else, while, for, fn) do **not** need semicolons after their closing brace:
```ks
if x > 0 {
    y = x * 2;
}                                     // no semicolon needed

fn myFunc() {
    // ...
}                                     // no semicolon needed
```

### Indentation
Indentation is **not significant** - it's purely for readability. Braces `{}` define scope, not whitespace.

---

## 3. Implicit Objects

Two objects are automatically available in scripts without declaration:

### `display` - Display Configuration
Available immediately at the top level. Configure before Setup() runs.

```ks
display.SetType("desktop");       // "desktop", "hub75", "ws2812"
display.SetWidth(1280);           // Window width (desktop) or panel width
display.SetHeight(720);           // Window height (desktop) or panel height
display.SetPixelWidth(192);       // Rendering resolution width
display.SetPixelHeight(94);       // Rendering resolution height
display.SetTargetFPS(60);         // Target frame rate
display.SetBrightness(50);        // Display brightness (0-255, embedded only)
```

**Important:** Display configuration must be set in top-level init code (before `fn Setup()`), as camera and scene dimensions are derived from it.

### `scene` - Scene Graph
Available inside `fn Setup()` and `fn Update(dt)`. Provides access to the 3D scene.

```ks
fn Setup() {
    scene.AddMesh(face.GetMesh());    // Add mesh to scene for rendering
}
```

### Module Globals

When modules are loaded, they register additional globals:

| Global | Module | Description |
|--------|--------|-------------|
| `cam` | Core | Camera object |
| `physics` | Physics | PhysicsWorld (collision, raycasting) |
| `audio` | Audio | Audio playback manager |
| `particles` | Particles | Particle system |
| `ai` | AI | State machines, pathfinding |
| `ui` | UI | Widget system |
| `input` | Core | InputManager (keyboard, mouse, gamepad) |
| `debug` | Core | Debug line/sphere/text drawing |
| `entities` | Core | ECS entity manager |
| `world` | Core | Level/world manager |
| `Time` | Core | TimeManager utilities |

Use `has_module("name")` to check availability before accessing module globals.

---

## 4. Functions

### Declaration
Functions are declared with the `fn` keyword (or `function` as an alias):

```ks
fn myFunction(param1, param2) {
    // body
    return result;   // optional
}
```

### Special Functions

| Function | Signature | When Called |
|----------|-----------|------------|
| `Setup()` | `fn Setup()` | Once after scene creation |
| `Update(dt)` | `fn Update(dt)` | Every frame (`dt` = delta time in seconds) |
| `OnKeyPress(key)` | `fn OnKeyPress(key)` | On keyboard key press (`key` is string) |
| `OnKeyRelease(key)` | `fn OnKeyRelease(key)` | On keyboard key release (`key` is string) |
| `OnPinInput(pin)` | `fn OnPinInput(pin)` | On GPIO input (embedded) |
| `OnCollisionEnter()` | `fn OnCollisionEnter()` | When a physics collision starts |
| `OnCollisionExit()` | `fn OnCollisionExit()` | When a physics collision ends |

### User Functions
```ks
fn lerp(a, b, t) {
    return a + (b - a) * t;
}

fn clamp(v, lo, hi) {
    if v < lo { return lo; }
    if v > hi { return hi; }
    return v;
}

fn resetMorphTargets() {
    morphTargets.anger = 0;
    morphTargets.sadness = 0;
}
```

- Parameters are passed by value
- Functions can call other functions (max call depth: 64)
- Functions can access all variables and reflected objects
- `return` exits the function with an optional value

---

## 5. Variables and Types

### Variable Declarations
```ks
var x = 10;
var name = "hello";
var active = true;
var items = [1, 2, 3];
var config = { speed: 12.0, easing: "cosine" };
var pos = Vector3D(0, -0.5, -7.5);     // constructor expression
var color = Color888(255, 50, 50);      // any reflected class
```

Variables are dynamically typed. Constructor expressions create real C++ objects via reflection.

### Value Types

| Type | Examples |
|------|---------|
| Number | `42`, `3.14`, `-1.0`, `true` (1.0), `false` (0.0) |
| String | `"hello"`, `"face.kmesh"` |
| Array | `[1, "two", true]` (mixed-type) |
| Table | `{ x: 1, y: 2, name: "player" }` |
| Object | Reflected C++ object reference |
| NONE | Null value |

### Tables and Arrays
```ks
// Tables (key-value)
var player = { x: 0, y: 0, health: 100 };
player.health = player.health - 10;
player["x"] = 5;

// Arrays (mixed-type, dynamic)
var items = [1, "two", true];
push(items, 4);
var last = pop(items);
remove(items, 0);
var has = contains(items, "two");
var val = items[1];
```

---

## 6. Constructor Expressions

Any class registered with `KL_CTOR` macros can be constructed from script:

```ks
// Math types
var pos = Vector3D(0, -0.5, -7.5);
var dir = Vector3D(1, 0, 0);
var uv = Vector2D(0.5, 0.5);

// Color
var red = Color888(255, 0, 0);

// Materials
var mat = UniformColorMaterial(Color888(255, 255, 255));

// Meshes
var face = MorphableMesh();

// Use directly as function arguments
face.GetMesh().SetMaterial(mat);
```

Constructor overloads are matched by argument count. Default constructors (0 args) are supported.

---

## 7. Method Calls and Chaining

### Basic Method Calls
```ks
face.SetMorphWeight("Anger", 1.0);
face.Reset();
face.Update();
faceMat.SetColor(255, 0, 0);
```

### Chained Method Calls
Methods that return objects can be chained like C++:
```ks
face.GetMesh().SetMaterial(faceMat);
scene.AddMesh(face.GetMesh());
```

The engine evaluates chains left-to-right: each method call returns an object reference, which becomes the receiver for the next call.

### Field Access
```ks
var x = player.position.X;
player.health = 100;
```

### Null Chain Safety
If any link in a chain is null/NONE, the chain silently returns NONE without crashing:
```ks
var result = nonexistent.DoSomething();  // returns NONE, no error
```

---

## 8. Control Flow

```ks
// If / else if / else
if condition {
    // ...
} else if other_condition {
    // ...
} else {
    // ...
}

// While loop
while x < 100 {
    x = x + 1;
    if x == 50 { continue; }
    if x == 75 { break; }
}

// For..in loop (arrays and tables)
for item in items {
    print(item);
}
```

Parentheses around conditions are optional: `if x > 0 {` and `if (x > 0) {` are both valid.

`break` exits the innermost loop. `continue` skips to the next iteration.

---

## 9. Built-In Variables and Functions

### Built-In Variables

| Variable | Type | Description |
|----------|------|-------------|
| `time` | float | Total elapsed time since start (seconds) |
| `deltaTime` | float | Time since last frame (seconds) |

### Built-In Functions

#### Math
| Function | Description |
|----------|-------------|
| `sin(x)` | Sine (radians) |
| `cos(x)` | Cosine (radians) |
| `tan(x)` | Tangent (radians) |
| `abs(x)` | Absolute value |
| `sqrt(x)` | Square root |
| `min(a, b)` | Minimum |
| `max(a, b)` | Maximum |
| `lerp(a, b, t)` | Linear interpolation: `a + (b - a) * t` |
| `clamp(x, lo, hi)` | Clamp to range |
| `map(x, inLo, inHi, outLo, outHi)` | Remap value from one range to another |
| `random()` | Random float 0.0-1.0 |
| `random(max)` | Random float 0-max |
| `random(min, max)` | Random float min-max |
| `floor(x)` | Floor |
| `ceil(x)` | Ceiling |
| `round(x)` | Round to nearest integer |
| `degrees(x)` | Convert radians to degrees |
| `radians(x)` | Convert degrees to radians |

#### Utility
| Function | Description |
|----------|-------------|
| `print(value)` | Output to console |
| `debug(value)` | Output with type information |
| `format(template, args...)` | String formatting with `{}` placeholders |
| `length(x)` | Length of array or string |
| `type(x)` | Returns type name as string |
| `str(x)` | Convert to string |
| `num(x)` | Convert to number |
| `contains(collection, item)` | Check if collection contains item |
| `push(array, item)` | Append item to array |
| `pop(array)` | Remove and return last item |
| `remove(array, index)` | Remove item at index |

#### Signals
| Function | Description |
|----------|-------------|
| `connect(signal, handler)` | Connect a signal to a handler function (by name) |
| `disconnect(signal, handler)` | Disconnect a signal from a handler |
| `connect_once(signal, handler)` | Connect for a single invocation, then auto-disconnect |

#### Coroutines
| Function | Description |
|----------|-------------|
| `start_coroutine(name)` | Start a coroutine by function name |
| `stop_coroutine(name)` | Stop a running coroutine |
| `stop_all_coroutines()` | Stop all running coroutines |

#### Modules
| Function | Description |
|----------|-------------|
| `has_module(name)` | Check if a module is loaded (returns 1.0 or 0.0) |
| `list_modules()` | List all loaded module names (returns array) |

#### Profiler
| Function | Description |
|----------|-------------|
| `profiler_fps()` | Get current FPS |
| `profiler_time(scope)` | Get average time for a named profiler scope |
| `profiler_enable(bool)` | Enable or disable profiling |

---

## 10. Operators

| Operator | Description |
|----------|-------------|
| `+`, `-`, `*`, `/`, `%` | Arithmetic (including modulo) |
| `==`, `!=` | Equality |
| `<`, `>`, `<=`, `>=` | Comparison |
| `&&`, `\|\|`, `!` | Logical AND, OR, NOT |
| `and`, `or`, `not` | Logical (keyword aliases) |
| `-` (unary) | Negation |
| `? :` | Ternary conditional |
| `+` (strings) | String concatenation |
| `[]` | Index access (arrays, tables) |
| `.` | Member access / method call |

### Operator Precedence (lowest to highest)
1. `||` / `or` (logical OR)
2. `&&` / `and` (logical AND)
3. `==`, `!=`, `<`, `>`, `<=`, `>=`
4. `+`, `-`
5. `*`, `/`, `%`
6. Unary `-`, `!`, `not`
7. Postfix `.member`, `[index]`, `(args)`

---

## 11. Variable Scoping

Variables declared with `var` are scoped to their enclosing block. Variables persist across calls in the same scope.

```ks
var globalCounter = 0;                   // top-level - persists across frames

fn Update(dt) {
    globalCounter = globalCounter + 1;    // access top-level var
    var localTemp = sin(time);            // scoped to this function
    
    if localTemp > 0.5 {
        var innerVal = localTemp * 2;     // scoped to this if-block
    }
    // innerVal is not accessible here
}
```

---

## 12. Expressions

### Expression Types

| Type | Syntax | Example |
|------|--------|---------|
| Number literal | `42`, `3.14`, `-1.0` | `var x = 0.8;` |
| String literal | `"hello"` | `var s = "face.kmesh";` |
| Boolean | `true`, `false` | `var active = true;` |
| Null | `NONE` | `var x = NONE;` |
| Array literal | `[a, b, c]` | `var pos = [0, 1, 2];` |
| Table literal | `{ key: value }` | `var t = { x: 1, y: 2 };` |
| Identifier | `varname` | `time`, `deltaTime` |
| Member access | `a.b.c` | `player.position.X` |
| Index access | `a[b]` | `table["key"]`, `arr[0]` |
| Method call | `obj.Method(args)` | `face.Update();` |
| Constructor | `ClassName(args)` | `Vector3D(0, 0, -7.5)` |
| New instance | `new ClassName()` | `new Enemy()` |
| Binary op | `a op b` | `time * 2.0 + 1.0` |
| Unary op | `-a`, `!a` | `-gravity`, `!active` |
| Ternary | `a ? b : c` | `x > 0 ? x : -x` |

---

## 13. Value Type System

Internal value representation:

```
Value {
    type: NUMBER | STRING | ARRAY | TABLE | OBJECT | NONE
    numberValue: double
    stringValue: string
    arrayValue: Value[]            (mixed-type arrays)
    tableValue: map<string,Value>  (key-value tables)
    objectName: string             (reference to reflected C++ object)
}
```

### Type Coercion
- Bool values: `true` = 1.0, `false` = 0.0
- `NONE` is a distinct type (not zero, not empty string)
- Arrays support mixed types: `[1, "hello", true, [2, 3]]`
- Tables support nested values: `{ pos: { x: 1, y: 2 }, name: "player" }`
- String + number -> string concatenation
- No implicit numeric coercion between types

---

## 14. Reflection Integration

KoiloScript creates real C++ objects via the reflection system.

### Supported Marshalled Types
- `float`, `double`, `int`, `uint8_t`, `bool`, `const char*`
- Pointer types (`Mesh*`, `Material*`) - auto-detected via GCC mangling
- Object values (reflected class instances)

### Method Calling from Script
```ks
face.SetMorphWeight("Anger", 1.0);
face.Reset();
var mag = vec.Magnitude();
```
- Methods resolved via `KL_BEGIN_METHODS` reflection metadata
- Parameters type-checked and marshalled at runtime
- Return values usable in expressions and chains

### Pointer Return Handling
Methods that return pointers (e.g., `GetMesh()` returns `Mesh*`) are automatically detected and the returned object is registered for further method calls and field access.

---

## 15. Script Classes

KoiloScript supports user-defined classes with fields, methods, and `self` references.

### Declaration

```ks
class Enemy {
    var health = 100;
    var speed = 5.0;
    var name = "goblin";

    fn TakeDamage(amount) {
        self.health = self.health - amount;
        if self.health <= 0 {
            print(self.name + " defeated!");
        }
    }

    fn Move(dt) {
        self.speed = self.speed * dt;
    }
}
```

### Instantiation

```ks
var e = new Enemy();
e.TakeDamage(30);
print(e.health);  // 70
```

- Fields declared with `var` get default values
- Methods receive an implicit `self` parameter (first local)
- `new ClassName()` creates a new instance with field defaults applied

---

## 16. Signals

Signals provide a publish/subscribe mechanism for decoupled communication.

### Declaration and Emission

```ks
signal player_hit(damage);

fn OnEnemyAttack() {
    emit player_hit(25);
}
```

### Connecting Handlers

```ks
fn handleHit(damage) {
    print("Took " + str(damage) + " damage");
}

fn Setup() {
    connect("player_hit", "handleHit");
}
```

- `signal name(params)` declares a signal (params are documentation-only)
- `emit name(args)` fires the signal, calling all connected handlers
- `connect_once` auto-disconnects after the first invocation

---

## 17. Coroutines

Coroutines allow functions to suspend and resume across frames.

```ks
fn patrol() {
    while true {
        moveTo(pointA);
        yield;
        moveTo(pointB);
        yield;
    }
}

fn Setup() {
    start_coroutine("patrol");
}
```

- `yield` suspends the coroutine until the next frame
- `stop_coroutine(name)` stops a specific coroutine
- `stop_all_coroutines()` stops all running coroutines

---

## 18. Imports

Scripts can import other script files:

```ks
import "utils/math_helpers.ks"
import "enemies/goblin.ks"
```

Imports are processed at the top level before any other code runs.

---

## 19. Complete Example

```ks
// protogen_viewer_v5.ks - Code-First KoiloScript Demo

// -- Display Configuration --------------------------------------------
display.SetType("desktop");
display.SetWidth(1280);
display.SetHeight(720);
display.SetPixelWidth(192);
display.SetPixelHeight(94);
display.SetTargetFPS(60);

// -- Global State -----------------------------------------------------
var faceMat = UniformColorMaterial(Color888(255, 255, 255));
var face = MorphableMesh();

var morphTargets = {
    anger: 0, sadness: 0, surprise: 0,
    doubt: 0, look_up: 0, blink: 0
};
var morphWeights = {
    anger: 0, sadness: 0, surprise: 0,
    doubt: 0, look_up: 0, blink: 0
};

var easeSpeed = 0.08;
var currentColor = { r: 255, g: 255, b: 255 };
var targetColor = { r: 255, g: 255, b: 255 };
var blinkTimer = 0;
var blinkInterval = 3.0;
var isBlinking = false;

// -- Setup (runs once) ------------------------------------------------
fn Setup() {
    face.Load("../../assets/kmesh/nukude_face3.kmesh");
    face.GetMesh().SetMaterial(faceMat);
    scene.AddMesh(face.GetMesh());
}

// -- Expression Functions ---------------------------------------------
fn resetMorphTargets() {
    morphTargets.anger = 0;
    morphTargets.sadness = 0;
    morphTargets.surprise = 0;
    morphTargets.doubt = 0;
}

fn setDefault() {
    resetMorphTargets();
    targetColor.r = 255; targetColor.g = 255; targetColor.b = 255;
}

fn setAngry() {
    resetMorphTargets();
    morphTargets.anger = 1.0;
    targetColor.r = 255; targetColor.g = 30; targetColor.b = 30;
}

// -- Utilities --------------------------------------------------------
fn lerp(a, b, t) {
    return a + (b - a) * t;
}

fn clamp(v, lo, hi) {
    if v < lo { return lo; }
    if v > hi { return hi; }
    return v;
}

// -- Input Handler ----------------------------------------------------
fn OnKeyPress(key) {
    if key == "1" { setDefault(); }
    if key == "2" { setAngry(); }
}

// -- Per-Frame Update -------------------------------------------------
fn Update(dt) {
    // Ease morph weights toward targets
    morphWeights.anger = lerp(morphWeights.anger, morphTargets.anger, easeSpeed);
    morphWeights.sadness = lerp(morphWeights.sadness, morphTargets.sadness, easeSpeed);

    // Ease color
    currentColor.r = lerp(currentColor.r, targetColor.r, easeSpeed);
    currentColor.g = lerp(currentColor.g, targetColor.g, easeSpeed);
    currentColor.b = lerp(currentColor.b, targetColor.b, easeSpeed);

    // Apply morph weights
    face.Reset();
    face.SetMorphWeight("Anger", morphWeights.anger);
    face.SetMorphWeight("Sadness", morphWeights.sadness);
    face.Update();

    // Apply color
    faceMat.SetColor(
        clamp(currentColor.r, 0, 255),
        clamp(currentColor.g, 0, 255),
        clamp(currentColor.b, 0, 255)
    );

    // Auto-blink
    blinkTimer = blinkTimer + dt;
    if blinkTimer > blinkInterval {
        blinkTimer = 0;
        morphTargets.blink = 1.0;
        isBlinking = true;
    }
    if isBlinking and morphWeights.blink > 0.9 {
        morphTargets.blink = 0;
        isBlinking = false;
    }
}
```

---

## 20. Error Handling

| Error Type | When | Access |
|------------|------|--------|
| Parse error | `LoadScript()` returns false | `GetError()` |
| Unknown class | Constructor expression fails | `GetError()` |
| Unknown method | Method call on reflected object | `HasError()` + `GetError()` |
| Type mismatch | Field assignment with wrong type | `GetError()` |
| Marshal failure | Argument type incompatible | `GetError()` |

---

## 21. C++ Host API Reference

### Lifecycle
```cpp
KoiloScriptEngine(IScriptFileReader* reader);
bool LoadScript(const char* filepath);
bool BuildScene();
void ExecuteUpdate(float deltaTime);
bool Reload();
```

### Scene Access
```cpp
Scene* GetScene() const;
Camera* GetCamera() const;
PixelGroup* GetPixelGroup() const;
```

### Object Registration
```cpp
void RegisterObject(const char* id, void* instance, const ClassDesc* classDesc);
template<typename T> T* GetReflectedObject(const char* name);
```

### Configuration
```cpp
const std::map<std::string, std::string>& GetDisplayConfig() const;
```

### Host<->Script Communication
```cpp
void SetGlobal(const std::string& name, const Value& value);
Value GetGlobal(const std::string& name) const;
Value CallFunction(const std::string& name, const std::vector<Value>& args = {});
```

### Input Handling
```cpp
bool HandleInput(const char* inputType, const char* inputId);
// Dispatches to fn OnKeyPress(key) or fn OnPinInput(pin)
```

### Error Handling
```cpp
bool HasError() const;
const char* GetError() const;
```

---

## 22. Grammar Summary (EBNF)

```ebnf
script      = { top_level_item } ;
top_level_item = import_decl | class_decl | signal_decl | function_decl | statement ;

import_decl   = "import" STRING [ ";" ] ;
signal_decl   = "signal" IDENTIFIER [ "(" [ param_list ] ")" ] [ ";" ] ;
class_decl    = "class" IDENTIFIER "{" { class_member } "}" ;
class_member  = ( "var" IDENTIFIER [ "=" expression ] ";" ) | function_decl ;
function_decl = ( "fn" | "function" ) IDENTIFIER "(" [ param_list ] ")" "{" { statement } "}" ;

statement   = ( var_decl | assignment | if_stmt | while_stmt 
            | for_stmt | return_stmt | break_stmt | continue_stmt
            | emit_stmt | yield_stmt | expr_stmt ) ;
var_decl    = "var" IDENTIFIER "=" expression ";" ;
assignment  = postfix_expr "=" expression ";"
            | postfix_expr "[" expression "]" "=" expression ";" ;
if_stmt     = "if" expression "{" { statement } "}" 
              { "else" "if" expression "{" { statement } "}" }
              [ "else" "{" { statement } "}" ] ;
while_stmt  = "while" expression "{" { statement } "}" ;
for_stmt    = "for" IDENTIFIER "in" expression "{" { statement } "}" ;
return_stmt = "return" [ expression ] ";" ;
break_stmt  = "break" ";" ;
continue_stmt = "continue" ";" ;
emit_stmt   = "emit" IDENTIFIER "(" [ arg_list ] ")" ";" ;
yield_stmt  = "yield" ";" ;
expr_stmt   = expression ";" ;

expression  = ternary ;
ternary     = logic_or [ "?" expression ":" expression ] ;
logic_or    = logic_and ( ( "||" | "or" ) logic_and )* ;
logic_and   = comparison ( ( "&&" | "and" ) comparison )* ;
comparison  = addition ( ("==" | "!=" | "<" | ">" | "<=" | ">=") addition )* ;
addition    = multiply ( ("+" | "-") multiply )* ;
multiply    = unary ( ("*" | "/" | "%") unary )* ;
unary       = "-" unary | ( "!" | "not" ) unary | postfix ;
postfix     = primary { "." IDENTIFIER | "(" [ arg_list ] ")" | "[" expression "]" } ;
primary     = NUMBER | STRING | "true" | "false" | "NONE"
            | "new" IDENTIFIER "(" [ arg_list ] ")"
            | IDENTIFIER | array_lit | table_lit
            | "(" expression ")" ;
array_lit   = "[" [ expression ( "," expression )* ] "]" ;
table_lit   = "{" [ IDENTIFIER ":" expression ( "," IDENTIFIER ":" expression )* ] "}" ;
param_list  = IDENTIFIER ( "," IDENTIFIER )* ;
arg_list    = expression ( "," expression )* ;
```

**Note on semicolons:** Simple statements (var declarations, assignments, return, break, continue, emit, yield, expression statements) require a terminating `;`. Block statements (if, while, for, fn, class) do NOT require a semicolon after the closing `}`.

**Note on postfix chaining:** The `postfix` rule enables C++-like expression chains. Each postfix operation (`.member`, `(args)`, `[index]`) is applied left-to-right. Example: `face.GetMesh().SetMaterial(mat);` parses as:
1. `face` -> identifier (reflected object lookup)
2. `.GetMesh` -> member access
3. `()` -> method call (returns Mesh* object)
4. `.SetMaterial` -> member access on returned Mesh
5. `(mat)` -> method call with material argument

**Note on constructor expressions:** `ClassName(args)` is syntactically a function call. At evaluation time, if the identifier matches a registered class in the reflection registry, it invokes the matching constructor.
