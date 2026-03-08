// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/display/displaymanager.hpp>
#include <koilo/systems/render/core/ipixelgroup.hpp>
#include <koilo/systems/render/core/pixelgroup.hpp>
#include <iostream>

namespace koilo {

koilo::DisplayManager::DisplayManager()
    : nextDisplayId_(0), vsyncEnabled_(false), autoPresent_(true) {
}

koilo::DisplayManager::~DisplayManager() {
    // Shutdown all displays
    for (auto& [id, entry] : displays_) {
        if (entry.backend) {
            entry.backend->Shutdown();
        }
    }
    displays_.clear();
    cameraToDisplay_.clear();
}

int koilo::DisplayManager::AddDisplay(std::unique_ptr<IDisplayBackend> backend) {
    if (!backend) {
        return -1;
    }
    
    // Initialize backend
    if (!backend->Initialize()) {
        return -1;
    }
    
    int displayId = nextDisplayId_++;
    
    DisplayEntry entry;
    entry.backend = std::move(backend);
    entry.camera = nullptr;
    
    displays_[displayId] = std::move(entry);
    
    return displayId;
}

int koilo::DisplayManager::AddDisplayRaw(IDisplayBackend* backend) {
    return AddDisplay(std::unique_ptr<IDisplayBackend>(backend));
}

bool koilo::DisplayManager::RemoveDisplay(int displayId) {
    auto it = displays_.find(displayId);
    if (it == displays_.end()) {
        return false;
    }
    
    // Unroute camera if any
    if (it->second.camera) {
        UnrouteCamera(it->second.camera);
    }
    
    // Shutdown backend
    if (it->second.backend) {
        it->second.backend->Shutdown();
    }
    
    displays_.erase(it);
    return true;
}

IDisplayBackend* koilo::DisplayManager::GetDisplay(int displayId) {
    auto it = displays_.find(displayId);
    if (it == displays_.end()) {
        return nullptr;
    }
    return it->second.backend.get();
}

int koilo::DisplayManager::GetDisplayCount() const {
    return static_cast<int>(displays_.size());
}

std::vector<int> koilo::DisplayManager::GetDisplayIds() const {
    std::vector<int> ids;
    ids.reserve(displays_.size());
    for (const auto& [id, entry] : displays_) {
        ids.push_back(id);
    }
    return ids;
}

bool koilo::DisplayManager::RouteCamera(CameraBase* camera, int displayId) {
    if (!camera) {
        return false;
    }
    
    auto it = displays_.find(displayId);
    if (it == displays_.end()) {
        return false;
    }
    
    // Unroute from previous display if any
    auto prevIt = cameraToDisplay_.find(camera);
    if (prevIt != cameraToDisplay_.end()) {
        auto prevDisplay = displays_.find(prevIt->second);
        if (prevDisplay != displays_.end()) {
            prevDisplay->second.camera = nullptr;
        }
    }
    
    // Route to new display
    it->second.camera = camera;
    cameraToDisplay_[camera] = displayId;
    
    return true;
}

bool koilo::DisplayManager::UnrouteCamera(CameraBase* camera) {
    if (!camera) {
        return false;
    }
    
    auto it = cameraToDisplay_.find(camera);
    if (it == cameraToDisplay_.end()) {
        return false;
    }
    
    int displayId = it->second;
    auto displayIt = displays_.find(displayId);
    if (displayIt != displays_.end()) {
        displayIt->second.camera = nullptr;
    }
    
    cameraToDisplay_.erase(it);
    return true;
}

int koilo::DisplayManager::GetCameraDisplay(CameraBase* camera) const {
    auto it = cameraToDisplay_.find(camera);
    if (it == cameraToDisplay_.end()) {
        return -1;
    }
    return it->second;
}

CameraBase* koilo::DisplayManager::GetDisplayCamera(int displayId) const {
    auto it = displays_.find(displayId);
    if (it == displays_.end()) {
        return nullptr;
    }
    return it->second.camera;
}

int koilo::DisplayManager::PresentAll() {
    int successCount = 0;
    
    for (auto& [id, entry] : displays_) {
        if (Present(id)) {
            ++successCount;
        }
    }
    
    return successCount;
}

bool koilo::DisplayManager::Present(int displayId) {
    auto it = displays_.find(displayId);
    if (it == displays_.end()) {
        return false;
    }
    
    DisplayEntry& entry = it->second;
    
    // Check if we have a camera and backend
    if (!entry.camera || !entry.backend) {
        return false;
    }
    
    // Build framebuffer from camera
    if (!BuildFramebuffer(entry)) {
        return false;
    }
    
    // Present to backend
    bool result = entry.backend->Present(entry.framebuffer);
    
    // Wait for VSync if enabled
    if (result && vsyncEnabled_) {
        entry.backend->WaitVSync();
    }
    
    return result;
}

void koilo::DisplayManager::ClearAll() {
    for (auto& [id, entry] : displays_) {
        if (entry.backend) {
            entry.backend->Clear();
        }
    }
}

bool koilo::DisplayManager::Clear(int displayId) {
    auto it = displays_.find(displayId);
    if (it == displays_.end()) {
        return false;
    }
    
    if (it->second.backend) {
        return it->second.backend->Clear();
    }
    
    return false;
}

void koilo::DisplayManager::SetVSyncEnabled(bool enabled) {
    vsyncEnabled_ = enabled;
    
    // Apply to all backends
    for (auto& [id, entry] : displays_) {
        if (entry.backend) {
            entry.backend->SetVSyncEnabled(enabled);
        }
    }
}

void koilo::DisplayManager::SetAutoPresent(bool enabled) {
    autoPresent_ = enabled;
}

bool koilo::DisplayManager::BuildFramebuffer(DisplayEntry& entry) {
    CameraBase* camera = entry.camera;
    if (!camera) {
        return false;
    }
    
    // Get pixel group from camera
    IPixelGroup* pixelGroup = camera->GetPixelGroup();
    if (!pixelGroup) {
        return false;
    }
    
    const uint32_t pixelCount = pixelGroup->GetPixelCount();
    if (pixelCount == 0) {
        return false;
    }

    Color888* colors = pixelGroup->GetColors();
    if (!colors) {
        return false;
    }

    // Check if this is a rectangular PixelGroup (e.g., for 2D displays)
    PixelGroup* rectPixelGroup = dynamic_cast<PixelGroup*>(pixelGroup);
    bool isRectangular = rectPixelGroup && rectPixelGroup->IsRectangular();
    
    if (isRectangular) {
        // === 2D Framebuffer Mode (for SDL2, OpenGL, etc.) ===
        // PixelGroup uses: rowCount = WIDTH, colCount = HEIGHT
        uint32_t width = rectPixelGroup->GetRowCount();     // Width (pixels per row)
        uint32_t height = rectPixelGroup->GetColumnCount(); // Height (number of rows)
        
        if (width == 0 || height == 0 || width * height != pixelCount) {
            // Fall back to 1D mode if dimensions are invalid
            isRectangular = false;
        } else {
            // Create 2D framebuffer with RGBA format (SDL2/GPU prefer 4-byte aligned pixels)
            entry.pixelBuffer.resize(static_cast<std::size_t>(pixelCount) * 4);
            
            // Debug output
            std::cout << "[DisplayManager] Creating 2D framebuffer: " << width << "x" << height << "\n";
            
            // Copy pixels row by row with RGBA format
            for (uint32_t y = 0; y < height; ++y) {
                for (uint32_t x = 0; x < width; ++x) {
                    uint32_t srcIdx = y * width + x;  // Source index in 1D array
                    uint32_t dstIdx = y * width + x;  // Destination index
                    const std::size_t base = static_cast<std::size_t>(dstIdx) * 4;
                    
                    const Color888& c = colors[srcIdx];
                    entry.pixelBuffer[base + 0] = c.r;  // R
                    entry.pixelBuffer[base + 1] = c.g;  // G
                    entry.pixelBuffer[base + 2] = c.b;  // B
                    entry.pixelBuffer[base + 3] = 255;  // A (opaque)
                }
            }
            
            entry.framebuffer.data = entry.pixelBuffer.data();
            entry.framebuffer.width = width;
            entry.framebuffer.height = height;
            entry.framebuffer.format = PixelFormat::RGBA8888;  // Use RGBA for better SDL2 compatibility
            entry.framebuffer.stride = width * 4;
        }
    }
    
    if (!isRectangular) {
        // === 1D Framebuffer Mode (for LED strips, matrices, etc.) ===
        // Original behavior: treat as single row
        entry.pixelBuffer.resize(static_cast<std::size_t>(pixelCount) * 3);
        
        for (uint32_t i = 0; i < pixelCount; ++i) {
            const Color888& c = colors[i];
            const std::size_t base = static_cast<std::size_t>(i) * 3;
            entry.pixelBuffer[base + 0] = c.r;
            entry.pixelBuffer[base + 1] = c.g;
            entry.pixelBuffer[base + 2] = c.b;
        }
        
        entry.framebuffer.data = entry.pixelBuffer.data();
        entry.framebuffer.width = pixelCount;
        entry.framebuffer.height = 1;
        entry.framebuffer.format = PixelFormat::RGB888;
        entry.framebuffer.stride = pixelCount * 3;
    }

    // Update geometry buffer (XY pairs) - used by geometry-aware backends
    entry.geometryBuffer.resize(static_cast<std::size_t>(pixelCount) * 2);
    for (uint32_t i = 0; i < pixelCount; ++i) {
        Vector2D coord = pixelGroup->GetCoordinate(static_cast<uint16_t>(i));
        const std::size_t base = static_cast<std::size_t>(i) * 2;
        entry.geometryBuffer[base + 0] = coord.X;
        entry.geometryBuffer[base + 1] = coord.Y;
    }
    
    if (auto* geomBackend = dynamic_cast<IGeometryDisplayBackend*>(entry.backend.get())) {
        geomBackend->UpdateGeometry(entry.geometryBuffer.data(), pixelCount);
    }
    
    return entry.framebuffer.data != nullptr;
}

} // namespace koilo
