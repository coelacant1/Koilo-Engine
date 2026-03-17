// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/scene/scene.hpp>


namespace koilo {

koilo::Scene::Scene() {
}

koilo::Scene::~Scene() {
    // ownedNodes_ cleaned up by unique_ptr destructors
}

void koilo::Scene::AddMesh(Mesh* mesh) {
	meshes.push_back(mesh);
	numMeshes = static_cast<unsigned int>(meshes.size());
}

void koilo::Scene::RemoveElement(unsigned int element) {
	if (element >= meshes.size()) {
		return;
	}

	meshes.erase(meshes.begin() + element);
	numMeshes = static_cast<unsigned int>(meshes.size());
}

void koilo::Scene::RemoveMesh(unsigned int i) {
	if (i < numMeshes) {
		RemoveElement(i);
	}
}

void koilo::Scene::RemoveMesh(Mesh* mesh) {
	for (unsigned int i = 0; i < numMeshes; i++) {
		if (meshes[i] == mesh) {
			RemoveElement(i);
			break;
		}
	}
}

Mesh** koilo::Scene::GetMeshes() {
	return meshes.data();
}

unsigned int koilo::Scene::GetMeshCount() {
	return numMeshes;
}

Mesh* koilo::Scene::GetMesh(unsigned int index) {
    if (index >= numMeshes) return nullptr;
    return meshes[index];
}

uint32_t koilo::Scene::GetTotalTriangleCount() const {
    uint32_t count = 0;
    for (unsigned int i = 0; i < numMeshes; ++i) {
        if (meshes[i] && meshes[i]->IsEnabled()) {
            count += meshes[i]->GetTriangleGroup()->GetTriangleCount();
        }
    }
    return count;
}

SceneNode* koilo::Scene::CreateObject(const std::string& name) {
    auto it = nodesByName_.find(name);
    if (it != nodesByName_.end()) {
        return it->second;  // already exists
    }
    ownedNodes_.push_back(std::make_unique<SceneNode>(name));
    auto* node = ownedNodes_.back().get();
    nodesByName_[name] = node;
    return node;
}

SceneNode* koilo::Scene::Find(const std::string& name) {
    auto it = nodesByName_.find(name);
    return (it != nodesByName_.end()) ? it->second : nullptr;
}

std::size_t koilo::Scene::GetNodeCount() const {
    return ownedNodes_.size();
}

} // namespace koilo