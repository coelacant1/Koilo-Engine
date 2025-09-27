#include <ptx/systems/render/raster/rasterizer.hpp>

RGBColor Rasterizer::RasterizePixel(RasterTriangle2D** candidate_triangles,
                                    unsigned short count,
                                    const Vector2D& pixel_coord) {
    float closest_z = std::numeric_limits<float>::max();
    const RasterTriangle2D* hit_triangle = nullptr;
    float hit_u = 0.0f, hit_v = 0.0f, hit_w = 0.0f;

    // Find the closest triangle that covers this pixel
    for (unsigned short i = 0; i < count; ++i) {
        RasterTriangle2D* tri = candidate_triangles[i];
        if (tri->averageDepth < closest_z) {
            float u, v, w;
            if (tri->GetBarycentricCoords(pixel_coord.X, pixel_coord.Y, u, v, w)) {
                closest_z   = tri->averageDepth;
                hit_triangle = tri;
                hit_u = u; hit_v = v; hit_w = w;
            }
        }
    }

    if (!hit_triangle) {
        return RGBColor(0, 0, 0);
    }

    // Interpolate position (and UV if present)
    const Vector3D intersect_pos =
        (*hit_triangle->t3p1 * hit_u) +
        (*hit_triangle->t3p2 * hit_v) +
        (*hit_triangle->t3p3 * hit_w);

    Vector2D uv_coords(0.0f, 0.0f);
    if (hit_triangle->hasUV) {
        uv_coords =
            (*hit_triangle->p1UV * hit_u) +
            (*hit_triangle->p2UV * hit_v) +
            (*hit_triangle->p3UV * hit_w);
    }

    const Vector3D uv = Vector3D(uv_coords.X, uv_coords.Y, 0.0f);

    const SurfaceProperties surf = SurfaceProperties(intersect_pos, *(hit_triangle->normal), uv);

    // Shade: pass BOTH the shader and the material (const-ref for the second arg)
    IMaterial& mat = *hit_triangle->material;
    return mat.GetShader()->Shade(surf, mat);
}

void Rasterizer::Rasterize(Scene* scene, CameraBase* camera) {
    if (!scene || !camera || camera->Is2D()) return;

    // --- Setup ---
    camera->GetTransform()->SetBaseRotation(camera->GetCameraLayout()->GetRotation());
    Quaternion lookDirection = camera->GetTransform()->GetRotation().Multiply(camera->GetLookOffset());

    Vector2D minCoord = camera->GetCameraMinCoordinate();
    Vector2D maxCoord = camera->GetCameraMaxCoordinate();
    QuadTree<RasterTriangle2D> tree(Rectangle2D(minCoord, maxCoord));

    // 1) Count triangles
    uint32_t totalTriangles = 0;
    for (uint8_t i = 0; i < scene->GetMeshCount(); ++i) {
        Mesh* mesh = scene->GetMeshes()[i];
        if (mesh && mesh->IsEnabled() && mesh->GetTriangleGroup()) {
            totalTriangles += mesh->GetTriangleGroup()->GetTriangleCount();
        }
    }
    if (totalTriangles == 0) return;

    // 2) Project all to 2D
    RasterTriangle2D* projectedTriangles = new RasterTriangle2D[totalTriangles];
    uint32_t tri_idx = 0;

    for (uint8_t i = 0; i < scene->GetMeshCount(); ++i) {
        Mesh* mesh = scene->GetMeshes()[i];
        if (!mesh || !mesh->IsEnabled()) continue;

        ITriangleGroup* triangleGroup = mesh->GetTriangleGroup();
        if (!triangleGroup) continue;

        for (uint16_t j = 0; j < triangleGroup->GetTriangleCount(); ++j) {
            const Triangle3D& src = triangleGroup->GetTriangles()[j];

            RasterTriangle3D rtri = mesh->HasUV()
                ? RasterTriangle3D(&src.p1, &src.p2, &src.p3,
                                   &mesh->GetUVVertices()[mesh->GetUVIndexGroup()[j].A],
                                   &mesh->GetUVVertices()[mesh->GetUVIndexGroup()[j].B],
                                   &mesh->GetUVVertices()[mesh->GetUVIndexGroup()[j].C])
                : RasterTriangle3D(&src.p1, &src.p2, &src.p3);

            projectedTriangles[tri_idx] =
                RasterTriangle2D(*camera->GetTransform(), lookDirection, rtri, mesh->GetMaterial());
            ++tri_idx;
        }
    }

    // 3) Build acceleration
    for (uint32_t i = 0; i < totalTriangles; ++i) {
        tree.Insert(&projectedTriangles[i]);
    }

    // 4) Shade per pixel
    IPixelGroup* pixelGroup = camera->GetPixelGroup();
    for (uint16_t i = 0; i < pixelGroup->GetPixelCount(); ++i) {
        Vector2D p = pixelGroup->GetCoordinate(i);
        auto* leaf = tree.GetRoot()->FindLeaf(p);

        RGBColor out(0,0,0);
        if (leaf && leaf->GetItemCount() > 0) {
            out = RasterizePixel(leaf->GetItems(), leaf->GetItemCount(), p);
        }
        *pixelGroup->GetColor(i) = out;
    }

    // 5) Cleanup
    delete[] projectedTriangles;
}
