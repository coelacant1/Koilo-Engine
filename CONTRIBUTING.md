# Contributing Guide

Thank you for considering a contribution to **uCore3D**.
This document explains the rules every pull-request must follow.

---

## 1. Project Scope

uCore3D is a **header / source** c++17 library that targets

* Arduino-class mcus (avr, cortex-m, esp32)
* Bare-metal embedded c++
* Desktop (clang / gcc / msvc)

The codebase is split into clear layers:

```
include/uc3d/
  assets/       immutable asset data
  geometry/     analytic shapes 2d / 3d
  ...
  math/         vectors, matrices, interpolation
  render/       core, raster, ray, post
  scene/        entities, animation, deform
src/            mirrors include/ tree
tests/          unity test cases (platformio native + teensy)
```

Patches should fit that layering; avoid creating cross-dependencies.

---

## 2. Getting Started

1. **Fork & Clone**

```bash
git clone https://github.com/<your-account>/umath3d.git
```

2. **Create a Branch**

```bash
git checkout -b feat/<short-topic>
```

3. **Make Changes**

* Follow the coding rules below
* Add / update unity tests under `tests/`
* Build for **native** and **teensy**:

```bash
pio test                 # native
pio test -e teensy40     # teensy40
```

4. **Commit & Push**

```bash
git commit -m "Feat: Short summary (fixes #123)"
git push --set-upstream origin feat/<short-topic>
```

5. **Open a Pull Request** against the `main` branch.

---

## 3. Coding Rules

### 3.1 Filenames & Directories

* Lower-case, no underscores: `triangle2d.hpp`, `triangle2d.cpp`.

### 3.2 Namespaces

* **No namespaces for now.** every type lives in the root scope - this may change later.

### 3.3 Functions & Variables

| item           | style example       |
| -------------- | ------------------- |
| Function names | `ComputeNormals()`  |
| Member vars    | `float edgeLength;` |
| Local vars     | `float baryCenter;` |
| Public vars    | `float BaryCenter;` |
| Braces         | `void Foo() {`      |

### 3.4 Headers & Comments

* Every file starts with a doxygen block: `@file`, `@brief`, `@date`, `@author`.
* Every class, function, data member is documented with at least a `@brief`.

### 3.5 Includes

* Only `<stdint.h>`, `<stddef.h>`, `<math.h>`, and other c++17 headers.
* **Never** `Arduino.h` in public headers. Use it only inside `#if defined(ARDUINO)` guards in `.cpp`.

### 3.6 Memory & Containers

* **No** `std::vector`, `std::string`, `new` inside hot loops.
* Use fixed-size arrays or caller-supplied buffers.

### 3.7 Formatting Helper

Run `clang-format -style=file` (a .clang-format is provided).

---

## 4. Tests

* Use **unity** (platformio) for unit tests.
* Each new feature or bug-fix must add a test in `tests/`.
* Run `pio test` before pushing.

---

## 5. Commit / PR Style

* Keep commits atomic and descriptive:
  `Feat: Add rastertriangle2d helper`
  `Fix: Overflow in barycentric math (#231)`
* Link related issues with `fixes #id` or `closes #id`.
* Update **CHANGELOG.md** if the change is user-visible.

---

## 6. Code Review

* The maintainer reviews every pull request.
* Feel free to push follow-up commits.
* When CI passes and approvals are given, the maintainer will merge your pr.

---

## 7. Thank You

Again, thank you for considering to contribute or contributing. Open an issue any time you need clarification.
