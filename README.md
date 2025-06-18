# uMath3D - Mathematics Library for Embedded and Non-Embedded Devices

> **Work in progress** -- the structure may change without notice while working on the first major release.

This code is currently being migrated from ProtoTracer to act as a submodule and generalize the functionality for more use cases.
- Project Goals:
    - Have a generic C++ library that can be used as a backbone to speed up development on microcontrollers as well as desktop computers
    - Re-integrate this functionality as a sub-module into ProtoTracer
    - Create a desktop GUI application that can utilize and simulate the same functionality as the embedded pipeline. 

---

**uMath3D** is a C++17 library targeted for embedded microcontrollers that bundles mathematics, control, signals, and physics features:

* vector, quaternion, matrix math, and rotation conversion
* 2D / 3D / spatial geometry primitives
* signal-processing helpers (FFT, Kalman, filters, noise)
* lightweight physics utilities (springs, bounce, fluid-style vector fields)
* time-step & PID control helpers
* rendering with raytracing/rasterizing + post effects + shaders
* scene creation with animations, mesh deformations, volumetric lighting
* asset tools for custom fonts, images, meshes, and volumes

---

## Directory layout

```
include/um3d/   <- public headers (header-only where possible)
src/            <- .cpp for heavy implementations
examples/       <- minimal buildable snippets
test/           <- Unity unit tests
docs/           <- auto-generated API docs with Doxygen (WIP)
```

---

## Quick start

```bash
# Fetch
git clone https://github.com/coelacant1/uMath3D.git
```

> WIP

---

## Current modules

| Module                  | Highlights                                                                                 |
| ----------------------- | ------------------------------------------------------------------------------------------ |
| **math**                | vectors, quaternions, rotations, transforms                                                |
| **geometry**            | circles, rectangles, planes, bounding volumes                                              |
| **signal**              | FFT, Kalman / average / peak filters, function generator, Simplex noise                    |
| **physics::motion**     | damped spring, bounce dynamics                                                             |
| **physics::simulation** | vector‐field, boundary motion sim                                                          |
| **control**             | classic PID controller                                                                     |
| **time**                | fixed/variable timestep helper, micro-wait                                                 |

> WIP

---

## Planned / in-progress

* 4×4 matrix + projection utilities for graphics pipelines
* Additional procedural noise
* Doxygen-based website hosted on GitHub Pages

---

## Contributing

Contributions are welcome! To contribute:
1. Fork the repository on GitHub.
2. Commit your changes with a descriptive message (git commit -m 'Add YourFeature').
3. Push the branch (git push origin main).
4. Submit a pull request on GitHub.

---

## License

uMath3D is licensed under the [AGPL-3.0](https://choosealicense.com/licenses/agpl-3.0/). This ensures modifications and contributions benefit the community. If you use or modify this software for a product, you must make the modified code publicly available as per the license.
