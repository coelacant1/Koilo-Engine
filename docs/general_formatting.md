# Formatting & Style Guide

This document defines the canonical formatting and naming conventions for KoiloEngine.

---

## 1. File Layout

### 1.1 Header files (`.hpp` / `.h`)

```
@file block
#pragma once
<blank line>
system includes      (angle brackets, alphabetical)
project includes     (angle brackets for cross-module, quotes for same-directory)
<blank line>
namespace koilo {
<blank line>
class/struct definitions
<blank line>
} // namespace koilo
```

### 1.2 Source files (`.cpp`)

```
@file block
#include "own header"
<blank line>
additional includes (if any)
<blank line>
using namespace / namespace block
<blank line>
implementations
```

### 1.3 File header block

Every file starts with a Doxygen block:

```cpp
// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file quaternion.hpp
 * @brief A mathematical construct representing a rotation in 3D space.
 *
 * Optional additional paragraphs for context, usage notes, or algorithm references.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */
```

Required tags: `@file`, `@brief`. Optional: `@date`, `@version`, `@author`, additional prose.

---

## 2. Naming Conventions

### 2.1 Summary table

| Element              | Style          | Example                          |
| -------------------- | -------------- | -------------------------------- |
| Files                | lowercase      | `quaternion.hpp`, `vector3d.cpp` |
| Classes / structs    | PascalCase     | `Quaternion`, `Vector3D`         |
| Enums (type)         | PascalCase     | `ColliderType`, `ChannelInterp`  |
| Enum values          | PascalCase     | `ColliderType::Sphere`           |
| Public methods       | PascalCase     | `RotateVector()`, `GetNormal()`  |
| Private methods      | PascalCase     | `ClearScene()`, `ReadHeader()`   |
| Public member vars   | PascalCase     | `float W;`, `float X;`          |
| Private member vars  | camelCase      | `float edgeLength;`              |
| Local variables      | camelCase      | `float halfAngle;`               |
| Parameters           | camelCase      | `const float& timeDelta`         |
| Constants            | PascalCase     | `Mathematics::EPSILON`           |
| Namespaces           | lowercase      | `namespace koilo`                |
| Template parameters  | PascalCase     | `template<typename ParamBlock>`  |
| Macros               | SCREAMING_CASE | `KL_BEGIN_FIELDS`, `KL_METHOD`   |

### 2.2 No underscores in identifiers

Do not use underscores in variable, method, or class names. Use camelCase or PascalCase:

```cpp
// Correct
float halfAngleLength;
void GetBiVector() const;

// Incorrect
float half_angle_length;
void get_bi_vector() const;
float vertices_;
```

Exceptions:
- Macros (`KL_BEGIN_FIELDS`, `KL_FIELD`)
- File-scope constants following existing C conventions (`__FLT_MAX__`)
- Third-party/vendor code

### 2.3 Abbreviations

Treat abbreviations as single words in PascalCase:

```cpp
// Correct
Vector2D, Vector3D, UString, GetID, IsNaN

// Incorrect
GetId, IsNan (unless it is a single common word)
```

Well-known abbreviations (2D, 3D, ID, FFT, PID, ECS, UI, AI) keep their canonical casing.

---

## 3. Formatting

### 3.1 Braces

Opening brace on the same line for functions, classes, and control flow:

```cpp
class Quaternion {
public:
    Quaternion Add(const Quaternion& quaternion) const {
        return Quaternion{ W + quaternion.W, X + quaternion.X, ... };
    }
};

if (dot < 0.0f) {
    q1U = q1U.AdditiveInverse();
}
```

### 3.2 Indentation

- 4 spaces, no tabs (except legacy files under conversion).

### 3.3 Line length

- Soft limit: 120 characters. Hard limit: 140 characters.

### 3.4 Spacing

```cpp
// Operators: spaces around binary operators
float result = a + b * c;

// No space before parentheses in function calls
RotateVector(v);

// Space after keywords
if (condition) {
for (int i = 0; i < n; i++) {

// No trailing whitespace
```

### 3.5 Includes

Order (separated by blank lines when groups are large):

1. Own header (source files only, using quotes)
2. Standard library headers (`<cstdint>`, `<vector>`, etc.)
3. Third-party headers (`<SDL2/SDL.h>`)
4. Project headers -- quotes for same directory, angle brackets for cross-module

```cpp
#include "mathematics.hpp"          // same directory

#include <cstdint>
#include <vector>

#include <koilo/registry/reflect_macros.hpp>
#include <koilo/core/math/vector3d.hpp>
```

Never use relative paths with `../`. Use angle bracket project includes for cross-module references.

---

## 4. Documentation

### 4.1 Classes

```cpp
/**
 * @class Quaternion
 * @brief A mathematical construct representing a rotation in 3D space.
 *
 * Quaternions consist of a scalar part (W) and a vector part (X, Y, Z).
 */
class Quaternion {
```

### 4.2 Methods

Every public and protected method gets a Doxygen block:

```cpp
/**
 * @brief Rotates a 3D vector by this quaternion.
 * @param v The 3D vector to rotate.
 * @return A new rotated 3D vector.
 */
Vector3D RotateVector(const Vector3D& v) const;
```

Private methods: a brief `@brief` is sufficient. Trivial getters/setters can use `///< ` inline.

### 4.3 Member variables

Use trailing Doxygen comments:

```cpp
float W; ///< Scalar part of the quaternion.
float X; ///< X component of the quaternion's vector part.
```

### 4.4 Implementation files

- `@file` header block required.
- Method-level Doxygen is optional in `.cpp` (documented in header).
- Short `// Comment` above each method implementation is encouraged for navigation:

```cpp
// Rotate vector
Vector3D Quaternion::RotateVector(const Vector3D& v) const {
    ...
}
```

### 4.5 Comment style

- Use `@` prefix for Doxygen commands (not `\`).
- ASCII only in all source files. No Unicode characters (no em-dashes, arrows, check marks, degree symbols, etc.).
- Comments should be terse and technical. Avoid marketing language.
- Section separators use ASCII dashes: `// --- Section ---`

---

## 5. Code Patterns

### 5.1 Const correctness

- Mark methods `const` when they don't modify state.
- Pass objects by `const&`. Pass primitives by `const&` or by value (both accepted).

### 5.2 Return values

- Return value types by value (not pointer or reference) when possible.
- Use `const` references for heavy objects only when lifetime is guaranteed.

### 5.3 Include guards

- Always `#pragma once`. No traditional `#ifndef` guards (exception: public SDK headers for external compatibility).

### 5.4 Namespaces

- All engine code lives in `namespace koilo` (or nested: `koilo::scripting`, `koilo::platform`).
- Close with `} // namespace koilo`.

---

## 6. Python files

- Module-level docstring required.
- Follow PEP 8 naming (snake_case functions/variables, PascalCase classes).
- ASCII only in source.

---

## 7. Reference files

For the canonical example of these conventions applied, see:

- **Header**: `engine/koilo/core/math/quaternion.hpp`
- **Source**: `engine/koilo/core/math/quaternion.cpp`
