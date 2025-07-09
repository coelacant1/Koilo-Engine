#include "rasterizer.hpp"


RGBColor Rasterizer::RasterizePixel(RasterTriangle2D** candidate_triangles, unsigned short count, const Vector2D& pixel_coord) {
    float closest_z = std::numeric_limits<float>::max();
    const RasterTriangle2D* hit_triangle = nullptr;
    float hit_u = 0.0f, hit_v = 0.0f, hit_w = 0.0f;

    // Find the closest triangle that intersects the pixel
    for (unsigned short i = 0; i < count; ++i) {
        RasterTriangle2D* tri_ptr = candidate_triangles[i];
        // Z-buffer check
        if (tri_ptr->averageDepth < closest_z) {
            float u, v, w;
            if (tri_ptr->GetBarycentricCoords(pixel_coord.X, pixel_coord.Y, u, v, w)) {
                // Intersection found, update the hit data
                closest_z = tri_ptr->averageDepth;
                hit_triangle = tri_ptr;
                hit_u = u;
                hit_v = v;
                hit_w = w;
            }
        }
    }

    // If a triangle was hit, calculate its color
    if (hit_triangle) {
        Vector3D intersect_pos = (*hit_triangle->t3p1 * hit_u) + (*hit_triangle->t3p2 * hit_v) + (*hit_triangle->t3p3 * hit_w);
        Vector2D uv_coords;

        if (hit_triangle->hasUV) {
            uv_coords = (*hit_triangle->p1UV * hit_u) + (*hit_triangle->p2UV * hit_v) + (*hit_triangle->p3UV * hit_w);
        }

        return hit_triangle->material->GetRGB(intersect_pos, hit_triangle->normal, Vector3D(uv_coords.X, uv_coords.Y, 0.0f));
    }

    return RGBColor(0, 0, 0); // No intersection, return black
}


void Rasterizer::Rasterize(Scene* scene, CameraBase* camera) {
    if (!scene || !camera || camera->Is2D()) {
        return;
    }

    // --- Setup ---
    camera->GetTransform()->SetBaseRotation(camera->GetCameraLayout()->GetRotation());
    Quaternion lookDirection = camera->GetTransform()->GetRotation().Multiply(camera->GetLookOffset());

    Vector2D minCoord = camera->GetCameraMinCoordinate();
    Vector2D maxCoord = camera->GetCameraMaxCoordinate();
    QuadTree<RasterTriangle2D> tree(Rectangle2D(minCoord, maxCoord));

    // 1. Calculate total number of triangles to allocate memory on the heap
    uint32_t totalTriangles = 0;
    for (uint8_t i = 0; i < scene->GetMeshCount(); ++i) {
        Mesh* mesh = scene->GetMeshes()[i];
        if (mesh && mesh->IsEnabled() && mesh->GetTriangleGroup()) {
            totalTriangles += mesh->GetTriangleGroup()->GetTriangleCount();
        }
    }

    if (totalTriangles == 0) {
        return; // No triangles to render
    }
    
    RasterTriangle2D* projectedTriangles = new RasterTriangle2D[totalTriangles];
    uint32_t tri_idx = 0; // Keep a running index for our heap array

    // 2. Project all visible triangles from 3D to 2D
    for (uint8_t i = 0; i < scene->GetMeshCount(); ++i) {
        Mesh* mesh = scene->GetMeshes()[i];
        if (mesh && mesh->IsEnabled()) {
            ITriangleGroup* triangleGroup = mesh->GetTriangleGroup();
            if(!triangleGroup) continue;

            if (mesh->HasUV()){

            }

            for (uint16_t j = 0; j < triangleGroup->GetTriangleCount(); ++j) {
                const Triangle3D& sourceTri = triangleGroup->GetTriangles()[j];

                const RasterTriangle3D rasterTri = mesh->HasUV() ? 
                    RasterTriangle3D(&sourceTri.p1, &sourceTri.p2, &sourceTri.p3, 
                        &mesh->GetUVVertices()[mesh->GetUVIndexGroup()[j].A], 
                        &mesh->GetUVVertices()[mesh->GetUVIndexGroup()[j].B], 
                        &mesh->GetUVVertices()[mesh->GetUVIndexGroup()[j].C]) :
                    RasterTriangle3D(&sourceTri.p1, &sourceTri.p2, &sourceTri.p3);

                // Construct the projected triangle directly in our heap-allocated array
                projectedTriangles[tri_idx] = RasterTriangle2D(*camera->GetTransform(), lookDirection, rasterTri, mesh->GetMaterial());
                tri_idx++;
            }
        }
    }

    // 3. Insert pointers to all projected 2D triangles into the QuadTree
    for (uint32_t i = 0; i < totalTriangles; ++i) {
        tree.Insert(&projectedTriangles[i]);
    }
    
    // 4. Rasterize each pixel
    IPixelGroup* pixelGroup = camera->GetPixelGroup();
    for (uint16_t i = 0; i < pixelGroup->GetPixelCount(); ++i) {
        Vector2D pixel_coord = pixelGroup->GetCoordinate(i);
        QuadTree<RasterTriangle2D>::Node* leafNode = tree.GetRoot()->FindLeaf(pixel_coord);

        RGBColor final_color(0,0,0);

        if (leafNode && leafNode->GetItemCount() > 0) {
            // Directly pass the raw array and count from the node to the rasterizer function
            final_color = RasterizePixel(leafNode->GetItems(), leafNode->GetItemCount(), pixel_coord);
        }

        *pixelGroup->GetColor(i) = final_color;
    }

    // 5. IMPORTANT: Clean up the memory allocated on the heap
    delete[] projectedTriangles;
}