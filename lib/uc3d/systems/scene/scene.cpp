#include "scene.hpp"

Scene::Scene(unsigned int maxMeshes) : maxMeshes(maxMeshes) {
	meshes = new Mesh*[maxMeshes];
}

Scene::~Scene() {
	delete[] meshes;
}

void Scene::AddMesh(Mesh* mesh) {
	meshes[numMeshes] = mesh;
	numMeshes++;
}

void Scene::RemoveElement(unsigned int element) {
	for (unsigned int i = element; i < numMeshes - 1; i++) {
		meshes[i] = meshes[i + 1];
	}
}

void Scene::RemoveMesh(unsigned int i) {
	if (i < numMeshes && i >= 0) {
		RemoveElement(i);
		numMeshes--;
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
	return meshes;
}

uint8_t Scene::GetMeshCount() {
	return numMeshes;
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