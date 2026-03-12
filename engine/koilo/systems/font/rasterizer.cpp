// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/font/rasterizer.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace koilo {
namespace font {

GlyphBitmap GlyphRasterizer::Rasterize(const GlyphOutline& outline,
                                        float scale, int padding) const {
    GlyphBitmap bmp;
    if (outline.contours.empty()) {
        bmp.width = static_cast<int>(
            std::ceil(outline.advanceWidth * scale)) + padding * 2;
        bmp.height = padding * 2;
        bmp.pixels.resize(bmp.width * bmp.height, 0);
        bmp.originX = padding;
        bmp.originY = padding;
        return bmp;
    }

    // Compute scaled bounding box (Y is flipped: TTF Y-up -> bitmap Y-down)
    float sxMin = outline.xMin * scale;
    float sxMax = outline.xMax * scale;
    float syMin = -(outline.yMax * scale);  // flip Y
    float syMax = -(outline.yMin * scale);

    int ixMin = static_cast<int>(std::floor(sxMin)) - padding;
    int iyMin = static_cast<int>(std::floor(syMin)) - padding;
    int ixMax = static_cast<int>(std::ceil(sxMax))  + padding;
    int iyMax = static_cast<int>(std::ceil(syMax))  + padding;

    bmp.width  = std::min(ixMax - ixMin, MAX_GLYPH_SIZE);
    bmp.height = std::min(iyMax - iyMin, MAX_GLYPH_SIZE);
    bmp.originX = -ixMin;
    bmp.originY = -iyMin;

    if (bmp.width <= 0 || bmp.height <= 0) return bmp;

    // Build edge list from contours
    std::vector<Edge> edges;
    BuildEdges(outline, scale, edges);

    // Rasterize with supersampling
    int ssHeight = bmp.height * SUPERSAMPLE;
    std::vector<float> coverage(bmp.width * bmp.height, 0.0f);

    for (int ssRow = 0; ssRow < ssHeight; ++ssRow) {
        float scanY = static_cast<float>(iyMin) +
                      (static_cast<float>(ssRow) + 0.5f) /
                      static_cast<float>(SUPERSAMPLE);

        // Collect x-intersections at this scanline
        intersections_.clear();
        for (const auto& edge : edges) {
            float eMinY = std::min(edge.y0, edge.y1);
            float eMaxY = std::max(edge.y0, edge.y1);
            if (scanY < eMinY || scanY >= eMaxY) continue;

            float dy = edge.y1 - edge.y0;
            if (std::fabs(dy) < 1e-8f) continue;
            float t = (scanY - edge.y0) / dy;
            float ix = edge.x0 + t * (edge.x1 - edge.x0);
            intersections_.push_back(ix);
        }

        if (intersections_.empty()) continue;
        std::sort(intersections_.begin(), intersections_.end());

        // Fill between pairs (non-zero winding / even-odd)
        int destRow = ssRow / SUPERSAMPLE;
        float weight = 1.0f / static_cast<float>(SUPERSAMPLE);

        for (size_t i = 0; i + 1 < intersections_.size(); i += 2) {
            float xLeft  = intersections_[i] -
                           static_cast<float>(ixMin);
            float xRight = intersections_[i + 1] -
                           static_cast<float>(ixMin);

            int colStart = std::max(0, static_cast<int>(std::floor(xLeft)));
            int colEnd   = std::min(bmp.width,
                                    static_cast<int>(std::ceil(xRight)));

            for (int col = colStart; col < colEnd; ++col) {
                float pixLeft  = static_cast<float>(col);
                float pixRight = static_cast<float>(col + 1);
                float overlap  = std::min(pixRight, xRight) -
                                 std::max(pixLeft, xLeft);
                if (overlap > 0.0f) {
                    coverage[destRow * bmp.width + col] +=
                        overlap * weight;
                }
            }
        }
    }

    // Convert coverage to alpha bytes
    bmp.pixels.resize(bmp.width * bmp.height);
    for (int i = 0; i < bmp.width * bmp.height; ++i) {
        float c = coverage[i];
        if (c <= 0.0f) { bmp.pixels[i] = 0; continue; }
        if (c >= 1.0f) { bmp.pixels[i] = 255; continue; }
        bmp.pixels[i] = static_cast<uint8_t>(c * 255.0f + 0.5f);
    }

    return bmp;
}

void GlyphRasterizer::BuildEdges(const GlyphOutline& outline, float scale,
                                  std::vector<Edge>& edges) const {
    for (const auto& contour : outline.contours) {
        if (contour.points.size() < 2) continue;

        const auto& pts = contour.points;
        size_t n = pts.size();

        // Resolve implicit on-curve points between consecutive off-curve
        std::vector<GlyphPoint> resolved;
        resolved.reserve(n * 2);

        for (size_t i = 0; i < n; ++i) {
            const auto& cur = pts[i];
            const auto& next = pts[(i + 1) % n];

            resolved.push_back(cur);

            if (!cur.onCurve && !next.onCurve) {
                // Insert implicit on-curve midpoint
                GlyphPoint mid;
                mid.x = (cur.x + next.x) * 0.5f;
                mid.y = (cur.y + next.y) * 0.5f;
                mid.onCurve = true;
                resolved.push_back(mid);
            }
        }

        // Walk resolved points, emit edges
        size_t rn = resolved.size();

        (void)rn;

        // Find first on-curve point
        size_t startIdx = 0;
        for (size_t j = 0; j < resolved.size(); ++j) {
            if (resolved[j].onCurve) { startIdx = j; break; }
        }

        for (size_t step = 0; step < resolved.size(); ) {
            size_t cur = (startIdx + step) % resolved.size();
            size_t next1 = (startIdx + step + 1) % resolved.size();

            if (resolved[cur].onCurve && resolved[next1].onCurve) {
                // Straight line
                Edge e;
                e.x0 = resolved[cur].x * scale;
                e.y0 = resolved[cur].y * scale;
                e.x1 = resolved[next1].x * scale;
                e.y1 = resolved[next1].y * scale;
                // TTF Y is up, bitmap Y is down - flip Y
                e.y0 = -e.y0;
                e.y1 = -e.y1;
                edges.push_back(e);
                step += 1;
            } else if (resolved[cur].onCurve && !resolved[next1].onCurve) {
                // Quadratic Bézier: cur -> next1 -> next2
                size_t next2 = (startIdx + step + 2) % rn;
                FlattenQuadratic(
                    resolved[cur].x * scale, -resolved[cur].y * scale,
                    resolved[next1].x * scale, -resolved[next1].y * scale,
                    resolved[next2].x * scale, -resolved[next2].y * scale,
                    edges);
                step += 2;
            } else {
                step += 1;
            }
        }
    }
}

void GlyphRasterizer::FlattenQuadratic(float x0, float y0, float cx, float cy,
                                        float x1, float y1,
                                        std::vector<Edge>& edges,
                                        int depth) const {
    if (depth > 12) {
        edges.push_back({x0, y0, x1, y1});
        return;
    }

    // Flatness: distance from control point to chord midpoint
    float mx = (x0 + x1) * 0.5f;
    float my = (y0 + y1) * 0.5f;
    float dx = cx - mx;
    float dy = cy - my;
    if (dx * dx + dy * dy < 0.25f) {
        edges.push_back({x0, y0, x1, y1});
        return;
    }

    // Subdivide at t=0.5
    float m0x = (x0 + cx) * 0.5f, m0y = (y0 + cy) * 0.5f;
    float m1x = (cx + x1) * 0.5f, m1y = (cy + y1) * 0.5f;
    float mpx = (m0x + m1x) * 0.5f, mpy = (m0y + m1y) * 0.5f;

    FlattenQuadratic(x0, y0, m0x, m0y, mpx, mpy, edges, depth + 1);
    FlattenQuadratic(mpx, mpy, m1x, m1y, x1, y1, edges, depth + 1);
}

} // namespace font
} // namespace koilo
