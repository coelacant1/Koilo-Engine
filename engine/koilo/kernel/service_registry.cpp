#include <koilo/kernel/service_registry.hpp>

namespace koilo {

void ServiceRegistry::Register(const std::string& name, void* service) {
    services_[name] = service;
}

void ServiceRegistry::Unregister(const std::string& name) {
    services_.erase(name);
}

void* ServiceRegistry::GetRaw(const std::string& name) const {
    auto it = services_.find(name);
    return it != services_.end() ? it->second : nullptr;
}

bool ServiceRegistry::Has(const std::string& name) const {
    return services_.find(name) != services_.end();
}

std::vector<std::string> ServiceRegistry::List() const {
    std::vector<std::string> names;
    names.reserve(services_.size());
    for (auto& [name, _] : services_) {
        names.push_back(name);
    }
    return names;
}

} // namespace koilo
