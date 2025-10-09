# Contributing Guide

Thank you for considering a contribution to **PTX Engine**.
This document explains the rules every pull-request must follow.

---

## 1. Project Scope

PTX Engine is a **header / source** c++17 library that targets

* Arduino-class mcus (avr, cortex-m, esp32)
* Bare-metal embedded c++
* Desktop (clang / gcc / msvc)

---

## 2. Getting Started

### 2.1 Prerequisites

Install the following tooling before you begin:

* **CMake 3.20+** and a C++17 compiler (clang, gcc, or msvc)
* **Python 3.10+** for code generation scripts under `scripts/`
* **PlatformIO CLI** *(optional but recommended)* for embedded/unit builds

### 2.2 Workflow

1. **Fork & Clone**

    ```bash
    git clone https://github.com/coelacant1/ptxengine.git
    cd ptxengine
    ```

2. **Create a Topic Branch**: `git checkout -b feat/<short-topic>` (use prefixes `feat/`, `fix/`, `docs/`, or `refactor/`).

3. **Configure & Build (desktop/native)**

    ```bash
    cmake -S . -B build
    cmake --build build -j
    ```

  *Running `./build.sh` is an alternative wrapper that performs the same steps plus code generation.*

4. **Run Tests**

    ```bash
    cmake --build build -j --target ptx_tests
    cd build
    ctest --output-on-failure
    ```

    Optional PlatformIO suite (native + teensy) when your change touches embedded code:

    ```bash
    pio test                 # native
    pio test -e teensy40     # teensy40
    ```

5. **Make Changes**

    * Follow the coding rules below, including Doxygen comment requirements.
    * Regenerate reflection bindings if you touched registry descriptors:

      ```bash
      python scripts/GeneratePTXAll.py
      ```

6. **Commit & Push**

    ```bash
    git commit -m "Feat: Short summary (fixes #123)"
    git push --set-upstream origin feat/<short-topic>
    ```

7. **Open a Pull Request** against `main`, filling in the template checklist.

---

## 3. Coding Rules

### 3.1 Filenames & Directories

* Lower-case, no underscores: `triangle2d.hpp`, `triangle2d.cpp`.

### 3.2 Namespaces

* **No namespaces for now.** Every type lives in the root scope - this may change later.

### 3.3 Functions & Variables

| item           | style example       |
| -------------- | ------------------- |
| Function names | `ComputeNormals()`  |
| Member vars    | `float edgeLength;` |
| Local vars     | `float baryCenter;` |
| Public vars    | `float BaryCenter;` |
| Braces         | `void Foo() {`      |

### 3.4 Headers & Comments

All contributions **must** retain and extend the project-wide Doxygen documentation. Follow these rules:

1. **File header block** - start every translation unit and header with a `/** ... */` block containing at least:
  - `@file` - canonical file name
  - `@brief` - one-sentence summary
  - Optional supporting lines (`@date`, `@version`, `@author`, etc.)
  - Additional prose paragraphs may follow the tagged lines to explain context or usage.

2. **Class / struct documentation** - place a `/** */` block immediately above each type declaration with:
  - `@class` (or `@struct`) and `@brief`
  - Optional `@details` section describing design intent, invariants, ownership rules, threading expectations, etc.
  - Enumerate important invariants as bullet points inside the comment when relevant.

3. **Functions and methods** - each member function (including constructors/destructors) requires:
  - `@brief`
  - `@details` when behaviour warrants explanation beyond one sentence
  - `@param` tags for every parameter (use `[in]`, `[out]`, `[in,out]` annotations when clarity helps)
  - `@return` for non-`void` functions, describing ownership and lifetime semantics of returned pointers/references.

4. **Data members** - add `///<` trailing comments (or preceding `/** */` blocks when multiline description is required) explaining meaning, ownership, ranges, and invariants.

5. **Do not remove existing documentation.** You may tighten wording or reflow text, but never drop semantic information. When existing comments become outdated, update them instead of deleting.

6. **Minimalism is fine.** If behaviour is obvious, keep the comment concise but still present.

#### Canonical skeleton

```cpp
/**
 * @file Foo.hpp
 * @brief Short summary of the file contents.
 *
 * Additional paragraphs may elaborate on runtime expectations, usage scenarios,
 * or references to related modules.
 */

/**
 * @class Foo
 * @brief Summarises the role of the type.
 * @details Optional multi-line description including invariants such as:
 *          - bullet points for ownership rules
 *          - constraints on method call ordering
 */
class Foo {
public:
   /**
    * @brief Construct a Foo from a data source.
    * @param[in] source Pointer to valid source data (must not be nullptr).
    */
   explicit Foo(const Source* source);

   /**
    * @brief Process the stored data.
    * @param[in] delta Time step in seconds.
    * @return Resulting processed value (caller owns the copy).
    */
   Result Process(float delta);

private:
   Result cache; ///< Cached output reused across frames.
};
```

Use this skeleton as the baseline for new files and when refactoring existing ones.

### 3.5 Includes

* Only `<stdint.h>`, `<stddef.h>`, `<math.h>`, and other c++17 headers.
* **Never** `Arduino.h` in public headers. Use it only inside `#if defined(ARDUINO)` guards in `.cpp`.

### 3.6 Memory & Containers

* Prefer stack allocation and deterministic lifetimes. Avoid heap churn inside real-time loops.
* `std::vector` and `std::string` are allowed when they meaningfully reduce complexity, but:
  * Reserve capacity up front (`reserve()`/`resize()`), 
  * Reuse instances instead of re-allocating per frame,
  * Do **not** perform dynamic growth inside the hot path of render or control loops.
* For predictable workloads (tight MCU loops, DSP kernels), favour fixed-size containers or caller-supplied buffers.
* Never leak ownership; pair each `new` with strict RAII wrappers or smart pointers. Raw `new/delete` should be avoided outside low-level allocators.

### 3.7 Formatting Helper

Run `clang-format -style=file` (a .clang-format is provided).

---

## 4. Tests

* Every feature or bug fix must include coverage in either the desktop suite (`ptx_tests`) or the PlatformIO suites (native/teensy) when embedded behaviour is affected.
* **Desktop workflow**

  ```bash
  cmake --build build -j --target ptx_tests
  cd build
  ctest --output-on-failure
  ```

* **Embedded/PlatformIO workflow (optional / whenever MCU code changes)**

  ```bash
  pio test                 # native host simulation
  pio test -e teensy40     # teensy40 target
  ```

* Add or update tests under `tests/` (unity-based) or `engine/src/...` integration harnesses as appropriate. Tests should be deterministic and avoid timing-based assertions.

---

## 5. Commit / PR Style

### 5.1 Commit messages

* Keep commits **atomic** and focused on one logical change.
* Prefix messages with conventional tags:
  * `Feat:` for new functionality
  * `Fix:` for bug fixes
  * `Refactor:` for structural changes
  * `Docs:` for documentation-only updates
* Suffix with issue references where applicable: `Fix: Stabilise quadtree build (fixes #231)`.
* Re-run formatting (`clang-format`) and relevant generators before committing.

### 5.2 Pull request expectations

* Squash noisy WIP commits before requesting review.
* Update **CHANGELOG.md** for user-visible changes.
* Include summaries of:
  * Why the change is needed
  * How it was implemented (mention scripts or regeneration steps)
  * The verification performed (build + test commands)

## 6. Pull Request Checklist

Before marking a PR ready for review, confirm the following:

- [ ] `cmake --build build -j` succeeds without warnings introduced by your change.
- [ ] `ctest --output-on-failure` (and, if applicable, relevant `pio test` invocations) pass.
- [ ] Doxygen headers/classes/methods are present and updated; no documentation removed.
- [ ] Reflection/code generation scripts (`scripts/GeneratePTXAll.py`, etc.) have been run when registry descriptors changed.
- [ ] `clang-format -style=file` has been applied to touched files.
- [ ] CHANGELOG entries, README snippets, and sample code updated when behaviour changes.
- [ ] New files follow the canonical skeleton from section 3.4.
- [ ] No stray debugging output, TODOs, or commented-out code remain.

Attach the checklist (checked items) in the PR description.

---

## 7. Code Review

* The maintainer reviews every pull request.
* Feel free to push follow-up commits.
* When CI passes and approvals are given, the maintainer will merge your PR.

---

## 8. Thank You

Again, thank you for considering to contribute or contributing. Open an issue any time you need clarification.
