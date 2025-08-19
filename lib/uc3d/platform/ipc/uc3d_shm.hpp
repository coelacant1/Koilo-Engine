#pragma once
#include <cstdint>
#include <atomic>
#include <cstring>
#include <vector>
#include <algorithm>
#include <cstdio>   // std::snprintf

/**
 * @file uc3d_shm.hpp
 * @brief Shared-memory IPC structures and helpers for UC3D framebuffers, geometry, control, and registry.
 */

#if defined(__unix__) || defined(__APPLE__)
    #define UC3D_HAS_POSIX_SHM 1
    #if defined(__has_include)
        #if __has_include(<sys/mman.h>)
            #include <sys/mman.h>
        #endif
    #endif
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
#else
    #define UC3D_HAS_POSIX_SHM 0
#endif

#pragma pack(push, 1)

/**
 * @brief Framebuffer header (RGB888, ring of buffers).
 *
 * magic: 'UCFB' (0x55434642)
 */
struct Uc3dFbHeader {
    uint32_t magic;        ///< Magic: 'UCFB'
    uint16_t version;      ///< Version (1)
    uint16_t format;       ///< 0 = RGB888
    uint32_t width;        ///< Width in pixels
    uint32_t height;       ///< Height in pixels
    uint32_t stride_bytes; ///< Bytes per row (width*3)
    uint32_t buffer_count; ///< Ring buffer count (e.g., 3)
    uint32_t active_index; ///< Last published buffer index
};

/**
 * @brief Framebuffer buffer header; payload (RGB888) follows.
 *
 * seq: even = being written, odd = ready.
 */
struct Uc3dFbBuffer {
    std::atomic<uint64_t> seq;
};

/**
 * @brief Shared control block.
 */
struct Uc3dCtrl {
    std::atomic<uint64_t> seq;  ///< Even=writing, odd=ready
    uint8_t  pause; uint8_t _pad[3];
    float    dt_scale;
    float    cam_pos[3];
    float    cam_look[3];
    float    cam_up[3];
    uint32_t debug_flags;
};

/**
 * @brief Geometry header for interleaved XY float32 pairs.
 *
 * magic: 'UCGM' (0x5543474D)
 */
struct Uc3dGeomHeader {
    uint32_t magic;              ///< Magic: 'UCGM'
    uint32_t count;              ///< Number of points (typically W*H)
    uint32_t width;              ///< Logical canvas width (optional)
    uint32_t height;             ///< Logical canvas height (optional)
    std::atomic<uint64_t> seq;   ///< Even=writing, odd=ready
    // payload: count * struct { float x; float y; } tightly packed
};

/**
 * @brief Registry header for enumerating cameras.
 *
 * magic: 'UCRG' (0x55435247)
 */
struct Uc3dRegHeader {
    uint32_t magic;                  ///< Magic: 'UCRG'
    uint32_t version;                ///< Version (1)
    std::atomic<uint32_t> cam_count; ///< Number of valid records
};

/**
 * @brief Per-camera registry record.
 */
struct Uc3dRegCamera {
    char     name[32];    ///< Stable user-visible id ("Main", "WS35", etc.)
    uint32_t index;       ///< Numeric index (0..N-1)
    uint32_t pixel_count; ///< N (for FB payload = 1xN)
    uint32_t width;       ///< Logical display width
    uint32_t height;      ///< Logical display height
    // Convention: channels named /uc3d_fb{index} and /uc3d_geom{index}
};

#pragma pack(pop)

/**
 * @brief Framebuffer + control shared memory handles.
 */
struct Uc3dShm {
    void* fb_base = nullptr;      ///< Mapped base of framebuffer region
    size_t fb_size = 0;           ///< Size of framebuffer mapping
    Uc3dFbHeader* fb_hdr = nullptr;
    uint8_t* fb_payload_base = nullptr;

    void* ctrl_base = nullptr;    ///< Mapped base of control region
    size_t ctrl_size = 0;         ///< Size of control mapping
    Uc3dCtrl* ctrl = nullptr;

#if UC3D_HAS_POSIX_SHM
    int fb_fd = -1;               ///< shm fd for framebuffer
    int ctrl_fd = -1;             ///< shm fd for control
#endif
};

/**
 * @brief Geometry shared memory handle.
 */
struct Uc3dGeom {
    void* base = nullptr;   ///< Mapped base
    size_t size = 0;        ///< Mapping size
#if UC3D_HAS_POSIX_SHM
    int fd = -1;            ///< shm fd
#endif
    Uc3dGeomHeader* hdr = nullptr;
    float* xy = nullptr;    ///< Interleaved XY float32
    uint32_t width = 0;     ///< Informational
    uint32_t height = 0;    ///< Informational
};

/**
 * @brief Registry shared memory handle.
 */
struct Uc3dRegistry {
    void* base = nullptr;
    size_t size = 0;
#if UC3D_HAS_POSIX_SHM
    int fd = -1;
#endif
    Uc3dRegHeader* hdr = nullptr;
    Uc3dRegCamera* cams = nullptr;
};

/** @brief Bytes for RGB payload (h * stride). */
inline size_t uc3d_payload_bytes(uint32_t h, uint32_t stride) { return size_t(h) * size_t(stride); }

/** @brief Bytes for a single framebuffer buffer (header + payload). */
inline size_t uc3d_onebuf_bytes(uint32_t h, uint32_t stride) { return sizeof(Uc3dFbBuffer) + uc3d_payload_bytes(h, stride); }

/** @brief Total bytes for a geometry segment (header + XY float32 pairs). */
inline size_t uc3d_geom_bytes(uint32_t count) {
    return sizeof(Uc3dGeomHeader) + size_t(count) * sizeof(float) * 2;
}

/**
 * @brief Initialize framebuffer + control shared memory regions.
 * @return true on success; false if POSIX SHM unsupported or any step fails.
 */
inline bool uc3d_shm_init(Uc3dShm& S, const char* fb_name, uint32_t w, uint32_t h, const char* ctrl_name) {
#if !UC3D_HAS_POSIX_SHM
    (void)S; (void)fb_name; (void)w; (void)h; (void)ctrl_name;
    return false;
#else
    const uint32_t stride = w * 3, bufcnt = 3;
    const size_t header_bytes = sizeof(Uc3dFbHeader);
    const size_t onebuf_bytes = sizeof(Uc3dFbBuffer) + size_t(h) * stride;
    const size_t fb_bytes = header_bytes + bufcnt * onebuf_bytes;

    S.fb_fd = shm_open(fb_name, O_CREAT | O_RDWR, 0666);
    if (S.fb_fd < 0) return false;
    if (ftruncate(S.fb_fd, fb_bytes) != 0) return false;
    S.fb_base = mmap(nullptr, fb_bytes, PROT_READ | PROT_WRITE, MAP_SHARED, S.fb_fd, 0);
    if (S.fb_base == MAP_FAILED) return false;
    S.fb_size = fb_bytes;

    S.fb_hdr = reinterpret_cast<Uc3dFbHeader*>(S.fb_base);
    S.fb_hdr->magic = 0x55434642; S.fb_hdr->version = 1; S.fb_hdr->format = 0;
    S.fb_hdr->width = w; S.fb_hdr->height = h; S.fb_hdr->stride_bytes = stride;
    S.fb_hdr->buffer_count = bufcnt; S.fb_hdr->active_index = 0;

    S.fb_payload_base = reinterpret_cast<uint8_t*>(S.fb_base) + header_bytes;
    for (uint32_t i = 0; i < bufcnt; ++i){
        auto* b = reinterpret_cast<Uc3dFbBuffer*>(S.fb_payload_base + i * onebuf_bytes);
        b->seq.store(1, std::memory_order_relaxed); // odd = ready/empty
    }

    S.ctrl_fd = shm_open(ctrl_name, O_CREAT | O_RDWR, 0666);
    if (S.ctrl_fd < 0) return false;
    S.ctrl_size = sizeof(Uc3dCtrl);
    if (ftruncate(S.ctrl_fd, S.ctrl_size) != 0) return false;
    S.ctrl_base = mmap(nullptr, S.ctrl_size, PROT_READ | PROT_WRITE, MAP_SHARED, S.ctrl_fd, 0);
    if (S.ctrl_base == MAP_FAILED) return false;
    S.ctrl = reinterpret_cast<Uc3dCtrl*>(S.ctrl_base);
    std::memset(S.ctrl, 0, S.ctrl_size);
    S.ctrl->dt_scale = 1.0f;
    S.ctrl->cam_look[2] = -1.0f;
    return true;
#endif
}

/**
 * @brief Pointer to RGB payload of buffer @p buf_index.
 */
inline uint8_t* uc3d_payload_ptr(Uc3dShm& S, uint32_t buf_index, uint32_t w, uint32_t h) {
    const size_t stride = size_t(w) * 3;
    const size_t onebuf = sizeof(Uc3dFbBuffer) + size_t(h) * stride;
    return reinterpret_cast<uint8_t*>(S.fb_payload_base) + buf_index * onebuf + sizeof(Uc3dFbBuffer);
}

/**
 * @brief Publish an RGB888 frame to the next ring buffer and mark it ready.
 */
inline void uc3d_publish_rgb888(Uc3dShm& S, uint32_t w, uint32_t h, const uint8_t* src_rgb888) {
#if !UC3D_HAS_POSIX_SHM
    (void)S; (void)w; (void)h; (void)src_rgb888;
#else
    const uint32_t bufcnt = S.fb_hdr->buffer_count;
    uint32_t idx = (S.fb_hdr->active_index + 1) % bufcnt;
    const size_t stride = size_t(w) * 3;
    const size_t onebuf = sizeof(Uc3dFbBuffer) + size_t(h) * stride;
    auto* b = reinterpret_cast<Uc3dFbBuffer*>(reinterpret_cast<uint8_t*>(S.fb_payload_base) + idx * onebuf);

    uint64_t s = b->seq.load(std::memory_order_relaxed);
    if (s & 1ULL) s++;                 // even = being written
    b->seq.store(s, std::memory_order_release);

    std::memcpy(uc3d_payload_ptr(S, idx, w, h), src_rgb888, size_t(h) * stride);

    b->seq.store(s + 1, std::memory_order_release); // odd = ready
    S.fb_hdr->active_index = idx;
#endif
}

/**
 * @brief Initialize a geometry shared-memory segment with @p count XY pairs.
 * @return true on success; false if POSIX SHM unsupported or any step fails.
 */
inline bool uc3d_geom_init(Uc3dGeom& G, const char* name, uint32_t count) {
#if !UC3D_HAS_POSIX_SHM
    (void)G; (void)name; (void)count;
    return false;
#else
    const size_t payload_bytes = size_t(count) * sizeof(float) * 2; // x,y
    const size_t total_bytes   = sizeof(Uc3dGeomHeader) + payload_bytes;

    G.fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (G.fd < 0) return false;
    if (ftruncate(G.fd, total_bytes) != 0) return false;

    void* base = mmap(nullptr, total_bytes, PROT_READ | PROT_WRITE, MAP_SHARED, G.fd, 0);
    if (base == MAP_FAILED) return false;

    G.base = base;
    G.size = total_bytes;
    G.hdr  = reinterpret_cast<Uc3dGeomHeader*>(base);
    G.xy   = reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(base) + sizeof(Uc3dGeomHeader));

    G.hdr->magic  = 0x5543474D;                // 'UCGM'
    G.hdr->count  = count;
    G.hdr->width  = G.width;                   // optional meta
    G.hdr->height = G.height;                  // optional meta
    G.hdr->seq.store(1, std::memory_order_relaxed); // odd => ready/empty
    return true;
#endif
}

/**
 * @brief Pointer to interleaved XY payload.
 */
inline float* uc3d_geom_payload(Uc3dGeom& G) {
    return reinterpret_cast<float*>(
        reinterpret_cast<uint8_t*>(G.base) + sizeof(Uc3dGeomHeader));
}

/**
 * @brief Publish @p count XY pairs into the geometry segment and mark ready.
 */
inline void uc3d_geom_publish(Uc3dGeom& G, const float* xy, uint32_t count) {
#if !UC3D_HAS_POSIX_SHM
    (void)G; (void)xy; (void)count;
#else
    if (!G.base || !G.hdr || !G.xy) return;
    if (count != G.hdr->count)      return;

    uint64_t s = G.hdr->seq.load(std::memory_order_relaxed);
    if (s & 1ULL) s++; // even = being written
    G.hdr->seq.store(s, std::memory_order_release);

    std::memcpy(G.xy, xy, size_t(count) * sizeof(float) * 2);

    G.hdr->seq.store(s + 1, std::memory_order_release); // odd = ready
#endif
}

/** @brief Alias of uc3d_geom_publish (XY form). */
inline void uc3d_geom_publish_xy(Uc3dGeom& G, const float* xy, uint32_t count) {
    uc3d_geom_publish(G, xy, count);
}

/** @brief RGB triple used by scatter rasterizer. */
struct Uc3dRGB { uint8_t r, g, b; };

/**
 * @brief Map N 2D points onto a W×H raster (top-left origin) with nearest-pixel placement.
 *
 * Y is inverted (input +Y up; image expects +Y down). Pixels not hit remain 0.
 *
 * @tparam GetCoordFn  Callable (i)-> { x, y }
 * @tparam GetColorFn  Callable (i)-> { r, g, b }
 */
template <class GetCoordFn, class GetColorFn>
inline void uc3d_rasterize_scatter_rgb888(
    uint32_t W, uint32_t H,
    uint8_t* out_rgb888,               // size = W*H*3
    GetCoordFn getCoord,               // (i)-> {x,y}
    GetColorFn getColor,               // (i)-> {r,g,b}
    uint32_t count) {
    std::fill(out_rgb888, out_rgb888 + size_t(W) * H * 3, 0);

    // Compute bounds from coordinates
    float minx = +1e30f, maxx = -1e30f, miny = +1e30f, maxy = -1e30f;
    for (uint32_t i = 0; i < count; ++i){
        auto c = getCoord(i);
        minx = std::min(minx, c.x); maxx = std::max(maxx, c.x);
        miny = std::min(miny, c.y); maxy = std::max(maxy, c.y);
    }
    const float dx = std::max(1e-6f, maxx - minx);
    const float dy = std::max(1e-6f, maxy - miny);

    for (uint32_t i = 0; i < count; ++i){
        auto c   = getCoord(i);
        auto col = getColor(i);
        const float nx = (c.x - minx) / dx;               // 0..1
        const float ny = (c.y - miny) / dy;               // 0..1 (up)
        const int ix = int(nx * (W - 1) + 0.5f);
        const int iy = int((1.0f - ny) * (H - 1) + 0.5f); // invert Y for top-left origin
        if (ix < 0 || iy < 0 || ix >= int(W) || iy >= int(H)) continue;
        const size_t o = (size_t(iy) * W + ix) * 3;
        out_rgb888[o + 0] = col.r;
        out_rgb888[o + 1] = col.g;
        out_rgb888[o + 2] = col.b;
    }
}

/**
 * @brief Initialize registry with capacity for @p max_cameras records.
 * @return true on success; false if POSIX SHM unsupported or any step fails.
 */
inline bool uc3d_registry_init(Uc3dRegistry& R, const char* name, uint32_t max_cameras) {
#if !UC3D_HAS_POSIX_SHM
    (void)R; (void)name; (void)max_cameras; return false;
#else
    const size_t bytes = sizeof(Uc3dRegHeader) + size_t(max_cameras) * sizeof(Uc3dRegCamera);
    R.fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (R.fd < 0) return false;
    if (ftruncate(R.fd, bytes) != 0) return false;
    void* base = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, R.fd, 0);
    if (base == MAP_FAILED) return false;
    R.base = base; R.size = bytes;
    R.hdr  = reinterpret_cast<Uc3dRegHeader*>(base);
    R.cams = reinterpret_cast<Uc3dRegCamera*>(R.hdr + 1);

    R.hdr->magic = 0x55435247;   // 'UCRG'
    R.hdr->version = 1;
    R.hdr->cam_count.store(0, std::memory_order_relaxed);
    return true;
#endif
}

/**
 * @brief Set a registry record at index @p i.
 */
inline void uc3d_registry_set(Uc3dRegistry& R, uint32_t i, const char* name,
                              uint32_t index, uint32_t count, uint32_t w, uint32_t h) {
    auto& rec = R.cams[i];
    std::snprintf(rec.name, sizeof(rec.name), "%s", name ? name : "Camera");
    rec.index = index;
    rec.pixel_count = count;
    rec.width  = w;
    rec.height = h;
}

/**
 * @brief Publish the number of valid camera records.
 */
inline void uc3d_registry_publish(Uc3dRegistry& R, uint32_t cam_count) {
    R.hdr->cam_count.store(cam_count, std::memory_order_release);
}
