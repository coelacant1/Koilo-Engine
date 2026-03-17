#include <koilo/kernel/console/console_commands.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/registry/global_registry.hpp>
#include <koilo/registry/registry.hpp>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace koilo {

static const ClassDesc* FindClass(const std::string& name) {
    auto& map = ClassRegistryMap();
    auto it = map.find(name);
    return it != map.end() ? it->second : nullptr;
}

static const char* FieldKindStr(FieldKind kind) {
    switch (kind) {
        case FieldKind::Float:   return "float";
        case FieldKind::Int:     return "int";
        case FieldKind::Double:  return "double";
        case FieldKind::Bool:    return "bool";
        case FieldKind::UInt8:   return "uint8";
        case FieldKind::UInt16:  return "uint16";
        case FieldKind::UInt32:  return "uint32";
        case FieldKind::SizeT:   return "size_t";
        case FieldKind::Complex: return "complex";
        default:                 return "unknown";
    }
}

void RegisterReflectionCommands(CommandRegistry& registry) {
    // -- classes --
    registry.Register({"classes", "classes [filter]", "List all reflected classes",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            auto& classes = GlobalClassRegistry();
            std::string filter = args.empty() ? "" : args[0];

            std::ostringstream ss;
            int count = 0;
            for (auto* cls : classes) {
                if (!cls || !cls->name) continue;
                std::string name = cls->name;
                if (!filter.empty() && name.find(filter) == std::string::npos) continue;
                ss << "  " << name
                   << " (" << cls->fields.count << " fields, "
                   << cls->methods.count << " methods)\n";
                ++count;
            }

            std::ostringstream header;
            header << "Reflected classes";
            if (!filter.empty()) header << " matching '" << filter << "'";
            header << " (" << count << "):\n";
            return ConsoleResult::Ok(header.str() + ss.str());
        }, nullptr
    });

    // -- inspect --
    registry.Register({"inspect", "inspect <class_name>", "Show fields and methods of a reflected class",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty()) return ConsoleResult::Error("Usage: inspect <class_name>");

            const ClassDesc* cls = FindClass(args[0]);
            if (!cls) return ConsoleResult::NotFound("Class not found: " + args[0]);

            std::ostringstream ss;
            ss << "Class: " << cls->name << " (size=" << cls->size << " bytes)\n";

            if (cls->fields.count > 0) {
                ss << "\n  Fields:\n";
                for (size_t i = 0; i < cls->fields.count; ++i) {
                    auto& f = cls->fields.data[i];
                    ss << "    " << std::left << std::setw(20) << f.name
                       << " " << std::setw(8) << FieldKindStr(f.kind);
                    if (f.description && f.description[0]) {
                        ss << "  // " << f.description;
                    }
                    if (f.min_value != 0.0 || f.max_value != 0.0) {
                        ss << "  [" << f.min_value << ".." << f.max_value << "]";
                    }
                    ss << "\n";
                }
            }

            if (cls->methods.count > 0) {
                ss << "\n  Methods:\n";
                for (size_t i = 0; i < cls->methods.count; ++i) {
                    auto& m = cls->methods.data[i];
                    ss << "    " << m.name << "()\n";
                }
            }

            if (cls->ctor_count > 0) {
                ss << "\n  Constructors: " << cls->ctor_count << "\n";
            }

            // JSON
            std::ostringstream js;
            js << "{\"name\":\"" << cls->name
               << "\",\"size\":" << cls->size
               << ",\"fields\":" << cls->fields.count
               << ",\"methods\":" << cls->methods.count << "}";

            return ConsoleResult::Ok(ss.str(), js.str());
        }, [](KoiloKernel&, const std::vector<std::string>&, const std::string& partial) -> std::vector<std::string> {
            auto& classes = GlobalClassRegistry();
            std::vector<std::string> matches;
            for (auto* cls : classes) {
                if (!cls || !cls->name) continue;
                std::string name = cls->name;
                if (partial.empty() || name.find(partial) == 0) {
                    matches.push_back(name);
                }
            }
            std::sort(matches.begin(), matches.end());
            return matches;
        }
    });
}

} // namespace koilo
