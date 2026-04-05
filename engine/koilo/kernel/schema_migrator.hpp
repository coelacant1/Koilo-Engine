// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file schema_migrator.hpp
 * @brief Registry for data migration functions between schema versions.
 *
 * Each serialized format registers migration steps that transform data
 * from version N to N+1. The migrator chains these steps to upgrade
 * data from any old version to the current version.
 *
 * Usage:
 *   SchemaMigrator migrator;
 *   migrator.Register("config", 1, 2, [](MigrationContext& ctx) {
 *       ctx.Rename("old.key", "new.key");
 *   });
 *   migrator.Register("config", 2, 3, [](MigrationContext& ctx) {
 *       ctx.SetDefault("new.option", "true");
 *   });
 *
 *   auto result = migrator.Migrate("config", 1, 3, data);
 *
 * @date 03/30/2026
 * @author Coela
 */
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace koilo {

/// Key-value migration context passed to migration functions.
/// Provides helpers for common transforms (rename, remove, set default).
class MigrationContext {
public:
    using KVMap = std::unordered_map<std::string, std::string>;

    explicit MigrationContext(KVMap& data) : data_(data) {}

    /// Rename a key (preserving value). Returns true if key existed.
    bool Rename(const std::string& oldKey, const std::string& newKey) {
        auto it = data_.find(oldKey);
        if (it == data_.end()) return false;
        data_[newKey] = std::move(it->second);
        data_.erase(it);
        return true;
    }

    /// Remove a key. Returns true if key existed.
    bool Remove(const std::string& key) {
        return data_.erase(key) > 0;
    }

    /// Set a value only if the key doesn't already exist.
    void SetDefault(const std::string& key, const std::string& value) {
        data_.emplace(key, value);
    }

    /// Set a value unconditionally.
    void Set(const std::string& key, const std::string& value) {
        data_[key] = value;
    }

    /// Get a value (empty string if not present).
    std::string Get(const std::string& key) const {
        auto it = data_.find(key);
        return (it != data_.end()) ? it->second : "";
    }

    /// Check if a key exists.
    bool Has(const std::string& key) const {
        return data_.find(key) != data_.end();
    }

    /// Direct access to the underlying map.
    KVMap& Data() { return data_; }

private:
    KVMap& data_;
};

/// A single migration step from one version to the next.
using MigrationFunc = std::function<void(MigrationContext&)>;

/// Result of a migration attempt.
struct MigrationResult {
    bool     success     = false;
    uint32_t fromVersion = 0;
    uint32_t toVersion   = 0;
    int      stepsApplied = 0;
    std::string error;
};

/// Registry for versioned data migration functions.
class SchemaMigrator {
public:
    SchemaMigrator() = default;

    /// Register a migration from `fromVersion` to `toVersion` for a format.
    void Register(const std::string& format, uint32_t fromVersion,
                  uint32_t toVersion, MigrationFunc func) {
        steps_[format].push_back({fromVersion, toVersion, std::move(func)});
    }

    /// Migrate data from oldVersion to targetVersion by chaining steps.
    MigrationResult Migrate(const std::string& format, uint32_t oldVersion,
                            uint32_t targetVersion,
                            MigrationContext::KVMap& data) const {
        MigrationResult result;
        result.fromVersion = oldVersion;
        result.toVersion = targetVersion;

        if (oldVersion == targetVersion) {
            result.success = true;
            return result;
        }

        if (oldVersion > targetVersion) {
            result.error = "Downgrade not supported";
            return result;
        }

        auto it = steps_.find(format);
        if (it == steps_.end()) {
            result.error = "No migrations registered for format '" + format + "'";
            return result;
        }

        MigrationContext ctx(data);
        uint32_t current = oldVersion;

        while (current < targetVersion) {
            bool found = false;
            for (auto& step : it->second) {
                if (step.from == current && step.to == current + 1) {
                    step.func(ctx);
                    ++current;
                    ++result.stepsApplied;
                    found = true;
                    break;
                }
            }
            if (!found) {
                result.error = "No migration from v" + std::to_string(current) +
                               " to v" + std::to_string(current + 1) +
                               " for format '" + format + "'";
                result.toVersion = current;
                return result;
            }
        }

        result.success = true;
        result.toVersion = current;
        return result;
    }

    /// Check if a migration path exists from oldVersion to targetVersion.
    bool CanMigrate(const std::string& format, uint32_t oldVersion,
                    uint32_t targetVersion) const {
        if (oldVersion >= targetVersion) return oldVersion == targetVersion;

        auto it = steps_.find(format);
        if (it == steps_.end()) return false;

        uint32_t current = oldVersion;
        while (current < targetVersion) {
            bool found = false;
            for (auto& step : it->second) {
                if (step.from == current && step.to == current + 1) {
                    ++current;
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        return true;
    }

    /// List all registered formats.
    std::vector<std::string> Formats() const {
        std::vector<std::string> result;
        for (auto& [k, _] : steps_) result.push_back(k);
        return result;
    }

    /// Get the highest registered target version for a format.
    uint32_t LatestVersion(const std::string& format) const {
        auto it = steps_.find(format);
        if (it == steps_.end()) return 0;
        uint32_t max = 0;
        for (auto& step : it->second) {
            if (step.to > max) max = step.to;
        }
        return max;
    }

private:
    struct Step {
        uint32_t      from;
        uint32_t      to;
        MigrationFunc func;
    };
    std::unordered_map<std::string, std::vector<Step>> steps_;
};

} // namespace koilo
