// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file scenedata.hpp
 * @brief Serializable scene data structures for pre-compiled scenes.
 *
 * This file defines POD (Plain Old Data) structures that can be serialized
 * to binary format and loaded on microcontrollers without dynamic allocation
 * during scene definition. The structures are designed for embedded systems
 * with limited RAM and flash memory.
 *
 * Design goals:
 * - Zero virtual functions (embedded-friendly)
 * - Minimal padding (tight memory layout)
 * - Endian-safe (cross-platform compatibility)
 * - PROGMEM compatible (AVR flash storage)
 *
 * @date 2025-12-11
 * @author Coela
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>  // for memset

namespace koilo {

// Forward declarations
class Scene;
class Mesh;
class Material;
class CameraManager;
class DisplayManager;
class IDisplayBackend;

//==============================================================================
// Magic Numbers & Version
//==============================================================================

constexpr uint32_t KOILOSCENE_MAGIC = 0x4B4F494C;  // "KOIL" in ASCII
constexpr uint32_t KOILOSCENE_VERSION = 0x00000001; // Version 1

//==============================================================================
// Geometry Data
//==============================================================================

/**
 * @struct Vector3DData
 * @brief Serializable 3D vector (position, direction, scale).
 */
struct Vector3DData {
    float x;
    float y;
    float z;
    
    Vector3DData() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector3DData(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
} __attribute__((packed));

/**
 * @struct QuaternionData
 * @brief Serializable quaternion for rotations.
 */
struct QuaternionData {
    float x;
    float y;
    float z;
    float w;
    
    QuaternionData() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
    QuaternionData(float x_, float y_, float z_, float w_) 
        : x(x_), y(y_), z(z_), w(w_) {}
} __attribute__((packed));

/**
 * @struct TransformData
 * @brief Serializable transform (position, rotation, scale).
 */
struct TransformData {
    Vector3DData position;
    QuaternionData rotation;
    Vector3DData scale;
    
    TransformData() : scale(1.0f, 1.0f, 1.0f) {}
} __attribute__((packed));

/**
 * @struct IndexGroupData
 * @brief Serializable triangle indices (3 vertices).
 */
struct IndexGroupData {
    uint16_t a;
    uint16_t b;
    uint16_t c;
    
    IndexGroupData() : a(0), b(0), c(0) {}
    IndexGroupData(uint16_t a_, uint16_t b_, uint16_t c_) 
        : a(a_), b(b_), c(c_) {}
} __attribute__((packed));

/**
 * @struct TriangleGroupData
 * @brief Serializable triangle mesh geometry.
 *
 * Storage layout in binary file:
 * - This header struct
 * - Array of Vector3DData (vertexCount elements)
 * - Array of IndexGroupData (indexCount elements)
 */
struct TriangleGroupData {
    uint32_t vertexCount;    ///< Number of vertices
    uint32_t indexCount;     ///< Number of triangles (IndexGroups)
    
    // Offsets from start of scene data (not pointers!)
    // Loader will resolve these to actual pointers
    uint32_t verticesOffset; ///< Byte offset to vertex array
    uint32_t indicesOffset;  ///< Byte offset to index array
    
    TriangleGroupData() 
        : vertexCount(0), indexCount(0), 
          verticesOffset(0), indicesOffset(0) {}
} __attribute__((packed));

//==============================================================================
// Material Data
//==============================================================================

/**
 * @enum MaterialType
 * @brief Type identifier for material variants.
 */
enum class MaterialType : uint8_t {
    UniformColor = 0,      ///< Solid color material
    PhongLight = 1,        ///< Phong-lit material
    Spiral = 2,            ///< Spiral pattern material
    UVMap = 3,             ///< Texture-mapped material
    Text = 4,              ///< Text rendering material
    ImageSequence = 5,     ///< Animated texture sequence
    VectorField2D = 6,     ///< 2D vector field visualization
    AudioReactive = 7,     ///< Audio-reactive (runtime only)
    Custom = 255           ///< Custom material (future)
};

/**
 * @struct Color888Data
 * @brief 24-bit RGB color.
 */
struct Color888Data {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    
    Color888Data() : r(255), g(255), b(255) {}
    Color888Data(uint8_t r_, uint8_t g_, uint8_t b_) 
        : r(r_), g(g_), b(b_) {}
} __attribute__((packed));

/**
 * @struct MaterialData
 * @brief Serializable material parameters.
 *
 * Uses a tagged union to store different material types compactly.
 * Only the relevant data for the specified MaterialType is valid.
 */
struct MaterialData {
    MaterialType type;
    
    // Simplified: No union, just flat fields
    // Python and C++ both read the fields they need based on type
    Color888Data color;          // UniformColor, Text
    Color888Data ambient;        // PhongLight
    Color888Data diffuse;        // PhongLight
    Color888Data specular;       // PhongLight
    float shininess;             // PhongLight
    Color888Data centerColor;    // Spiral
    Color888Data outerColor;     // Spiral
    float rotationSpeed;         // Spiral
    uint8_t armCount;            // Spiral
    uint32_t imageDataOffset;    // UVMap, ImageSequence
    uint16_t width;              // UVMap, ImageSequence
    uint16_t height;             // UVMap, ImageSequence
    uint8_t pixelFormat;         // UVMap, ImageSequence
    uint8_t frameCount;          // ImageSequence
    uint8_t fps;                 // ImageSequence
    uint8_t fontSize;            // Text
    uint32_t textDataOffset;     // Text
    uint32_t fieldDataOffset;    // VectorField2D
    float scale;                 // VectorField2D
    
    // Padding for alignment
    uint8_t _padding[8];
    
    MaterialData() : type(MaterialType::UniformColor),
                     color(255, 255, 255),
                     ambient(0, 0, 0),
                     diffuse(255, 255, 255),
                     specular(255, 255, 255),
                     shininess(32.0f),
                     centerColor(255, 255, 255),
                     outerColor(0, 0, 0),
                     rotationSpeed(1.0f),
                     armCount(4),
                     imageDataOffset(0),
                     width(0),
                     height(0),
                     pixelFormat(0),
                     frameCount(0),
                     fps(0),
                     fontSize(16),
                     textDataOffset(0),
                     fieldDataOffset(0),
                     scale(1.0f) {
        memset(_padding, 0, sizeof(_padding));
    }
};

//==============================================================================
// Mesh Data
//==============================================================================

/**
 * @struct MeshData
 * @brief Serializable mesh (geometry + material + transform).
 */
struct MeshData {
    uint32_t staticGeometryId;      ///< Index into geometries array
    uint32_t modifiableGeometryId;  ///< Index into geometries array (or same as static)
    uint32_t materialId;            ///< Index into materials array
    TransformData transform;        ///< Initial transform
    
    MeshData() 
        : staticGeometryId(0), modifiableGeometryId(0), materialId(0) {}
} __attribute__((packed));

//==============================================================================
// Camera Data
//==============================================================================

/**
 * @struct CameraData
 * @brief Serializable camera configuration.
 */
struct CameraData {
    TransformData transform;  ///< Camera position and orientation
    float fov;                ///< Field of view in degrees
    float nearPlane;          ///< Near clipping plane
    float farPlane;           ///< Far clipping plane
    uint16_t width;           ///< Viewport width
    uint16_t height;          ///< Viewport height
    
    CameraData() 
        : fov(60.0f), nearPlane(0.1f), farPlane(1000.0f), 
          width(640), height(480) {}
} __attribute__((packed));

//==============================================================================
// Display Backend Data (EMBEDDED FOCUS)
//==============================================================================

/**
 * @enum DisplayBackendType
 * @brief Type identifier for display backends.
 *
 * PRIORITY: Embedded backends first (HUB75, SPI, I2C)
 */
enum class DisplayBackendType : uint8_t {
    // === Embedded Displays (Priority) ===
    HUB75 = 0,             ///< HUB75 RGB LED matrix (primary embedded target)
    SPI_ILI9341 = 1,       ///< ILI9341 SPI TFT display
    SPI_ST7735 = 2,        ///< ST7735 SPI TFT display
    I2C_SSD1306 = 3,       ///< SSD1306 I2C OLED display
    WS2812B = 4,           ///< WS2812B addressable LEDs (NeoPixel)
    APA102 = 5,            ///< APA102 addressable LEDs (DotStar)
    
    // === Desktop Displays (Secondary) ===
    OpenGL = 100,          ///< OpenGL window (desktop preview)
    SDL3 = 101,            ///< SDL3 window (desktop preview)
    Null = 102,            ///< Null backend (headless)
    SharedMemory = 103,    ///< Shared memory IPC
    
    Custom = 255           ///< Custom backend (future)
};

/**
 * @struct HUB75Config
 * @brief Configuration for HUB75 RGB LED matrix displays.
 *
 * Common configurations:
 * - 64x32 panels (2048 pixels)
 * - 64x64 panels (4096 pixels)
 * - Chained panels (multiple panels in series)
 */
struct HUB75Config {
    // GPIO pin assignments (platform-specific)
    uint8_t r1_pin;      ///< Red data for upper half
    uint8_t g1_pin;      ///< Green data for upper half
    uint8_t b1_pin;      ///< Blue data for upper half
    uint8_t r2_pin;      ///< Red data for lower half
    uint8_t g2_pin;      ///< Green data for lower half
    uint8_t b2_pin;      ///< Blue data for lower half
    
    uint8_t a_pin;       ///< Row address A
    uint8_t b_pin;       ///< Row address B
    uint8_t c_pin;       ///< Row address C
    uint8_t d_pin;       ///< Row address D (for 32+ row panels)
    uint8_t e_pin;       ///< Row address E (for 64+ row panels)
    
    uint8_t clk_pin;     ///< Clock signal
    uint8_t lat_pin;     ///< Latch signal
    uint8_t oe_pin;      ///< Output enable (active low)
    
    // Panel configuration
    uint8_t panelWidth;   ///< Width of single panel (typically 64)
    uint8_t panelHeight;  ///< Height of single panel (typically 32 or 64)
    uint8_t chainLength;  ///< Number of panels chained together
    uint8_t scanPattern;  ///< Scan pattern (0=1/16, 1=1/8, 2=1/4)
    
    // Display settings
    uint8_t brightness;   ///< Global brightness (0-255)
    uint8_t colorDepth;   ///< Bits per color channel (8, 10, 12)
    uint8_t refreshRate;  ///< Target refresh rate in Hz
    uint8_t _pad;
    
    HUB75Config() 
        : r1_pin(2), g1_pin(3), b1_pin(4), 
          r2_pin(5), g2_pin(6), b2_pin(7),
          a_pin(8), b_pin(9), c_pin(10), d_pin(11), e_pin(12),
          clk_pin(13), lat_pin(14), oe_pin(15),
          panelWidth(64), panelHeight(32), chainLength(1), scanPattern(0),
          brightness(128), colorDepth(8), refreshRate(60), _pad(0) {}
} __attribute__((packed));

/**
 * @struct SPIDisplayConfig
 * @brief Configuration for SPI-based TFT displays (ILI9341, ST7735, etc.).
 */
struct SPIDisplayConfig {
    uint8_t cs_pin;       ///< Chip select
    uint8_t dc_pin;       ///< Data/command select
    uint8_t rst_pin;      ///< Reset pin
    uint8_t mosi_pin;     ///< SPI MOSI (data out)
    uint8_t sck_pin;      ///< SPI clock
    uint8_t backlight_pin; ///< Backlight PWM control
    
    uint32_t spiFrequency; ///< SPI frequency in Hz (e.g., 40000000 = 40 MHz)
    uint8_t rotation;      ///< Display rotation (0, 1, 2, 3)
    uint8_t brightness;    ///< Backlight brightness (0-255)
    uint8_t _pad[2];
    
    SPIDisplayConfig()
        : cs_pin(10), dc_pin(9), rst_pin(8), 
          mosi_pin(11), sck_pin(13), backlight_pin(6),
          spiFrequency(40000000), rotation(0), brightness(255) {}
} __attribute__((packed));

/**
 * @struct I2CDisplayConfig
 * @brief Configuration for I2C-based OLED displays (SSD1306, etc.).
 */
struct I2CDisplayConfig {
    uint8_t sda_pin;      ///< I2C data line
    uint8_t scl_pin;      ///< I2C clock line
    uint8_t i2c_address;  ///< I2C device address (typically 0x3C or 0x3D)
    uint8_t rotation;     ///< Display rotation (0, 1, 2, 3)
    
    uint32_t i2cFrequency; ///< I2C frequency in Hz (e.g., 400000 = 400 kHz)
    uint8_t contrast;      ///< Display contrast (0-255)
    uint8_t _pad[3];
    
    I2CDisplayConfig()
        : sda_pin(4), scl_pin(5), i2c_address(0x3C), rotation(0),
          i2cFrequency(400000), contrast(128) {}
} __attribute__((packed));

/**
 * @struct LEDStripConfig
 * @brief Configuration for addressable LED strips (WS2812B, APA102, etc.).
 */
struct LEDStripConfig {
    uint8_t data_pin;     ///< Data pin (DIN for WS2812B)
    uint8_t clock_pin;    ///< Clock pin (CK for APA102, unused for WS2812B)
    uint16_t ledCount;    ///< Number of LEDs in strip
    
    uint8_t brightness;   ///< Global brightness (0-255)
    uint8_t colorOrder;   ///< Color order (0=RGB, 1=GRB, 2=BGR, etc.)
    uint8_t _pad[2];
    
    LEDStripConfig()
        : data_pin(6), clock_pin(255), ledCount(60),
          brightness(128), colorOrder(1) {} // Default: GRB (common for WS2812B)
} __attribute__((packed));

/**
 * @struct DisplayBackendData
 * @brief Serializable display backend configuration.
 *
 * Uses a tagged union to store different backend configurations compactly.
 * The loader will instantiate the appropriate IDisplayBackend implementation.
 */
struct DisplayBackendData {
    DisplayBackendType type;
    uint8_t cameraIndex;  ///< Which camera routes to this display
    uint16_t width;       ///< Display width in pixels
    uint16_t height;      ///< Display height in pixels
    
    // Simplified: Flat fields instead of union to avoid non-trivial constructor issues
    // HUB75 config
    uint8_t r1_pin, g1_pin, b1_pin, r2_pin, g2_pin, b2_pin;
    uint8_t a_pin, b_pin, c_pin, d_pin, e_pin;
    uint8_t clk_pin, lat_pin, oe_pin;
    uint8_t hub75_brightness;
    
    // SPI config
    uint8_t spi_cs_pin, spi_dc_pin, spi_rst_pin;
    uint32_t spi_frequency;
    
    // I2C config
    uint8_t i2c_address;
    uint8_t i2c_sda_pin, i2c_scl_pin;
    uint32_t i2c_frequency;
    
    // LED Strip config
    uint8_t led_data_pin;
    uint16_t led_count;
    uint8_t led_brightness;
    
    // Padding for future expansion
    uint8_t _padding[64];
    
    DisplayBackendData() 
        : type(DisplayBackendType::HUB75), cameraIndex(0), 
          width(64), height(32),
          r1_pin(0), g1_pin(0), b1_pin(0),
          r2_pin(0), g2_pin(0), b2_pin(0),
          a_pin(0), b_pin(0), c_pin(0), d_pin(0), e_pin(0),
          clk_pin(0), lat_pin(0), oe_pin(0),
          hub75_brightness(128),
          spi_cs_pin(0), spi_dc_pin(0), spi_rst_pin(0),
          spi_frequency(0),
          i2c_address(0), i2c_sda_pin(0), i2c_scl_pin(0),
          i2c_frequency(0),
          led_data_pin(0), led_count(0), led_brightness(128) {
        memset(_padding, 0, sizeof(_padding));
    }
} __attribute__((packed));

//==============================================================================
// Animation Data (Future)
//==============================================================================

/**
 * @struct KeyframeData
 * @brief Serializable keyframe for animation tracks.
 */
struct KeyframeData {
    float time;           ///< Time in seconds
    float value;          ///< Parameter value at this time
    uint8_t interpolation; ///< Interpolation type (0=linear, 1=cosine, 2=overshoot)
    uint8_t _pad[3];
    
    KeyframeData() : time(0.0f), value(0.0f), interpolation(0) {}
} __attribute__((packed));

/**
 * @struct AnimationTrackData
 * @brief Serializable animation track.
 */
struct AnimationTrackData {
    uint32_t targetMeshId;      ///< Index of target mesh
    uint32_t targetParameter;   ///< Parameter ID (0=pos.x, 1=pos.y, etc.)
    uint32_t keyframeCount;
    uint32_t keyframesOffset;   ///< Offset to keyframe array
    uint8_t loop;               ///< Loop animation (1=yes, 0=no)
    uint8_t _pad[3];
    
    AnimationTrackData() 
        : targetMeshId(0), targetParameter(0), 
          keyframeCount(0), keyframesOffset(0), loop(0) {}
} __attribute__((packed));

//==============================================================================
// Complete Scene Definition
//==============================================================================

/**
 * @struct CompiledSceneHeader
 * @brief Header for compiled scene binary file.
 *
 * Binary file layout:
 * 1. CompiledSceneHeader
 * 2. Array of TriangleGroupData
 * 3. Vertex data (all geometries)
 * 4. Index data (all geometries)
 * 5. Array of MaterialData
 * 6. Array of MeshData
 * 7. Array of CameraData
 * 8. Array of DisplayBackendData
 * 9. Array of AnimationTrackData (optional)
 * 10. Keyframe data (optional)
 * 11. Image/texture data (optional)
 * 12. String data (optional)
 */
struct CompiledSceneHeader {
    // === File Identification ===
    uint32_t magic;        ///< Magic number (KOILOSCENE_MAGIC)
    uint32_t version;      ///< Format version (KOILOSCENE_VERSION)
    char name[64];         ///< Scene name (null-terminated)
    
    // === Array Counts ===
    uint32_t geometryCount;
    uint32_t materialCount;
    uint32_t meshCount;
    uint32_t cameraCount;
    uint32_t displayCount;
    uint32_t animationTrackCount; // Optional (0 if no animations)
    
    // === Data Section Offsets ===
    // All offsets are from start of file (byte 0)
    uint32_t geometriesOffset;
    uint32_t vertexDataOffset;    ///< Start of vertex arrays
    uint32_t indexDataOffset;     ///< Start of index arrays
    uint32_t materialsOffset;
    uint32_t meshesOffset;
    uint32_t camerasOffset;
    uint32_t displaysOffset;
    uint32_t animationsOffset;    // 0 if no animations
    uint32_t imageDataOffset;     // 0 if no images
    uint32_t stringDataOffset;    // 0 if no strings
    uint32_t scriptDataOffset;    ///< Offset to embedded script text (0 if none)
    
    // === Data Section Sizes ===
    uint32_t vertexDataSize;      ///< Total bytes of vertex data
    uint32_t indexDataSize;       ///< Total bytes of index data
    uint32_t imageDataSize;       ///< Total bytes of image data
    uint32_t stringDataSize;      ///< Total bytes of string data
    uint32_t scriptDataSize;      ///< Total bytes of script data
    
    // === Metadata ===
    uint32_t totalFileSize;       ///< Total file size in bytes
    uint32_t crc32;               ///< CRC32 checksum (optional validation)
    uint8_t scriptType;           ///< 0=None, 1=Lua, 2=Python (future), 3=JavaScript (future)
    uint8_t scriptVersion;        ///< Script API version (for compatibility)
    uint8_t _scriptPad[2];        ///< Alignment padding
    
    CompiledSceneHeader() 
        : magic(KOILOSCENE_MAGIC), version(KOILOSCENE_VERSION),
          geometryCount(0), materialCount(0), meshCount(0), 
          cameraCount(0), displayCount(0), animationTrackCount(0),
          geometriesOffset(0), vertexDataOffset(0), indexDataOffset(0),
          materialsOffset(0), meshesOffset(0), camerasOffset(0),
          displaysOffset(0), animationsOffset(0), imageDataOffset(0),
          stringDataOffset(0), scriptDataOffset(0),
          vertexDataSize(0), indexDataSize(0),
          imageDataSize(0), stringDataSize(0), scriptDataSize(0),
          totalFileSize(0), crc32(0),
          scriptType(0), scriptVersion(1) {
        name[0] = '\0';
        memset(_scriptPad, 0, sizeof(_scriptPad));
    }
} __attribute__((packed));

//==============================================================================
// Runtime Scene Container
//==============================================================================

/**
 * @struct CompiledScene
 * @brief Runtime container for compiled scene data.
 *
 * This structure holds pointers to scene data that has been loaded from
 * a binary file or flash memory. The loader resolves offsets to actual
 * pointers for runtime access.
 *
 * Memory ownership:
 * - Header: Owned by CompiledScene
 * - Arrays: Point into loaded data buffer (not owned)
 * - Data buffer: Managed by loader (PROGMEM or heap)
 */
struct CompiledScene {
    CompiledSceneHeader header;
    
    // Resolved pointers (NULL if not present)
    const TriangleGroupData* geometries;
    const uint8_t* vertexData;        ///< Raw vertex array bytes
    const uint8_t* indexData;         ///< Raw index array bytes
    const MaterialData* materials;
    const MeshData* meshes;
    const CameraData* cameras;
    const DisplayBackendData* displays;
    const AnimationTrackData* animations;
    const uint8_t* imageData;
    const uint8_t* stringData;
    const uint8_t* scriptData;        ///< Pointer to embedded script text (or nullptr)
    
    CompiledScene() 
        : geometries(nullptr), vertexData(nullptr), indexData(nullptr),
          materials(nullptr), meshes(nullptr), cameras(nullptr),
          displays(nullptr), animations(nullptr), imageData(nullptr),
          stringData(nullptr), scriptData(nullptr) {}
};

} // namespace koilo
