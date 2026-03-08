# Reflection System

The reflection system lets C++ classes expose fields, methods, and constructors to KoiloScript without external codegen tools. Any class adds a `KL_BEGIN_DESCRIBE`/`KL_END_DESCRIBE` block to register itself in the global type registry.

---

## Describing a Class

```cpp
class MyObject {
    float speed_ = 1.0f;
    Vector3D position_;

public:
    void SetSpeed(float s) { speed_ = s; }
    Vector3D GetPosition() const { return position_; }

    KL_BEGIN_FIELDS(MyObject)
        KL_FIELD(MyObject, speed_, "Speed", 0.0f, 100.0f)
    KL_END_FIELDS

    KL_BEGIN_METHODS(MyObject)
        KL_METHOD_AUTO(MyObject, SetSpeed, "Set speed")
        KL_METHOD_AUTO(MyObject, GetPosition, "Get position")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(MyObject)
        KL_CTOR0(MyObject)
        KL_CTOR(MyObject, float)
    KL_END_DESCRIBE(MyObject)
};
```

`KL_BEGIN_DESCRIBE` creates a static `Describe()` method that returns a `ClassDesc` containing all fields, methods, and constructors. An `AutoRegistrar` inserts the descriptor into the global registry at static init time.

---

## Type System

The bytecode VM uses NaN-boxed values (`NanBoxedValue`) - all values are 8 bytes. Numbers are raw IEEE 754 doubles. Non-number types (string, array, table, object, bool, nil, function, script instance) encode a 3-bit type tag in the quiet-NaN bit pattern with a 48-bit payload.

The legacy `Value` struct (`NUMBER`, `STRING`, `ARRAY`, `OBJECT`, `NONE`) still exists in `script_context.hpp` for the reflection bridge interface between the VM and C++ methods.

When a method returns a complex type (e.g. `Vector3D`):
1. The return type is looked up in the type registry
2. Memory is allocated for the result
3. The return value is copied in
4. A unique temp name is generated (`_temp_return_0`, etc.)
5. The object is stored in `reflectedObjects` and a `Value::Object` reference is returned

---

## Method Dispatch

Script call dispatch: the VM looks up the object name in `reflectedObjects`, finds the `ClassDesc`, and locates a matching `MethodDesc`. Arguments are marshalled from NaN-boxed values to C++ types via `ArgMarshaller`, and the C++ function pointer is invoked. Return values are marshalled back onto the VM stack.

### Overload Resolution

`FindMethodTyped()` performs type-aware overload resolution. It collects candidates with matching argument count, scores each by type compatibility, and returns the best match. Up to 16 candidates are evaluated from a stack-allocated buffer.

### Nested Field Access

`obj.field.Method()` is handled by splitting the path at the last dot. The left side is walked through field descriptors using `ReflectionBridge::GetFieldPointer()` and `ClassForType()` to resolve the final object and class, then the method is dispatched on that object.

### Operator Overloading

Binary operators (`+`, `-`, `*`, etc.) on reflected objects are dispatched through the reflection bridge. The VM checks if the left operand's class has a matching operator method and invokes it with the right operand as an argument.

---

## Object Lifecycle

Temporary C++ objects created during a frame (method return values, constructor results) are tracked in `frameTempObjects`. At the end of each `ExecuteUpdate()`, `CleanupFrameTemps()` destroys them via `ClassDesc::destroy` or `operator delete`.

Objects assigned to script variables or fields are promoted to persistent via `PromoteToPersistent()` and survive frame cleanup. When a persistent slot is overwritten, the old object is destroyed immediately.

---

## Limitations

- Chained expressions (`a.b().c()` in one statement) are not supported - split across separate statements
- `const` is not tracked by reflection
