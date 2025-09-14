// registry_test.cpp
#include <iostream>
#include <cstring>
#include <memory>

#include "../lib/ptx/core/color/rgbcolor.hpp"      // your RGBColor (with PTX_* macros)
#include "../lib/ptx/registry/demangle.hpp"         // Demangle()
#include "../lib/ptx/registry/reflect_helpers.hpp"  // FieldRef/FieldSet/FindField/FindMethod

static void printColor(const char* label, const RGBColor& c) {
    std::cout << label << " = (" << int(c.R) << "," << int(c.G) << "," << int(c.B) << ")\n";
}

int main() {
    // --- Enumerate fields
    std::cout << "=== Reflection: enumerate fields ===\n";
    const auto fields = RGBColor::Fields();
    for (size_t i = 0; i < fields.count; ++i) {
        const auto& fd = fields.data[i];
        std::cout << "  Field[" << i << "]: name=" << fd.name
                  << ", size=" << fd.size
                  << ", type=" << Demangle(fd.type->name()) << "\n";
    }

    // --- Basic object and field get/set via reflection
    RGBColor c{10, 20, 30};
    printColor("Initial", c);

    if (const auto* Rfd = ptx::FindField(fields, "R")) {
        auto& R = ptx::FieldRef<uint8_t>(&c, *Rfd);
        std::cout << "Read R -> " << unsigned(R) << "\n";
    }
    if (const auto* Gfd = ptx::FindField(fields, "G")) {
        ptx::FieldSet<uint8_t>(&c, *Gfd, 42);
    }
    printColor("After setting G=42 via FieldAccess", c);

    // --- Methods: enumerate and call

    const auto methods = RGBColor::Methods();
    for (size_t i = 0; i < methods.count; ++i) {
        const auto& md = methods.data[i];
        std::cout << "  Method[" << i << "]: " << PrettySignature(md) << "\n";
    }

    // Call instance method: Add(uint8_t)
    if (const auto* addM = ptx::FindMethod(methods, "Add")) {
        uint8_t addVal = 15;
        void* argv[] = { &addVal };
        std::unique_ptr<RGBColor> ret(
            static_cast<RGBColor*>(addM->invoker(&c, argv))
        );
        if (ret) printColor("Add(+15) ->", *ret);
    }

    // Call instance method: Scale(uint8_t)
    if (const auto* scaleM = ptx::FindMethod(methods, "Scale")) {
        uint8_t maxB = 128;
        void* argv[] = { &maxB };
        std::unique_ptr<RGBColor> ret(
            static_cast<RGBColor*>(scaleM->invoker(&c, argv))
        );
        if (ret) printColor("Scale(128) ->", *ret);
    }

    // Call instance method: HueShift(float)
    if (const auto* hueM = ptx::FindMethod(methods, "HueShift")) {
        float deg = 90.0f;
        void* argv[] = { &deg };
        std::unique_ptr<RGBColor> ret(
            static_cast<RGBColor*>(hueM->invoker(&c, argv))
        );
        if (ret) printColor("HueShift(90deg) ->", *ret);
    }

    // Call instance method returning UString: ToString()
    if (const auto* toStr = ptx::FindMethod(methods, "ToString")) {
        // no args
        std::unique_ptr<ptx::UString> s(
            static_cast<ptx::UString*>(toStr->invoker(&c, nullptr))
        );
        if (s) {
            std::cout << "ToString() -> " << s->CStr() << "\n";
        }
    }

    // Call static method: InterpolateColors(const RGBColor&, const RGBColor&, const float&)
    if (const auto* lerpM = ptx::FindMethod(methods, "InterpolateColors")) {
        RGBColor a{0, 0, 0}, b{255, 128, 64};
        float t = 0.25f;
        void* argv[] = { &a, &b, &t };
        std::unique_ptr<RGBColor> ret(
            static_cast<RGBColor*>(lerpM->invoker(nullptr, argv))
        );
        if (ret) printColor("InterpolateColors(a,b,0.25) ->", *ret);
    }

    // Round-trip const read check
    if (const auto* Bfd = ptx::FindField(fields, "B")) {
        ptx::FieldSet<uint8_t>(&c, *Bfd, 200);
        const auto& Bc = ptx::FieldCref<uint8_t>(&c, *Bfd);
        std::cout << "Round-trip B set -> " << unsigned(Bc) << "\n";
    }

    // --- Constructors via Describe()
    const auto& cd = RGBColor::Describe();

    for (size_t i = 0; i < cd.ctor_count; ++i) {
        std::cout << "  Ctor[" << i << "]: " << PrettyCtor(cd.name, cd.ctors[i]) << "\n";
    }

    // pick the 3-arg ctor if available, else the default
    const ptx::ConstructorDesc* pick = nullptr;
    for (size_t i = 0; i < cd.ctor_count; ++i) {
        const auto& ct = cd.ctors[i];
        if (ct.arg_types.count == 3) { pick = &ct; break; }
        if (!pick && ct.arg_types.count == 0) pick = &ct;
    }

    if (pick) {
        std::unique_ptr<RGBColor, void(*)(RGBColor*)> owned(
            [&](){
                if (pick->arg_types.count == 3) {
                    uint8_t r = 7, g = 33, b = 99;
                    void* argv[] = { &r, &g, &b };
                    return static_cast<RGBColor*>(pick->invoker(argv));
                } else {
                    // default
                    return static_cast<RGBColor*>(pick->invoker(nullptr));
                }
            }(),
            [](RGBColor* p){ RGBColor::Describe().destroy(p); }
        );

        if (owned) {
            printColor("Constructed via Describe()", *owned);
            // also exercise ToString reflectively on the constructed instance
            if (const auto* toStr2 = ptx::FindMethod(methods, "ToString")) {
                std::unique_ptr<ptx::UString> s(
                    static_cast<ptx::UString*>(toStr2->invoker(owned.get(), nullptr))
                );
                if (s) {
                    std::cout << "Constructed->ToString() -> " << s->CStr() << "\n";
                }
            }
        }
    }

    std::cout << "=== Done ===\n";
    return 0;
}
