# PTX Engine Tests

This directory contains C++ unit tests for core mathematical and utility components of the PTX Engine. The test harness uses the Unity framework vendored under `external/unity` and is built into a single test executable target via CMake: `ptx_tests`.

## Building Tests

Tests are enabled by default (`PTX_BUILD_TESTS=ON`). Build with:

```bash
cmake -S . -B build -DPTX_BUILD_TESTS=ON
cmake --build build -j
```

The executable will be at:
```
build/ptx_tests
```

## Running Tests

You can run tests directly:
```bash
./build/ptx_tests
```

Or via CTest (which integrates with IDEs and CI):
```bash
cd build
ctest --output-on-failure
```

## Adding a Test

1. Create a `test<feature>.cpp` (and optionally a matching header) in this directory.
2. Add Unity `TEST_GROUP` / `TEST` cases or continue following the existing style (each .cpp aggregates related checks with simple assert-like calls).
3. The `file(GLOB ...)` in `CMakeLists.txt` will auto-pickup new `*.cpp` files; rebuild to include them.

## Test Focus Areas

Current tests exercise:
* Vector and quaternion math
* Rotation / rotation matrix conversions
* FFT and signal utilities (basic correctness checks)

## Writing Assertions

Unity's minimal API is used. Prefer small, single-purpose tests. Consider adding helper functions inside the test translation unit (not shared headers) to avoid coupling tests.

## Planned Improvements

* Add CI workflow to run tests on each push.
* Introduce property-based or randomized tests for quaternion edge cases (generation scripts exist under `tests/utils`).
* Add performance micro-benchmarks (separate target) to catch regressions.

## Troubleshooting

* If a new test file is not building, ensure it matches the glob pattern (`tests/engine/*.cpp` or correct directory after restructure).
* If linking fails due to missing symbols, verify the symbol belongs in `ptx_core` or adjust the library linkage in the top-level CMake.

