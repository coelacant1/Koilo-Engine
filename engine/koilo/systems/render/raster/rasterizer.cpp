// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/render/raster/rasterizer.hpp>
#include <koilo/systems/render/sky/sky.hpp>
#include <koilo/systems/render/material/implementations/kslmaterial.hpp>
#include <koilo/core/geometry/2d/shape.hpp>
#include <koilo/systems/profiling/performanceprofiler.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>


namespace koilo {

namespace {

static Vector3D s_cameraPos(0, 0, 0);

// Quake III fast inverse square root (one Newton-Raphson iteration)
static inline float FastInvSqrt(float x) {
    float xhalf = 0.5f * x;
    int32_t i;
    std::memcpy(&i, &x, sizeof(i));
    i = 0x5f3759df - (i >> 1);
    std::memcpy(&x, &i, sizeof(x));
    x *= (1.5f - xhalf * x * x);  // Newton-Raphson refinement
    return x;
}

}  // namespace

// Static member definitions
bool Rasterizer::s_debugEnabled = false;
std::vector<float> Rasterizer::s_depthBuf;
std::vector<float> Rasterizer::s_normalBuf;
int Rasterizer::s_bufW = 0;
int Rasterizer::s_bufH = 0;

// Saved projection state for DrawLine3D
bool     Rasterizer::s_rsPerspective = false;
float    Rasterizer::s_rsFovScale    = 1.0f;
float    Rasterizer::s_rsNearPlane   = 0.1f;
float    Rasterizer::s_rsCX          = 0.0f;
float    Rasterizer::s_rsCY          = 0.0f;
float    Rasterizer::s_rsHH          = 1.0f;
Quaternion Rasterizer::s_rsInvRot;
Vector3D Rasterizer::s_rsCamPos;
Vector3D Rasterizer::s_rsCamScl(1, 1, 1);

void koilo::Rasterizer::Rasterize(Scene* scene, CameraBase* camera) {
    KL_PERF_SCOPE("SW.Total");
    if (!scene || !camera || camera->Is2D()) return;

    // --- Setup ---
    camera->GetTransform()->SetBaseRotation(camera->GetCameraLayout()->GetRotation());
    Quaternion lookDirection = camera->GetTransform()->GetRotation().Multiply(camera->GetLookOffset());

    Vector2D minCoord = camera->GetCameraMinCoordinate();
    Vector2D maxCoord = camera->GetCameraMaxCoordinate();

    // Projection settings
    bool perspective = camera->IsPerspective();
    float nearPlane = camera->GetNearPlane();
    float fovScale = 0.0f;
    float viewportCenterX = (minCoord.X + maxCoord.X) * 0.5f;
    float viewportCenterY = (minCoord.Y + maxCoord.Y) * 0.5f;
    float viewportHalfW = (maxCoord.X - minCoord.X) * 0.5f;
    float viewportHalfH = (maxCoord.Y - minCoord.Y) * 0.5f;
    if (perspective) {
        float fovRad = camera->GetFOV() * 3.14159265f / 180.0f;
        fovScale = 1.0f / std::tan(fovRad * 0.5f);
    }
    bool cullBackfaces = camera->GetBackfaceCulling();
    s_cameraPos = camera->GetTransform()->GetPosition();

    // Viewport dimensions (needed early for pixel AABB computation during projection)
    int vpW = static_cast<int>(maxCoord.X - minCoord.X + 1);
    int vpH = static_cast<int>(maxCoord.Y - minCoord.Y + 1);
    float stepX = (vpW > 1) ? (maxCoord.X - minCoord.X) / static_cast<float>(vpW - 1) : 1.0f;
    float stepY = (vpH > 1) ? (maxCoord.Y - minCoord.Y) / static_cast<float>(vpH - 1) : 1.0f;
    float invStepX = 1.0f / stepX;
    float invStepY = 1.0f / stepY;

    // Save projection state for DrawLine3D
    s_rsPerspective = perspective;
    s_rsFovScale    = fovScale;
    s_rsNearPlane   = nearPlane;
    s_rsCX          = viewportCenterX;
    s_rsCY          = viewportCenterY;
    s_rsHH          = viewportHalfH;
    s_rsInvRot      = lookDirection.Conjugate();
    s_rsCamPos      = s_cameraPos;
    s_rsCamScl      = camera->GetTransform()->GetScale();

    // 1) Count triangles
    uint32_t totalTriangles = 0;
    auto& _perf = koilo::PerformanceProfiler::GetInstance();
    _perf.BeginSample("SW.Project");
    for (unsigned int i = 0; i < scene->GetMeshCount(); ++i) {
        Mesh* mesh = scene->GetMeshes()[i];
        if (mesh && mesh->IsEnabled() && mesh->GetTriangleGroup()) {
            totalTriangles += mesh->GetTriangleGroup()->GetTriangleCount();
        }
    }
    if (totalTriangles == 0) { _perf.EndSample("SW.Project"); return; }

    // 2) Project all triangles to 2D (reuse persistent buffer to avoid heap alloc per frame)
    static std::vector<RasterTriangle2D> projectedTriangles;
    projectedTriangles.clear();
    projectedTriangles.reserve(totalTriangles);

    // Near-plane clipping: precompute camera-space Z transform
    Quaternion clipInvRot = lookDirection.Conjugate();
    float cqw = clipInvRot.W, cqx = clipInvRot.X, cqy = clipInvRot.Y, cqz = clipInvRot.Z;
    Vector3D clipCamPos = s_cameraPos;
    Vector3D clipCamScl = camera->GetTransform()->GetScale();
    float clipInvSz = (clipCamScl.Z != 0.0f) ? 1.0f / clipCamScl.Z : 0.0f;

    auto getCamZ = [&](const Vector3D& w) -> float {
        float rx = w.X - clipCamPos.X;
        float ry = w.Y - clipCamPos.Y;
        float rz = w.Z - clipCamPos.Z;
        float tx = 2.0f * (cqy * rz - cqz * ry);
        float ty = 2.0f * (cqz * rx - cqx * rz);
        float tz = 2.0f * (cqx * ry - cqy * rx);
        float pz = (rz + cqw * tz + (cqx * ty - cqy * tx)) * clipInvSz;
        return -pz;
    };

    // Helper to emit a projected triangle and set up its rasterizer state
    auto emitTriangle = [&](const Vector3D& v1, const Vector3D& v2, const Vector3D& v3,
                            const Vector2D* t1, const Vector2D* t2, const Vector2D* t3,
                            IMaterial* mat, const Vector3D& normal,
                            ksl::KSLShadeFn shadeFn, void* shaderInst,
                            const ksl::FrameContext* ctx, uint8_t attribs) {
        // Use stack-allocated Vector3D for clipped vertices
        const Vector3D* pv1 = &v1;
        const Vector3D* pv2 = &v2;
        const Vector3D* pv3 = &v3;
        projectedTriangles.emplace_back(*camera->GetTransform(), lookDirection,
                                        pv1, pv2, pv3, t1, t2, t3, mat,
                                        nearPlane, fovScale, viewportCenterX, viewportCenterY,
                                        viewportHalfW, viewportHalfH);

        if (cullBackfaces && projectedTriangles.back().IsBackFacing()) {
            projectedTriangles.pop_back();
            return;
        }
        auto& pt = projectedTriangles.back();
        pt.normal = normal;
        pt.shadeFn = shadeFn;
        pt.shaderInstance = shaderInst;
        pt.frameCtx = ctx;
        pt.attribMask = attribs;
        pt.pixMinX = static_cast<int16_t>(std::max(0, static_cast<int>(std::ceil((pt.boundsMinX - minCoord.X) * invStepX))));
        pt.pixMaxX = static_cast<int16_t>(std::min(vpW - 1, static_cast<int>(std::floor((pt.boundsMaxX - minCoord.X) * invStepX))));
        pt.pixMinY = static_cast<int16_t>(std::max(0, static_cast<int>(std::ceil((pt.boundsMinY - minCoord.Y) * invStepY))));
        pt.pixMaxY = static_cast<int16_t>(std::min(vpH - 1, static_cast<int>(std::floor((pt.boundsMaxY - minCoord.Y) * invStepY))));
    };

    for (unsigned int i = 0; i < scene->GetMeshCount(); ++i) {
        Mesh* mesh = scene->GetMeshes()[i];
        if (!mesh || !mesh->IsEnabled()) continue;

        ITriangleGroup* triangleGroup = mesh->GetTriangleGroup();
        if (!triangleGroup) continue;

        // Extract shade binding once per mesh (all triangles share same material)
        ksl::KSLShadeFn meshShadeFn = nullptr;
        void* meshInstance = nullptr;
        const ksl::FrameContext* meshCtx = nullptr;
        uint8_t meshAttribs = ksl::SHADE_ATTRIB_ALL;
        IMaterial* mat = mesh->GetMaterial();
        if (mat && mat->GetShader()) {
            auto* bridge = static_cast<KSLShaderBridge*>(
                const_cast<IShader*>(mat->GetShader()));
            bridge->EnsureReady();
            meshShadeFn = bridge->GetShadeFnDirect();
            meshInstance = bridge->GetShaderInstance();
            meshCtx = bridge->GetFrameContext();
            meshAttribs = bridge->GetRequiredAttribs();
        }

        for (uint32_t j = 0; j < triangleGroup->GetTriangleCount(); ++j) {
            const Triangle3D& src = triangleGroup->GetTriangles()[j];

            const Vector2D* t1 = nullptr;
            const Vector2D* t2 = nullptr;
            const Vector2D* t3 = nullptr;
            if (mesh->HasUV()) {
                const auto& uvIdx = mesh->GetUVIndexGroup()[j];
                t1 = &mesh->GetUVVertices()[uvIdx.A];
                t2 = &mesh->GetUVVertices()[uvIdx.B];
                t3 = &mesh->GetUVVertices()[uvIdx.C];
            }

            if (!perspective) {
                projectedTriangles.emplace_back(*camera->GetTransform(), lookDirection,
                                                src.p1, src.p2, src.p3, t1, t2, t3, mat);
                if (cullBackfaces && projectedTriangles.back().IsBackFacing()) {
                    projectedTriangles.pop_back();
                } else {
                    auto& pt = projectedTriangles.back();
                    pt.shadeFn = meshShadeFn;
                    pt.shaderInstance = meshInstance;
                    pt.frameCtx = meshCtx;
                    pt.attribMask = meshAttribs;
                    pt.pixMinX = static_cast<int16_t>(std::max(0, static_cast<int>(std::ceil((pt.boundsMinX - minCoord.X) * invStepX))));
                    pt.pixMaxX = static_cast<int16_t>(std::min(vpW - 1, static_cast<int>(std::floor((pt.boundsMaxX - minCoord.X) * invStepX))));
                    pt.pixMinY = static_cast<int16_t>(std::max(0, static_cast<int>(std::ceil((pt.boundsMinY - minCoord.Y) * invStepY))));
                    pt.pixMaxY = static_cast<int16_t>(std::min(vpH - 1, static_cast<int>(std::floor((pt.boundsMaxY - minCoord.Y) * invStepY))));
                }
                continue;
            }

            // Perspective: near-plane clipping
            const Vector3D& wp1 = *src.p1;
            const Vector3D& wp2 = *src.p2;
            const Vector3D& wp3 = *src.p3;
            float cz1 = getCamZ(wp1);
            float cz2 = getCamZ(wp2);
            float cz3 = getCamZ(wp3);
            bool b1 = cz1 <= nearPlane, b2 = cz2 <= nearPlane, b3 = cz3 <= nearPlane;
            int behind = b1 + b2 + b3;

            if (behind == 3) continue; // fully behind camera

            Vector3D triNormal = src.GetNormal();

            if (behind == 0) {
                // No clipping needed
                emitTriangle(wp1, wp2, wp3, t1, t2, t3, mat, triNormal,
                             meshShadeFn, meshInstance, meshCtx, meshAttribs);
            } else {
                // Near-plane clipping required
                // Identify which vertices are in front (F) and behind (B)
                // Lerp factor: t = (nearPlane - czB) / (czF - czB)
                static Vector2D clipUVs[4]; // scratch for interpolated UVs
                static Vector3D clipVerts[4]; // scratch for interpolated positions

                auto lerpV3 = [](const Vector3D& a, const Vector3D& b, float t) -> Vector3D {
                    return Vector3D(a.X + (b.X - a.X) * t, a.Y + (b.Y - a.Y) * t, a.Z + (b.Z - a.Z) * t);
                };
                auto lerpV2 = [](const Vector2D& a, const Vector2D& b, float t) -> Vector2D {
                    return Vector2D(a.X + (b.X - a.X) * t, a.Y + (b.Y - a.Y) * t);
                };
                static const Vector2D zeroUV(0, 0);
                const Vector2D& uv1r = t1 ? *t1 : zeroUV;
                const Vector2D& uv2r = t2 ? *t2 : zeroUV;
                const Vector2D& uv3r = t3 ? *t3 : zeroUV;

                if (behind == 1) {
                    // 1 vertex behind -> clip to quad (2 triangles)
                    // Reorder so vA is behind, vB and vC are in front
                    const Vector3D *vA, *vB, *vC;
                    const Vector2D *uvA, *uvB, *uvC;
                    float czA, czB, czC;
                    if (b1) {
                        vA = &wp1; vB = &wp2; vC = &wp3;
                        uvA = &uv1r; uvB = &uv2r; uvC = &uv3r;
                        czA = cz1; czB = cz2; czC = cz3;
                    } else if (b2) {
                        vA = &wp2; vB = &wp3; vC = &wp1;
                        uvA = &uv2r; uvB = &uv3r; uvC = &uv1r;
                        czA = cz2; czB = cz3; czC = cz1;
                    } else {
                        vA = &wp3; vB = &wp1; vC = &wp2;
                        uvA = &uv3r; uvB = &uv1r; uvC = &uv2r;
                        czA = cz3; czB = cz1; czC = cz2;
                    }
                    float tAB = (nearPlane - czA) / (czB - czA);
                    float tAC = (nearPlane - czA) / (czC - czA);
                    clipVerts[0] = lerpV3(*vA, *vB, tAB); // intersection on edge AB
                    clipVerts[1] = lerpV3(*vA, *vC, tAC); // intersection on edge AC
                    clipUVs[0] = lerpV2(*uvA, *uvB, tAB);
                    clipUVs[1] = lerpV2(*uvA, *uvC, tAC);

                    // Triangle 1: clipAB, B, C
                    emitTriangle(clipVerts[0], *vB, *vC, &clipUVs[0], uvB, uvC, mat, triNormal,
                                 meshShadeFn, meshInstance, meshCtx, meshAttribs);
                    // Triangle 2: clipAB, C, clipAC
                    emitTriangle(clipVerts[0], *vC, clipVerts[1], &clipUVs[0], uvC, &clipUVs[1], mat, triNormal,
                                 meshShadeFn, meshInstance, meshCtx, meshAttribs);
                } else {
                    // 2 vertices behind -> clip to single triangle
                    // Reorder so vC is in front, vA and vB are behind
                    const Vector3D *vA, *vB, *vC;
                    const Vector2D *uvA, *uvB, *uvC;
                    float czA, czB, czC;
                    if (!b1) {
                        vC = &wp1; vA = &wp2; vB = &wp3;
                        uvC = &uv1r; uvA = &uv2r; uvB = &uv3r;
                        czC = cz1; czA = cz2; czB = cz3;
                    } else if (!b2) {
                        vC = &wp2; vA = &wp3; vB = &wp1;
                        uvC = &uv2r; uvA = &uv3r; uvB = &uv1r;
                        czC = cz2; czA = cz3; czB = cz1;
                    } else {
                        vC = &wp3; vA = &wp1; vB = &wp2;
                        uvC = &uv3r; uvA = &uv1r; uvB = &uv2r;
                        czC = cz3; czA = cz1; czB = cz2;
                    }
                    float tAC = (nearPlane - czA) / (czC - czA);
                    float tBC = (nearPlane - czB) / (czC - czB);
                    clipVerts[0] = lerpV3(*vA, *vC, tAC); // intersection on edge AC
                    clipVerts[1] = lerpV3(*vB, *vC, tBC); // intersection on edge BC
                    clipUVs[0] = lerpV2(*uvA, *uvC, tAC);
                    clipUVs[1] = lerpV2(*uvB, *uvC, tBC);

                    // Single clipped triangle: clipAC, clipBC, C
                    emitTriangle(clipVerts[0], clipVerts[1], *vC, &clipUVs[0], &clipUVs[1], uvC, mat, triNormal,
                                 meshShadeFn, meshInstance, meshCtx, meshAttribs);
                }
            }
        }
    }

    if (projectedTriangles.empty()) { _perf.EndSample("SW.Project"); return; }

    // 3) Sort: primary by shader fn ptr (I-cache coherency), secondary front-to-back (early z-reject)
    std::sort(projectedTriangles.begin(), projectedTriangles.end(),
              [](const RasterTriangle2D& a, const RasterTriangle2D& b) {
                  if (a.shadeFn != b.shadeFn)
                      return reinterpret_cast<uintptr_t>(a.shadeFn) < reinterpret_cast<uintptr_t>(b.shadeFn);
                  return a.averageDepth < b.averageDepth;
              });
    _perf.EndSample("SW.Project");

    // 4) Setup buffers
    IPixelGroup* pixelGroup = camera->GetPixelGroup();
    Color888* colorBuf = pixelGroup->GetColors();

    int totalPixels = vpW * vpH;

    // Initialize depth buffer
    s_bufW = vpW;
    s_bufH = vpH;
    s_depthBuf.resize(totalPixels);
    std::fill(s_depthBuf.begin(), s_depthBuf.end(), std::numeric_limits<float>::max());

    if (s_debugEnabled) {
        s_normalBuf.resize(totalPixels * 3);
        std::memset(s_normalBuf.data(), 0, s_normalBuf.size() * sizeof(float));
    }

    // 5) Fill sky background
    _perf.BeginSample("SW.Sky");
    Sky& sky = Sky::GetInstance();
    bool hasSky = sky.IsEnabled() || camera->HasSkyGradient();

    if (hasSky && vpH > 1) {
        // Sample sky top/bottom once, integer-lerp per row
        KSLMaterial* skyMat = sky.GetMaterial();
        if (skyMat && skyMat->IsBound()) {
            const IShader* skyShader = skyMat->GetShader();
            Vector3D skyPos(0, 0, 0);
            Vector3D skyNrm(0, 0, 1);
            Vector3D uvTop(0.5f, 0.0f, 0);
            Vector3D uvBot(0.5f, 1.0f, 0);
            SurfaceProperties spTop(skyPos, skyNrm, uvTop);
            SurfaceProperties spBot(skyPos, skyNrm, uvBot);
            Color888 topColor = skyShader->Shade(spTop, *skyMat);
            Color888 botColor = skyShader->Shade(spBot, *skyMat);
            int hm1 = vpH - 1;
            int rowBytes = vpW * 3;
            for (int y = 0; y < vpH; ++y) {
                int t256 = (y * 256) / hm1;
                uint8_t cr = static_cast<uint8_t>(topColor.R + ((botColor.R - topColor.R) * t256 >> 8));
                uint8_t cg = static_cast<uint8_t>(topColor.G + ((botColor.G - topColor.G) * t256 >> 8));
                uint8_t cb = static_cast<uint8_t>(topColor.B + ((botColor.B - topColor.B) * t256 >> 8));
                // Fill row: write a small seed, then double with memcpy
                uint8_t* dst = reinterpret_cast<uint8_t*>(&colorBuf[y * vpW]);
                dst[0] = cr; dst[1] = cg; dst[2] = cb;
                int filled = 3;
                while (filled * 2 <= rowBytes) {
                    std::memcpy(dst + filled, dst, filled);
                    filled *= 2;
                }
                if (filled < rowBytes) {
                    std::memcpy(dst + filled, dst, rowBytes - filled);
                }
            }
        }
    } else {
        std::memset(static_cast<void*>(colorBuf), 0, totalPixels * sizeof(Color888));
    }
    _perf.EndSample("SW.Sky");

    // 6) Scanline rasterize each triangle with edge walking + incremental interpolation
    _perf.BeginSample("SW.Triangles");
    for (const auto& tri : projectedTriangles) {
        if (tri.denominator == 0.0f || !tri.shadeFn) continue;

        // Use precomputed pixel-space AABB directly
        int xMin = tri.pixMinX;
        int xMax = tri.pixMaxX;
        int yMin = tri.pixMinY;
        int yMax = tri.pixMaxY;

        if (yMin > yMax || xMin > xMax) continue;

        // Set FrameContext and normal once per triangle
        ksl::ShadeInput shadeInput;
        shadeInput.ctx = tri.frameCtx;
        uint8_t amask = tri.attribMask;
        bool needPos  = (amask & ksl::SHADE_ATTRIB_POS) || (amask & ksl::SHADE_ATTRIB_VIEWDIR);
        bool needUV   = (amask & ksl::SHADE_ATTRIB_UV) != 0;
        bool needView = (amask & ksl::SHADE_ATTRIB_VIEWDIR) != 0;
        if (amask & ksl::SHADE_ATTRIB_NORMAL)
            shadeInput.normal = { tri.normal.X, tri.normal.Y, tri.normal.Z };

        // Incremental barycentric deltas per pixel step
        float dv_dx = tri.v1.Y * tri.denominator * stepX;
        float dw_dx = -tri.v0.Y * tri.denominator * stepX;
        float dv_dy = -tri.v1.X * tri.denominator * stepY;
        float dw_dy = tri.v0.X * tri.denominator * stepY;
        float dvw_dx = dv_dx + dw_dx;

        // Depth deltas (always needed)
        float dz12 = tri.z2 - tri.z1;
        float dz13 = tri.z3 - tri.z1;
        float dz_dx = dz12 * dv_dx + dz13 * dw_dx;
        float dz_dy = dz12 * dv_dy + dz13 * dw_dy;

        // Perspective-correct interpolation: pre-divide attributes by camera-space Z
        // Near-plane clipping guarantees all vertices are in front of camera (z > nearPlane)
        bool needPC = needPos || needUV;
        float invW1 = 0, invW2 = 0, invW3 = 0;
        float dinvW12 = 0, dinvW13 = 0;
        float dinvW_dx = 0, dinvW_dy = 0;
        if (needPC) {
            invW1 = 1.0f / tri.z1; invW2 = 1.0f / tri.z2; invW3 = 1.0f / tri.z3;
            dinvW12 = invW2 - invW1; dinvW13 = invW3 - invW1;
            dinvW_dx = dinvW12 * dv_dx + dinvW13 * dw_dx;
            dinvW_dy = dinvW12 * dv_dy + dinvW13 * dw_dy;
        }

        // Edge differences for pos/z and uv/z (perspective-correct)
        float ewpx12 = 0, ewpx13 = 0, ewpy12 = 0, ewpy13 = 0, ewpz12 = 0, ewpz13 = 0;
        float dpx_dx = 0, dpy_dx = 0, dpz_dx = 0, dpx_dy = 0, dpy_dy = 0, dpz_dy = 0;
        if (needPos) {
            float pcpx1 = tri.wp1.X * invW1, pcpx2 = tri.wp2.X * invW2, pcpx3 = tri.wp3.X * invW3;
            float pcpy1 = tri.wp1.Y * invW1, pcpy2 = tri.wp2.Y * invW2, pcpy3 = tri.wp3.Y * invW3;
            float pcpz1 = tri.wp1.Z * invW1, pcpz2 = tri.wp2.Z * invW2, pcpz3 = tri.wp3.Z * invW3;
            ewpx12 = pcpx2 - pcpx1; ewpx13 = pcpx3 - pcpx1;
            ewpy12 = pcpy2 - pcpy1; ewpy13 = pcpy3 - pcpy1;
            ewpz12 = pcpz2 - pcpz1; ewpz13 = pcpz3 - pcpz1;
            dpx_dx = ewpx12 * dv_dx + ewpx13 * dw_dx;
            dpy_dx = ewpy12 * dv_dx + ewpy13 * dw_dx;
            dpz_dx = ewpz12 * dv_dx + ewpz13 * dw_dx;
            dpx_dy = ewpx12 * dv_dy + ewpx13 * dw_dy;
            dpy_dy = ewpy12 * dv_dy + ewpy13 * dw_dy;
            dpz_dy = ewpz12 * dv_dy + ewpz13 * dw_dy;
        }

        float euvx12 = 0, euvx13 = 0, euvy12 = 0, euvy13 = 0;
        float duvx_dx = 0, duvy_dx = 0, duvx_dy = 0, duvy_dy = 0;
        if (needUV) {
            float pcux1 = tri.uv1.X * invW1, pcux2 = tri.uv2.X * invW2, pcux3 = tri.uv3.X * invW3;
            float pcuy1 = tri.uv1.Y * invW1, pcuy2 = tri.uv2.Y * invW2, pcuy3 = tri.uv3.Y * invW3;
            euvx12 = pcux2 - pcux1; euvx13 = pcux3 - pcux1;
            euvy12 = pcuy2 - pcuy1; euvy13 = pcuy3 - pcuy1;
            duvx_dx = euvx12 * dv_dx + euvx13 * dw_dx;
            duvy_dx = euvy12 * dv_dx + euvy13 * dw_dx;
            duvx_dy = euvx12 * dv_dy + euvx13 * dw_dy;
            duvy_dy = euvy12 * dv_dy + euvy13 * dw_dy;
        }

        // Barycentrics at starting pixel (xMin, yMin)
        float startVX = minCoord.X + xMin * stepX;
        float startVY = minCoord.Y + yMin * stepY;
        float v2x = startVX - tri.p1.X;
        float v2y = startVY - tri.p1.Y;
        float v_rowStart = (v2x * tri.v1.Y - tri.v1.X * v2y) * tri.denominator;
        float w_rowStart = (tri.v0.X * v2y - v2x * tri.v0.Y) * tri.denominator;
        float z_rowStart = tri.z1 + dz12 * v_rowStart + dz13 * w_rowStart;

        // Row-start values for 1/z and pos/z, uv/z (perspective-correct)
        float invW_rowStart = 0;
        if (needPC) invW_rowStart = invW1 + dinvW12 * v_rowStart + dinvW13 * w_rowStart;
        float px_rowStart = 0, py_rowStart = 0, pz_rowStart = 0;
        if (needPos) {
            px_rowStart = tri.wp1.X * invW1 + ewpx12 * v_rowStart + ewpx13 * w_rowStart;
            py_rowStart = tri.wp1.Y * invW1 + ewpy12 * v_rowStart + ewpy13 * w_rowStart;
            pz_rowStart = tri.wp1.Z * invW1 + ewpz12 * v_rowStart + ewpz13 * w_rowStart;
        }
        float uvx_rowStart = 0, uvy_rowStart = 0;
        if (needUV) {
            uvx_rowStart = tri.uv1.X * invW1 + euvx12 * v_rowStart + euvx13 * w_rowStart;
            uvy_rowStart = tri.uv1.Y * invW1 + euvy12 * v_rowStart + euvy13 * w_rowStart;
        }

        // Cache shade function and instance for inner loop
        auto shadeFn = tri.shadeFn;
        void* shaderInst = tri.shaderInstance;
        float xSpan = static_cast<float>(xMax - xMin);

        for (int y = yMin; y <= yMax; ++y) {
            // Edge walking: compute exact x_start/x_end from barycentric half-planes
            float xl = 0.0f;
            float xr = xSpan;
            bool rowValid = true;

            if (dv_dx > 0.0f) {
                float xb = -v_rowStart / dv_dx;
                if (xb > xl) xl = xb;
            } else if (dv_dx < 0.0f) {
                float xb = -v_rowStart / dv_dx;
                if (xb < xr) xr = xb;
            } else if (v_rowStart < 0.0f) {
                rowValid = false;
            }

            if (rowValid) {
                if (dw_dx > 0.0f) {
                    float xb = -w_rowStart / dw_dx;
                    if (xb > xl) xl = xb;
                } else if (dw_dx < 0.0f) {
                    float xb = -w_rowStart / dw_dx;
                    if (xb < xr) xr = xb;
                } else if (w_rowStart < 0.0f) {
                    rowValid = false;
                }
            }

            if (rowValid) {
                float u_row = 1.0f - v_rowStart - w_rowStart;
                if (dvw_dx > 0.0f) {
                    float xb = u_row / dvw_dx;
                    if (xb < xr) xr = xb;
                } else if (dvw_dx < 0.0f) {
                    float xb = u_row / dvw_dx;
                    if (xb > xl) xl = xb;
                } else if (u_row < 0.0f) {
                    rowValid = false;
                }
            }

            if (rowValid) {
                // Sub-pixel epsilon expansion to prevent gaps at shared triangle edges
                int x_start = xMin + static_cast<int>(std::ceil(xl - 1e-3f));
                int x_end   = xMin + static_cast<int>(std::floor(xr + 1e-3f));
                if (x_start < xMin) x_start = xMin;
                if (x_end > xMax) x_end = xMax;

                if (x_start <= x_end) {
                    int dxOff = x_start - xMin;
                    float z   = z_rowStart + dxOff * dz_dx;
                    float iw = 0;
                    if (needPC) iw = invW_rowStart + dxOff * dinvW_dx;
                    float px = 0, py = 0, pz = 0;
                    if (needPos) {
                        px = px_rowStart + dxOff * dpx_dx;
                        py = py_rowStart + dxOff * dpy_dx;
                        pz = pz_rowStart + dxOff * dpz_dx;
                    }
                    float uvx = 0, uvy = 0;
                    if (needUV) {
                        uvx = uvx_rowStart + dxOff * duvx_dx;
                        uvy = uvy_rowStart + dxOff * duvy_dx;
                    }
                    int rowOffset = y * vpW;

                    for (int x = x_start; x <= x_end; ++x) {
                        int idx = rowOffset + x;
                        if (z < s_depthBuf[idx]) {
                            s_depthBuf[idx] = z;

                            if (needPC) {
                                float w = 1.0f / iw;
                                if (needPos) shadeInput.position = { px * w, py * w, pz * w };
                                if (needUV)  shadeInput.uv       = { uvx * w, uvy * w };
                                if (needView) {
                                    float wpx = px * w, wpy = py * w, wpz = pz * w;
                                    shadeInput.viewDir = { wpx - s_cameraPos.X, wpy - s_cameraPos.Y, wpz - s_cameraPos.Z };
                                }
                            }

                            ksl::vec4 c = shadeFn(shaderInst, &shadeInput);
                            colorBuf[idx] = Color888(
                                c.x <= 0.0f ? 0 : c.x >= 1.0f ? 255 : static_cast<uint8_t>(c.x * 255.0f),
                                c.y <= 0.0f ? 0 : c.y >= 1.0f ? 255 : static_cast<uint8_t>(c.y * 255.0f),
                                c.z <= 0.0f ? 0 : c.z >= 1.0f ? 255 : static_cast<uint8_t>(c.z * 255.0f));

                            if (s_debugEnabled) {
                                s_normalBuf[idx * 3 + 0] = tri.normal.X;
                                s_normalBuf[idx * 3 + 1] = tri.normal.Y;
                                s_normalBuf[idx * 3 + 2] = tri.normal.Z;
                            }
                        }
                        z += dz_dx;
                        if (needPC)  iw += dinvW_dx;
                        if (needPos) { px += dpx_dx; py += dpy_dx; pz += dpz_dx; }
                        if (needUV)  { uvx += duvx_dx; uvy += duvy_dx; }
                    }
                }
            }

            // Advance row-start values
            v_rowStart   += dv_dy;
            w_rowStart   += dw_dy;
            z_rowStart   += dz_dy;
            if (needPC)  invW_rowStart += dinvW_dy;
            if (needPos) { px_rowStart += dpx_dy; py_rowStart += dpy_dy; pz_rowStart += dpz_dy; }
            if (needUV)  { uvx_rowStart += duvx_dy; uvy_rowStart += duvy_dy; }
        }
    }
    _perf.EndSample("SW.Triangles");

    // 7) Sun/moon disc overlay on sky pixels (depth still at infinity)
    _perf.BeginSample("SW.SunMoon");
    if (hasSky && sky.IsEnabled()) {
        int sunSX = 0, sunSY = 0, moonSX = 0, moonSY = 0;
        bool sunVis = false, moonVis = false;

        Quaternion invRot = lookDirection.Conjugate();
        Vector3D camScl = camera->GetTransform()->GetScale();

        float sunDist = 400.0f;
        Vector3D sunWorld = sky.GetSunDirection() * sunDist;
        Vector3D moonWorld = sky.GetMoonDirection() * sunDist;

        auto projectCelestial = [&](const Vector3D& worldDir, int& sx, int& sy) -> bool {
            Vector3D rel = worldDir - s_cameraPos;
            Vector3D proj = invRot.RotateVector(rel);
            if (camScl.X != 0) proj.X /= camScl.X;
            if (camScl.Y != 0) proj.Y /= camScl.Y;
            if (camScl.Z != 0) proj.Z /= camScl.Z;
            float nz = -proj.Z;
            if (perspective && nz < nearPlane) return false;
            float dz = nz > nearPlane ? nz : nearPlane;
            sx = static_cast<int>(viewportCenterX + proj.X * fovScale / dz * viewportHalfH + 0.5f);
            sy = static_cast<int>(viewportCenterY - proj.Y * fovScale / dz * viewportHalfH + 0.5f);
            return sx >= 0 && sx < vpW && sy >= 0 && sy < vpH;
        };

        sunVis  = projectCelestial(sunWorld,  sunSX,  sunSY);
        moonVis = projectCelestial(moonWorld, moonSX, moonSY);

        if (sunVis || moonVis) {
            float sr = sky.GetSunScreenRadius();
            float mr = sky.GetMoonScreenRadius();
            float sunRadSq  = sr * sr;
            float moonRadSq = mr * mr;
            float sunGlowRadSq  = (sr * 3.0f) * (sr * 3.0f);
            float moonGlowRadSq = (mr * 2.0f) * (mr * 2.0f);
            Color888 sunColor  = sky.GetSunColor();
            Color888 moonColor = sky.GetMoonColor();

            float maxGlowR = std::max(sr * 3.0f, mr * 2.0f);
            int cMinX = vpW, cMaxX = 0, cMinY = vpH, cMaxY = 0;
            if (sunVis) {
                cMinX = std::max(0, static_cast<int>(sunSX - maxGlowR));
                cMaxX = std::min(vpW - 1, static_cast<int>(sunSX + maxGlowR));
                cMinY = std::max(0, static_cast<int>(sunSY - maxGlowR));
                cMaxY = std::min(vpH - 1, static_cast<int>(sunSY + maxGlowR));
            }
            if (moonVis) {
                cMinX = std::min(cMinX, std::max(0, static_cast<int>(moonSX - maxGlowR)));
                cMaxX = std::max(cMaxX, std::min(vpW - 1, static_cast<int>(moonSX + maxGlowR)));
                cMinY = std::min(cMinY, std::max(0, static_cast<int>(moonSY - maxGlowR)));
                cMaxY = std::max(cMaxY, std::min(vpH - 1, static_cast<int>(moonSY + maxGlowR)));
            }

            float skyThreshold = std::numeric_limits<float>::max() * 0.5f;
            for (int py = cMinY; py <= cMaxY; ++py) {
                for (int px = cMinX; px <= cMaxX; ++px) {
                    int idx = py * vpW + px;
                    if (s_depthBuf[idx] < skyThreshold) continue;

                    Color888 out = colorBuf[idx];
                    if (sunVis) {
                        float dx = static_cast<float>(px - sunSX);
                        float dy = static_cast<float>(py - sunSY);
                        float distSq = dx * dx + dy * dy;
                        if (distSq < sunRadSq) {
                            out = sunColor;
                        } else if (distSq < sunGlowRadSq) {
                            float dist = distSq * FastInvSqrt(distSq);
                            float glow = 1.0f - (dist - sr) / (sr * 2.0f);
                            glow *= glow;
                            out = Color888(
                                static_cast<uint8_t>(std::min(255, out.R + static_cast<int>(sunColor.R * glow * 0.5f))),
                                static_cast<uint8_t>(std::min(255, out.G + static_cast<int>(sunColor.G * glow * 0.4f))),
                                static_cast<uint8_t>(std::min(255, out.B + static_cast<int>(sunColor.B * glow * 0.3f))));
                        }
                    }
                    if (moonVis) {
                        float dx = static_cast<float>(px - moonSX);
                        float dy = static_cast<float>(py - moonSY);
                        float distSq = dx * dx + dy * dy;
                        if (distSq < moonRadSq) {
                            out = moonColor;
                        } else if (distSq < moonGlowRadSq) {
                            float dist = distSq * FastInvSqrt(distSq);
                            float glow = 1.0f - (dist - mr) / mr;
                            glow *= glow;
                            out = Color888(
                                static_cast<uint8_t>(std::min(255, out.R + static_cast<int>(moonColor.R * glow * 0.2f))),
                                static_cast<uint8_t>(std::min(255, out.G + static_cast<int>(moonColor.G * glow * 0.2f))),
                                static_cast<uint8_t>(std::min(255, out.B + static_cast<int>(moonColor.B * glow * 0.25f))));
                        }
                    }
                    colorBuf[idx] = out;
                }
            }
        }
    }
    _perf.EndSample("SW.SunMoon");
}

void Rasterizer::DrawLine3D(const Vector3D& worldA, const Vector3D& worldB,
                             Color888 color, bool depthTest,
                             Color888* buffer, int w, int h) {
    if (!buffer || w <= 0 || h <= 0) return;
    if (s_depthBuf.empty() || s_bufW != w || s_bufH != h) return;

    auto toCam = [&](const Vector3D& world, Vector3D& cam, float& z) {
        Vector3D rel = world - s_rsCamPos;
        cam = s_rsInvRot.RotateVector(rel);
        if (s_rsCamScl.X != 0) cam.X /= s_rsCamScl.X;
        if (s_rsCamScl.Y != 0) cam.Y /= s_rsCamScl.Y;
        if (s_rsCamScl.Z != 0) cam.Z /= s_rsCamScl.Z;
        z = -cam.Z;
    };

    Vector3D camA, camB;
    float zA, zB;
    toCam(worldA, camA, zA);
    toCam(worldB, camB, zB);

    if (zA < s_rsNearPlane && zB < s_rsNearPlane) return;

    if (zA < s_rsNearPlane) {
        float t = (s_rsNearPlane - zA) / (zB - zA);
        camA.X += (camB.X - camA.X) * t;
        camA.Y += (camB.Y - camA.Y) * t;
        zA = s_rsNearPlane;
    } else if (zB < s_rsNearPlane) {
        float t = (s_rsNearPlane - zB) / (zA - zB);
        camB.X += (camA.X - camB.X) * t;
        camB.Y += (camA.Y - camB.Y) * t;
        zB = s_rsNearPlane;
    }

    if (!s_rsPerspective) {
        float sx0 = s_rsCX + camA.X * s_rsHH;
        float sy0 = s_rsCY - camA.Y * s_rsHH;
        float sx1 = s_rsCX + camB.X * s_rsHH;
        float sy1 = s_rsCY - camB.Y * s_rsHH;
        zA = zA > s_rsNearPlane ? zA : s_rsNearPlane;
        zB = zB > s_rsNearPlane ? zB : s_rsNearPlane;
        camA.X = sx0; camA.Y = sy0;
        camB.X = sx1; camB.Y = sy1;
    } else {
        float dzA = zA > s_rsNearPlane ? zA : s_rsNearPlane;
        float dzB = zB > s_rsNearPlane ? zB : s_rsNearPlane;
        float px0 = s_rsCX + camA.X * s_rsFovScale / dzA * s_rsHH;
        float py0 = s_rsCY - camA.Y * s_rsFovScale / dzA * s_rsHH;
        float px1 = s_rsCX + camB.X * s_rsFovScale / dzB * s_rsHH;
        float py1 = s_rsCY - camB.Y * s_rsFovScale / dzB * s_rsHH;
        zA = dzA; zB = dzB;
        camA.X = px0; camA.Y = py0;
        camB.X = px1; camB.Y = py1;
    }

    float t0 = 0.0f, t1 = 1.0f;
    float dsx = camB.X - camA.X, dsy = camB.Y - camA.Y;
    auto clip = [](float p, float q, float& t0_, float& t1_) -> bool {
        if (p == 0.0f) return q >= 0.0f;
        float t = q / p;
        if (p < 0.0f) { if (t > t1_) return false; if (t > t0_) t0_ = t; }
        else           { if (t < t0_) return false; if (t < t1_) t1_ = t; }
        return true;
    };
    if (!clip(-dsx, camA.X,           t0, t1)) return;
    if (!clip( dsx, (w - 1) - camA.X, t0, t1)) return;
    if (!clip(-dsy, camA.Y,           t0, t1)) return;
    if (!clip( dsy, (h - 1) - camA.Y, t0, t1)) return;

    int ix0 = static_cast<int>(camA.X + t0 * dsx + 0.5f);
    int iy0 = static_cast<int>(camA.Y + t0 * dsy + 0.5f);
    int ix1 = static_cast<int>(camA.X + t1 * dsx + 0.5f);
    int iy1 = static_cast<int>(camA.Y + t1 * dsy + 0.5f);
    float iz0 = (1.0f - t0) / zA + t0 / zB;
    float iz1 = (1.0f - t1) / zA + t1 / zB;

    int adx = ix1 - ix0 < 0 ? -(ix1 - ix0) : (ix1 - ix0);
    int ady = iy1 - iy0 < 0 ? -(iy1 - iy0) : (iy1 - iy0);
    int sx = ix0 < ix1 ? 1 : -1;
    int sy = iy0 < iy1 ? 1 : -1;
    int err = adx - ady;
    int steps = adx > ady ? adx : ady;
    if (steps == 0) steps = 1;
    float izStep = (iz1 - iz0) / static_cast<float>(steps);
    float iz = iz0;

    for (;;) {
        if (ix0 >= 0 && ix0 < w && iy0 >= 0 && iy0 < h) {
            int idx = iy0 * w + ix0;
            float z = (iz > 0.0f) ? 1.0f / iz : 0.0f;
            if (!depthTest || z < s_depthBuf[idx] + 0.01f * z) {
                buffer[idx] = color;
                if (depthTest) s_depthBuf[idx] = z;
            }
        }
        if (ix0 == ix1 && iy0 == iy1) break;
        int e2 = 2 * err;
        if (e2 > -ady) { err -= ady; ix0 += sx; }
        if (e2 <  adx) { err += adx; iy0 += sy; }
        iz += izStep;
    }
}

} // namespace koilo
