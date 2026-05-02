// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/scene/scene.hpp>
#include <koilo/systems/scene/meshraycast.hpp>
#include <koilo/systems/physics/raycasthit.hpp>
#include <koilo/core/geometry/ray.hpp>
#include <koilo/core/math/transform.hpp>
#include <koilo/core/math/rotation.hpp>
#include <koilo/core/math/eulerangles.hpp>
#include <koilo/core/math/eulerconstants.hpp>

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <cctype>


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
	if (meshes.empty()) {
		meshes.reserve(1);
	}
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
    ++hierarchyGen_;
    return node;
}

SceneNode* koilo::Scene::Find(const std::string& name) {
    auto it = nodesByName_.find(name);
    return (it != nodesByName_.end()) ? it->second : nullptr;
}

std::size_t koilo::Scene::GetNodeCount() const {
    return ownedNodes_.size();
}

std::vector<SceneNode*> koilo::Scene::GetRootNodes() const {
    std::vector<SceneNode*> roots;
    roots.reserve(ownedNodes_.size());
    for (const auto& up : ownedNodes_) {
        if (up && up->GetParent() == nullptr) {
            roots.push_back(up.get());
        }
    }
    return roots;
}

SceneNode* koilo::Scene::PickNode(const Ray& worldRay, float maxDistance) {
    SceneNode* best        = nullptr;
    float      bestDist    = maxDistance;

    for (const auto& up : ownedNodes_) {
        SceneNode* node = up.get();
        if (!node) continue;
        Mesh* mesh = node->GetMesh();
        if (!mesh) continue;

        // Transform the world-space ray into the node's local space so
        // MeshRaycast (which tests raw vertex positions) sees a ray
        // consistent with how the renderer places the mesh.
        const Transform& world = node->GetWorldTransform();
        Vector3D nodePos = world.GetPosition();
        Quaternion nodeRot = world.GetRotation();
        Quaternion invRot = nodeRot.Conjugate();

        Vector3D localOrigin = invRot.RotateVector(worldRay.origin - nodePos);
        Vector3D localDir    = invRot.RotateVector(worldRay.direction);
        Ray      localRay(localOrigin, localDir);

        RaycastHit hit;
        // Editor picking is tolerant: no backface culling so the user
        // can click any visible-looking face.
        if (MeshRaycast::Raycast(localRay, mesh, hit, bestDist,
                                  /*backfaceCulling=*/false)) {
            if (hit.distance < bestDist) {
                bestDist = hit.distance;
                best     = node;
            }
        }
    }

    return best;
}

} // namespace koilo
// ============================================================================
// .kscene serializer (v1: SceneNodes only - see scene.hpp doc comment).
// ============================================================================

namespace {

// Sanitize a node name into a valid KoiloScript identifier suffix.
// Replaces non-alnum with '_', prefixes a digit-leading name with 'n'.
std::string SanitizeIdent(const std::string& name) {
    std::string out;
    out.reserve(name.size());
    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
            out.push_back(c);
        else
            out.push_back('_');
    }
    if (out.empty()) out = "node";
    if (std::isdigit(static_cast<unsigned char>(out[0]))) out.insert(out.begin(), 'n');
    return out;
}

void EmitVec3(std::ostringstream& os, const koilo::Vector3D& v) {
    os << "Vector3D(" << v.X << ", " << v.Y << ", " << v.Z << ")";
}

// Escape a string for inclusion in a KoiloScript double-quoted literal.
std::string EscapeStr(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (char c : s) {
        if (c == '\\' || c == '"') { out.push_back('\\'); out.push_back(c); }
        else if (c == '\n')        { out.append("\\n"); }
        else                        { out.push_back(c); }
    }
    return out;
}

} // namespace

bool koilo::Scene::SaveToKScene(const std::string& path) const {
    std::ostringstream os;
    os << "// Auto-generated by Scene::SaveToKScene.\n"
       << "// Declarative subset of KoiloScript -- see docs/kscene-format.md.\n"
       << "// v1: emits SceneNode hierarchy + local transforms only.\n\n";

    // Build a stable, parent-before-child traversal so SetParent calls
    // always reference an already-declared variable. ownedNodes_ is in
    // creation order which is usually fine, but we play it safe with a
    // topological sort over the node->parent edges.
    std::vector<SceneNode*> ordered;
    ordered.reserve(ownedNodes_.size());
    std::unordered_set<SceneNode*> emitted;
    emitted.reserve(ownedNodes_.size() * 2);

    std::function<void(SceneNode*)> visit = [&](SceneNode* n) {
        if (!n || emitted.count(n)) return;
        if (auto* p = n->GetParent()) visit(p);
        emitted.insert(n);
        ordered.push_back(n);
    };
    for (auto& up : ownedNodes_) visit(up.get());

    // Generate a unique identifier per node (handles duplicate names).
    std::unordered_map<SceneNode*, std::string> idents;
    std::unordered_set<std::string> usedIdents;
    for (auto* n : ordered) {
        std::string base = "node_" + SanitizeIdent(n->GetName());
        std::string ident = base;
        int suffix = 1;
        while (usedIdents.count(ident)) {
            ident = base + "_" + std::to_string(suffix++);
        }
        usedIdents.insert(ident);
        idents[n] = ident;
    }

    // Pass 1: declare nodes + write transforms.
    for (auto* n : ordered) {
        const std::string& id = idents[n];
        os << "var " << id << " = SceneNode(\""
           << EscapeStr(n->GetName()) << "\");\n";

        Transform& t = n->GetLocalTransform();
        Vector3D pos = t.GetPosition();
        Vector3D scl = t.GetScale();
        // Convert quaternion back to XYZS Euler degrees for round-trip
        // with SceneNode::SetRotation(Vector3D).
        Quaternion q = t.GetRotation();
        Rotation rot(q);
        Vector3D euler = rot.GetEulerAngles(EulerConstants::EulerOrderXYZS).Angles;

        os << id << ".SetPosition("; EmitVec3(os, pos); os << ");\n";
        os << id << ".SetRotation("; EmitVec3(os, euler); os << ");\n";
        os << id << ".SetScale(";    EmitVec3(os, scl);   os << ");\n";

        const std::string& sp = n->GetScriptPath();
        if (!sp.empty()) {
            os << id << ".SetScriptPath(\"" << EscapeStr(sp) << "\");\n";
        }
        os << "\n";
    }

    // Pass 2: parent links.
    for (auto* n : ordered) {
        SceneNode* p = n->GetParent();
        if (!p) continue;
        auto pit = idents.find(p);
        if (pit == idents.end()) continue;  // parent not owned by Scene
        os << idents[n] << ".SetParent(" << pit->second << ");\n";
    }

    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    const std::string out = os.str();
    f.write(out.data(), static_cast<std::streamsize>(out.size()));
    return f.good();
}
