#pragma once
#include <string>
#include <typeinfo>

#if defined(__GNUG__)
    #include <cxxabi.h>
    inline std::string Demangle(const char* n) {
        int status = 0; char* p = abi::__cxa_demangle(n, nullptr, nullptr, &status);
        std::string s = (status == 0 && p) ? p : n;
        std::free(p);

        return s;
    }
#elif defined(_MSC_VER)
    #include <windows.h>
    #include <dbghelp.h>
    #pragma comment(lib, "Dbghelp.lib")

    inline std::string Demangle(const char* n) {
        char out[1024];

        if (UnDecorateSymbolName(n, out, sizeof(out), UNDNAME_COMPLETE)) return out;
        
        return n;
    }
#else
  inline std::string Demangle(const char* n) { return n; }
#endif
