#include <iostream>
#include <cstring>
#include <memory>
#include "../lib/ptx/core/color/rgbcolor.hpp"
#include "../lib/ptx/registry/demangle.hpp"
#include "../lib/ptx/registry/reflect_helpers.hpp"

// Spaghetti

static const ptx::FieldDecl* FindField(const ptx::FieldList& fl, const char* name) {
    for (size_t i = 0; i < fl.count; ++i)
        if (std::strcmp(fl.data[i].name, name) == 0) return &fl.data[i];
    return nullptr;
}
static const ptx::MethodDesc* FindMethod(const ptx::MethodList& ml, const char* name) {
    for (size_t i = 0; i < ml.count; ++i)
        if (std::strcmp(ml.data[i].name, name) == 0) return &ml.data[i];
    return nullptr;
}

static void printColor(const char* label, const RGBColor& c) {
    std::cout << label << " = (" << int(c.R) << "," << int(c.G) << "," << int(c.B) << ")\n";
}

int main() {
    std::cout << "=== Reflection: enumerate fields ===\n";
    auto fields = RGBColor::Fields();
    for (size_t i = 0; i < fields.count; ++i) {
        const auto& fd = fields.data[i];
        std::cout << "  Field[" << i << "]: name=" << fd.name
                  << ", size=" << fd.size
                  << ", type=" << Demangle(fd.type->name())
                  << "\n";
    }

    RGBColor c{10, 20, 30};
    printColor("Initial", c);

    // Read/write via helpers
    if (auto* Rfd = FindField(fields, "R")) {
        auto& R = FieldRef<uint8_t>(&c, *Rfd);
        std::cout << "Read R -> " << unsigned(R) << "\n";
    }
    if (auto* Gfd = FindField(fields, "G")) {
        FieldSet<uint8_t>(&c, *Gfd, 42);
    }
    printColor("After setting G=42 via FieldAccess", c);

    // Methods
    auto methods = RGBColor::Methods();

    if (auto* addM = FindMethod(methods, "Add")) {
        uint8_t addVal = 15;
        std::unique_ptr<RGBColor> ret(InvokeRet<RGBColor>(*addM, &c, &addVal));
        if (ret) printColor("Add(+15) ->", *ret);
    }

    if (auto* scaleM = FindMethod(methods, "Scale")) {
        uint8_t maxB = 128;
        std::unique_ptr<RGBColor> ret(InvokeRet<RGBColor>(*scaleM, &c, &maxB));
        if (ret) printColor("Scale(128) ->", *ret);
    }

    if (auto* hueM = FindMethod(methods, "HueShift")) {
        float deg = 90.0f;
        std::unique_ptr<RGBColor> ret(InvokeRet<RGBColor>(*hueM, &c, &deg));
        if (ret) printColor("HueShift(90deg) ->", *ret);
    }

    if (auto* toStr = FindMethod(methods, "ToString")) {
        std::unique_ptr<ptx::UString> s(InvokeRet<ptx::UString>(*toStr, &c));
        if (s) {
            std::cout << "ToString() -> " << s->CStr() << "\n";
        }
    }

    // Static InterpolateColors(a,b,t)
    if (auto* lerpM = FindMethod(methods, "InterpolateColors")) {
        RGBColor a{0, 0, 0}, b{255, 128, 64};
        float t = 0.25f;
        std::unique_ptr<RGBColor> ret(InvokeRet<RGBColor>(*lerpM, nullptr, &a, &b, &t));
        if (ret) printColor("InterpolateColors(a,b,0.25) ->", *ret);
    }

    // Round trip const read check
    if (auto* Bfd = FindField(fields, "B")) {
        FieldSet<uint8_t>(&c, *Bfd, 200);
        auto& Bc = FieldCref<uint8_t>(&c, *Bfd);
        std::cout << "Round-trip B set -> " << unsigned(Bc) << "\n";
    }

    std::cout << "=== Done ===\n";
    return 0;
}
