#include <ptx/systems/scene/scene.hpp>

Scene::Scene(unsigned int maxMeshes) : maxMeshes(static_cast<int>(maxMeshes)) {
	meshes.reserve(this->maxMeshes);
}

void Scene::AddMesh(Mesh* mesh) {
	if (numMeshes >= static_cast<unsigned int>(maxMeshes)) {
		return;
	}

	if (numMeshes < meshes.size()) {
		meshes[numMeshes] = mesh;
	} else {
		meshes.push_back(mesh);
	}
	numMeshes = static_cast<unsigned int>(meshes.size());
}

void Scene::RemoveElement(unsigned int element) {
	if (element >= meshes.size()) {
		return;
	}

	meshes.erase(meshes.begin() + element);
	numMeshes = static_cast<unsigned int>(meshes.size());
}

void Scene::RemoveMesh(unsigned int i) {
	if (i < numMeshes) {
		RemoveElement(i);
	}
}

void Scene::RemoveMesh(Mesh* mesh) {
	for (unsigned int i = 0; i < numMeshes; i++) {
		if (meshes[i] == mesh) {
			RemoveElement(i);
			break;
		}
	}
}

Mesh** Scene::GetMeshes() {
	return meshes.data();
}

uint8_t Scene::GetMeshCount() {
	return static_cast<uint8_t>(numMeshes);
}

uint32_t Scene::GetTotalTriangleCount() const {
    uint32_t count = 0;
    for (unsigned int i = 0; i < numMeshes; ++i) {
        if (meshes[i] && meshes[i]->IsEnabled()) {
            count += meshes[i]->GetTriangleGroup()->GetTriangleCount();
        }
    }
    return count;
}