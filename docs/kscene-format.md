# `.kscene` - Koilo Scene File Format

**Status:** v1. Authoring + import-based loading. Runtime
`LoadScene()` and `SaveScene()` are scoped to v2.

## What is a `.kscene` file?

A `.kscene` file is a **declarative subset of KoiloScript** that
describes the static, authored portion of a scene - entities, world
objects (rigid bodies, colliders, lights, etc.), and their initial
field values. It is loaded into a running script by the standard
`import` mechanism.

## Authoring rules

A `.kscene` file MUST contain only **top-level declarative statements**:

* `var name = ClassName(args...);` - construct a reflected C++ object
  via its registered constructor. The instance becomes available to
  the importing script under `name`.
* `name.Setter(value);` - call a reflected setter method on a
  previously-declared object.
* `display.SetX(value);` - configure the host display (only if the
  importing script has not already done so).

A `.kscene` file MUST NOT contain:

* Function definitions (`fn ...`) - runtime hooks belong in the
  outer `.ks` script.
* Per-frame logic, control flow that depends on input/time, or
  anything that is not idempotent at scene-build time.
* `Setup` / `Update` / `Render` callbacks.
* `import` statements (kept flat for v1 to keep merge order simple).

These rules are conventions, not enforced by the compiler. A `.kscene`
file that breaks them will still load - but the editor's reload / save
flows assume the rules hold.

## Loading

From a `.ks` script:

```ks
import "scenes/physics_demo_world.kscene"

fn Setup() {
    // ... runtime wiring that uses objects defined in the .kscene
    physics.AddBody(groundBody);
}
```

The `import` keyword resolves the path relative to the importing
script's directory, parses the imported file, and merges its top-level
init statements into the importing script's AST. After
`KoiloScriptEngine::BuildScene()` runs, every `var` declared in the
`.kscene` is available in the global scope of the importing script.

There is **no extension restriction** - `.kscene` is a convention. A
file ending in `.ks` could also be imported. The `.kscene` extension
exists so editor tools (file browser, hot-reload watcher, scene-list
panels) can distinguish authored scene data from runtime logic.

## Example

`assets/scenes/empty_room.kscene`:

```ks
// empty_room.kscene - 80x80 ground + 4 walls. Hand-edit safe.

var groundCol  = BoxCollider(Vector3D(0.0, 0.0 - 0.5, 0.0),
                             Vector3D(80.0, 1.0, 80.0));
var groundBody = RigidBody();
groundBody.MakeStatic();
groundBody.SetCollider(groundCol);
groundBody.SetRestitution(0.4);
groundBody.SetFriction(0.7);

var wallNCol  = BoxCollider(Vector3D(0.0,  5.0,  40.0),
                            Vector3D(80.0, 10.0, 1.0));
var wallNBody = RigidBody();
wallNBody.MakeStatic();
wallNBody.SetCollider(wallNCol);

var wallSCol  = BoxCollider(Vector3D(0.0,  5.0, 0.0 - 40.0),
                            Vector3D(80.0, 10.0, 1.0));
var wallSBody = RigidBody();
wallSBody.MakeStatic();
wallSBody.SetCollider(wallSCol);

// (East/West walls omitted for brevity.)
```

`my_demo.ks`:

```ks
display.SetType("desktop");
display.SetWidth(1280);
display.SetHeight(720);

import "scenes/empty_room.kscene"

fn Setup() {
    physics.AddBody(groundBody);
    physics.AddBody(wallNBody);
    physics.AddBody(wallSBody);
    physics.SetGravity(Vector3D(0.0, 0.0 - 9.81, 0.0));
}

fn Update(dt) {
    // ... runtime logic ...
}
```