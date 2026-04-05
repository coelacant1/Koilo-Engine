// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/kernel/config/config_store.hpp>
#include <koilo/kernel/cvar/cvar_system.hpp>
#include <koilo/kernel/logging/log.hpp>
#include <koilo/kernel/schema_version.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>

namespace koilo {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string Trim(const std::string& s) {
    auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return {};
    auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

static std::string ValueToString(const ConfigStore::Value& v) {
    struct Visitor {
        std::string operator()(bool b)               const { return b ? "true" : "false"; }
        std::string operator()(int i)                 const { return std::to_string(i); }
        std::string operator()(float f)               const {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%g", f);
            return buf;
        }
        std::string operator()(const std::string& s)  const { return "\"" + s + "\""; }
    };
    return std::visit(Visitor{}, v);
}

static ConfigStore::Value ParseValue(const std::string& raw) {
    if (raw == "true")  return ConfigStore::Value{true};
    if (raw == "false") return ConfigStore::Value{false};

    // Quoted string
    if (raw.size() >= 2 && raw.front() == '"' && raw.back() == '"')
        return ConfigStore::Value{raw.substr(1, raw.size() - 2)};

    // Try int (no decimal point)
    if (raw.find('.') == std::string::npos) {
        try { return ConfigStore::Value{std::stoi(raw)}; }
        catch (...) {}
    }
    // Try float
    try { return ConfigStore::Value{std::stof(raw)}; }
    catch (...) {}

    // Fallback: unquoted string
    return ConfigStore::Value{raw};
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------

bool ConfigStore::GetBool(const char* key, bool fallback) const {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = entries_.find(key);
    if (it == entries_.end()) return fallback;
    if (auto* v = std::get_if<bool>(&it->second))        return *v;
    if (auto* v = std::get_if<int>(&it->second))          return *v != 0;
    return fallback;
}

int ConfigStore::GetInt(const char* key, int fallback) const {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = entries_.find(key);
    if (it == entries_.end()) return fallback;
    if (auto* v = std::get_if<int>(&it->second))          return *v;
    if (auto* v = std::get_if<float>(&it->second))        return static_cast<int>(*v);
    if (auto* v = std::get_if<bool>(&it->second))         return *v ? 1 : 0;
    return fallback;
}

float ConfigStore::GetFloat(const char* key, float fallback) const {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = entries_.find(key);
    if (it == entries_.end()) return fallback;
    if (auto* v = std::get_if<float>(&it->second))        return *v;
    if (auto* v = std::get_if<int>(&it->second))          return static_cast<float>(*v);
    return fallback;
}

const char* ConfigStore::GetString(const char* key, const char* fallback) const {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = entries_.find(key);
    if (it == entries_.end()) return fallback;
    if (auto* v = std::get_if<std::string>(&it->second)) {
        stringBuf_ = *v;
        return stringBuf_.c_str();
    }
    // Coerce non-string types
    stringBuf_ = ValueToString(it->second);
    return stringBuf_.c_str();
}

// ---------------------------------------------------------------------------
// Setters
// ---------------------------------------------------------------------------

void ConfigStore::SetBool(const char* key, bool value) {
    std::lock_guard<std::mutex> lock(mu_);
    entries_[key] = value;
    dirty_ = true;
}

void ConfigStore::SetInt(const char* key, int value) {
    std::lock_guard<std::mutex> lock(mu_);
    entries_[key] = value;
    dirty_ = true;
}

void ConfigStore::SetFloat(const char* key, float value) {
    std::lock_guard<std::mutex> lock(mu_);
    entries_[key] = value;
    dirty_ = true;
}

void ConfigStore::SetString(const char* key, const char* value) {
    std::lock_guard<std::mutex> lock(mu_);
    entries_[key] = std::string(value);
    dirty_ = true;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

bool ConfigStore::Has(const char* key) const {
    std::lock_guard<std::mutex> lock(mu_);
    return entries_.count(key) > 0;
}

void ConfigStore::Remove(const char* key) {
    std::lock_guard<std::mutex> lock(mu_);
    entries_.erase(key);
    dirty_ = true;
}

std::vector<std::string> ConfigStore::Keys(const char* prefix) const {
    std::lock_guard<std::mutex> lock(mu_);
    std::vector<std::string> result;
    size_t plen = std::strlen(prefix);
    for (auto& [k, v] : entries_) {
        if (plen == 0 || k.compare(0, plen, prefix) == 0)
            result.push_back(k);
    }
    std::sort(result.begin(), result.end());
    return result;
}

size_t ConfigStore::Count() const {
    std::lock_guard<std::mutex> lock(mu_);
    return entries_.size();
}

// ---------------------------------------------------------------------------
// File I/O
// ---------------------------------------------------------------------------

bool ConfigStore::SaveToFile(const char* path) const {
    std::lock_guard<std::mutex> lock(mu_);
    std::ofstream out(path);
    if (!out) {
        KL_WARN("Config", "Failed to open %s for writing", path);
        return false;
    }
    out << MakeTextSchemaHeader("config", 1) << '\n';
    out << "# Koilo Engine configuration\n";

    // Sort keys for deterministic output
    std::vector<std::string> keys;
    keys.reserve(entries_.size());
    for (auto& [k, v] : entries_) keys.push_back(k);
    std::sort(keys.begin(), keys.end());

    std::string prevNs;
    for (auto& k : keys) {
        // Insert blank line between namespaces
        auto dot = k.find('.');
        std::string ns = (dot != std::string::npos) ? k.substr(0, dot) : "";
        if (!ns.empty() && ns != prevNs) {
            if (!prevNs.empty()) out << '\n';
            prevNs = ns;
        }
        out << k << " = " << ValueToString(entries_.at(k)) << '\n';
    }

    dirty_ = false;
    KL_DBG("Config", "Saved %zu entries to %s", entries_.size(), path);
    return true;
}

bool ConfigStore::LoadFromFile(const char* path) {
    std::lock_guard<std::mutex> lock(mu_);
    std::ifstream in(path);
    if (!in) return false;

    size_t loaded = 0;
    uint32_t fileVersion = 0;
    std::string line;
    while (std::getline(in, line)) {
        line = Trim(line);
        if (line.empty()) continue;

        // Check for version header on comment lines
        if (line[0] == '#') {
            if (fileVersion == 0) {
                fileVersion = ParseTextSchemaVersion(line, "config");
            }
            continue;
        }

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = Trim(line.substr(0, eq));
        std::string val = Trim(line.substr(eq + 1));
        if (key.empty()) continue;

        entries_[key] = ParseValue(val);
        ++loaded;
    }

    // Version 0 = legacy file without header (treated as v1)
    if (fileVersion == 0) fileVersion = 1;

    KL_DBG("Config", "Loaded %zu entries from %s (schema v%u)",
           loaded, path, fileVersion);
    return true;
}

// ---------------------------------------------------------------------------
// CVar Archive Sync
// ---------------------------------------------------------------------------

void ConfigStore::PullArchiveCVars() {
    auto& sys = CVarSystem::Get();
    sys.ForEach([this](const CVarParameter& p) {
        if (!HasFlag(p.flags, CVarFlags::Archive)) return;
        std::string key = "cvar." + p.name;
        switch (p.type) {
            case CVarType::Int:    entries_[key] = static_cast<int>(p.intVal);   break;
            case CVarType::Float:  entries_[key] = p.floatVal;                    break;
            case CVarType::Bool:   entries_[key] = p.boolVal;                     break;
            case CVarType::String: entries_[key] = std::string(p.strVal);         break;
        }
    });
}

void ConfigStore::PushArchiveCVars() const {
    auto& sys = CVarSystem::Get();
    for (auto& [k, v] : entries_) {
        if (k.compare(0, 5, "cvar.") != 0) continue;
        std::string cvarName = k.substr(5);
        auto* entry = sys.Find(cvarName);
        if (!entry || !HasFlag(entry->flags, CVarFlags::Archive)) continue;
        entry->SetFromString(ValueToString(v));
    }
}

} // namespace koilo
