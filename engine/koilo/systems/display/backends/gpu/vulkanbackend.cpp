// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file vulkanbackend.cpp
 * @brief Vulkan display backend implementation.
 */

#include "vulkanbackend.hpp"
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <koilo/kernel/logging/log.hpp>
#include <cstring>
#include <algorithm>
#include <limits>

namespace koilo {

// ============================================================================
// Validation layer callback
// ============================================================================

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT /*type*/,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* /*user*/)
{
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        KL_ERR("Vulkan", "%s", data->pMessage);
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        KL_WARN("Vulkan", "%s", data->pMessage);
    else
        KL_LOG("Vulkan", "%s", data->pMessage);
    return VK_FALSE;
}

static const char* kValidationLayer = "VK_LAYER_KHRONOS_validation";

// ============================================================================
// Constructor / Destructor
// ============================================================================

VulkanBackend::VulkanBackend(uint32_t width, uint32_t height,
                             const std::string& title,
                             bool fullscreen, bool resizable)
    : windowWidth_(width), windowHeight_(height),
      title_(title), fullscreen_(fullscreen), resizable_(resizable)
{
    info_.name       = "Vulkan";
    info_.width      = width;
    info_.height     = height;
}

VulkanBackend::~VulkanBackend() {
    if (initialized_) Shutdown();
}

// ============================================================================
// IDisplayBackend Implementation
// ============================================================================

bool VulkanBackend::Initialize() {
    if (initialized_) return true;

    // Initialize SDL video
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            KL_ERR("VulkanBackend", "SDL_Init failed: %s", SDL_GetError());
            return false;
        }
    }

    // Load Vulkan library through SDL
    if (!SDL_Vulkan_LoadLibrary(nullptr)) {
        KL_ERR("VulkanBackend", "SDL_Vulkan_LoadLibrary failed: %s", SDL_GetError());
        return false;
    }

    // Create window with Vulkan flag
    Uint32 flags = SDL_WINDOW_VULKAN;
    if (resizable_)   flags |= SDL_WINDOW_RESIZABLE;
    if (fullscreen_)  flags |= SDL_WINDOW_FULLSCREEN;

    window_ = SDL_CreateWindow(title_.c_str(),
                               static_cast<int>(windowWidth_),
                               static_cast<int>(windowHeight_),
                               flags);
    if (!window_) {
        KL_ERR("VulkanBackend", "SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }

    if (!CreateInstance())       { Shutdown(); return false; }
    if (!SetupDebugMessenger())  { /* non-fatal */ }
    if (!CreateSurface())       { Shutdown(); return false; }
    if (!EnumerateGPUs())       { Shutdown(); return false; }

    // Auto-select best GPU (prefer discrete)
    uint32_t bestIdx = 0;
    for (uint32_t i = 0; i < gpuInfos_.size(); i++) {
        if (gpuInfos_[i].isDiscrete) { bestIdx = i; break; }
    }
    if (!SelectGPU(bestIdx))       { Shutdown(); return false; }
    if (!CreateLogicalDevice())    { Shutdown(); return false; }
    if (!CreateSwapchain())        { Shutdown(); return false; }
    if (!CreateImageViews())       { Shutdown(); return false; }
    if (!CreateRenderPass())       { Shutdown(); return false; }
    if (!CreateFramebuffers())     { Shutdown(); return false; }
    if (!CreateCommandPool())      { Shutdown(); return false; }
    if (!CreateCommandBuffers())   { Shutdown(); return false; }
    if (!CreateSyncObjects())      { Shutdown(); return false; }

    initialized_ = true;

    KL_LOG("VulkanBackend", "GPU: %s", gpuInfos_[selectedGPU_].name.c_str());
    KL_LOG("VulkanBackend", "Swapchain: %ux%u", swapExtentW_, swapExtentH_);
    KL_LOG("VulkanBackend", "Initialized successfully.");
    return true;
}

void VulkanBackend::Shutdown() {
    if (device_) vkDeviceWaitIdle(device_);

    DestroyStagingBuffer();
    CleanupSwapchain();

    if (device_) {
        for (auto sem : renderFinishedSems_) {
            if (sem) vkDestroySemaphore(device_, sem, nullptr);
        }
        renderFinishedSems_.clear();
        for (int i = 0; i < kMaxFramesInFlight; i++) {
            if (imageAvailableSems_[i]) vkDestroySemaphore(device_, imageAvailableSems_[i], nullptr);
            if (inFlightFences_[i])     vkDestroyFence(device_, inFlightFences_[i], nullptr);
            imageAvailableSems_[i] = nullptr;
            inFlightFences_[i]     = nullptr;
        }
        if (commandPool_) { vkDestroyCommandPool(device_, commandPool_, nullptr); commandPool_ = nullptr; }
        if (renderPass_)  { vkDestroyRenderPass(device_, renderPass_, nullptr); renderPass_ = nullptr; }
        vkDestroyDevice(device_, nullptr);
        device_ = nullptr;
    }

    if (surface_ && instance_) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = nullptr;
    }

    if (debugMessenger_ && instance_) {
        auto destroyFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT");
        if (destroyFunc) destroyFunc(instance_, debugMessenger_, nullptr);
        debugMessenger_ = nullptr;
    }

    if (instance_) { vkDestroyInstance(instance_, nullptr); instance_ = nullptr; }
    if (window_)   { SDL_DestroyWindow(window_); window_ = nullptr; }

    SDL_Vulkan_UnloadLibrary();
    initialized_ = false;
}

bool VulkanBackend::IsInitialized() const { return initialized_; }

DisplayInfo VulkanBackend::GetInfo() const { return info_; }

bool VulkanBackend::HasCapability(DisplayCapability cap) const {
    switch (cap) {
        case DisplayCapability::VSync:
        case DisplayCapability::GPUAcceleration:
        case DisplayCapability::DoubleBuffering:
        case DisplayCapability::RGB888:
            return true;
        default:
            return false;
    }
}

bool VulkanBackend::Present(const Framebuffer& fb) {
    if (!PresentNoSwap(fb)) return false;
    SwapOnly();
    return true;
}

void VulkanBackend::WaitVSync() {
    // VSync is managed by swapchain present mode
}

bool VulkanBackend::Clear() {
    if (!initialized_) return false;
    auto cmd = BeginFrame();
    if (!cmd) return false;
    EndFrame();
    SwapOnly();
    return true;
}

bool VulkanBackend::SetRefreshRate(uint32_t /*hz*/) { return false; }
bool VulkanBackend::SetOrientation(Orientation /*orient*/) { return false; }
bool VulkanBackend::SetBrightness(uint8_t /*brightness*/) { return false; }

bool VulkanBackend::SetVSyncEnabled(bool enabled) {
    if (vsyncEnabled_ == enabled) return true;
    vsyncEnabled_ = enabled;
    if (initialized_) RecreateSwapchain();
    return true;
}

// ============================================================================
// IGPUDisplayBackend Implementation
// ============================================================================

void VulkanBackend::SwapOnly() {
    if (!initialized_ || !frameActive_) return;

    // End swapchain render pass if it was started
    if (swapRenderPassActive_) {
        vkCmdEndRenderPass(commandBuffers_[currentFrame_]);
        swapRenderPassActive_ = false;
    }

    // End command buffer + submit
    VkResult endResult = vkEndCommandBuffer(commandBuffers_[currentFrame_]);
    if (endResult != VK_SUCCESS) {
        KL_ERR("VulkanBackend", "vkEndCommandBuffer failed: %d (frame %d)",
               (int)endResult, currentFrame_);
    }

    // Submit
    VkSemaphore waitSems[]   = { imageAvailableSems_[currentFrame_] };
    VkPipelineStageFlags waitStages[] = {
        transferOnlyFrame_ ? VK_PIPELINE_STAGE_TRANSFER_BIT
                           : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    VkSemaphore signalSems[] = { renderFinishedSems_[imageIndex_] };
    transferOnlyFrame_ = false;

    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSems;
    submitInfo.pWaitDstStageMask    = waitStages;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &commandBuffers_[currentFrame_];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSems;

    VkResult submitResult = vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]);
    if (submitResult != VK_SUCCESS) {
        KL_ERR("VulkanBackend", "SwapOnly vkQueueSubmit failed: %d (frame %d)",
               (int)submitResult, currentFrame_);
    }

    // Present
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSems;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &swapchain_;
    presentInfo.pImageIndices      = &imageIndex_;

    VkResult result = vkQueuePresentKHR(presentQueue_, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized_) {
        framebufferResized_ = false;
        RecreateSwapchain();
    }

    frameActive_ = false;
    currentFrame_ = (currentFrame_ + 1) % kMaxFramesInFlight;
}

bool VulkanBackend::PresentNoSwap(const Framebuffer& fb) {
    if (!initialized_) return false;

    // Scale CPU framebuffer to swapchain size (nearest-neighbor)
    uint32_t dstW = swapExtentW_;
    uint32_t dstH = swapExtentH_;
    size_t dataSize = static_cast<size_t>(dstW) * dstH * 4;
    if (!stagingBuffer_ || stagingSize_ < dataSize) {
        DestroyStagingBuffer();
        if (!CreateStagingBuffer(dataSize)) return false;
    }

    const uint8_t* src = static_cast<const uint8_t*>(fb.data);
    auto* dst = static_cast<uint8_t*>(stagingMapped_);
    int srcW = fb.width;
    int srcH = fb.height;

    for (uint32_t y = 0; y < dstH; y++) {
        int sy = static_cast<int>(y) * srcH / static_cast<int>(dstH);
        if (sy >= srcH) sy = srcH - 1;
        for (uint32_t x = 0; x < dstW; x++) {
            int sx = static_cast<int>(x) * srcW / static_cast<int>(dstW);
            if (sx >= srcW) sx = srcW - 1;
            int di = (y * dstW + x) * 4;
            if (fb.format == PixelFormat::RGB888) {
                int si = (sy * srcW + sx) * 3;
                dst[di + 0] = src[si + 2]; // B
                dst[di + 1] = src[si + 1]; // G
                dst[di + 2] = src[si + 0]; // R
                dst[di + 3] = 255;
            } else {
                int si = (sy * srcW + sx) * 4;
                dst[di + 0] = src[si + 2]; // B
                dst[di + 1] = src[si + 1]; // G
                dst[di + 2] = src[si + 0]; // R
                dst[di + 3] = src[si + 3]; // A
            }
        }
    }

    auto cmd = BeginFrame();
    if (!cmd) return false;

    transferOnlyFrame_ = true;
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = swapImages_[imageIndex_];
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Copy staging buffer to swapchain image (already scaled to swapchain size)
    VkBufferImageCopy region{};
    region.bufferRowLength = dstW;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {dstW, dstH, 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer_, swapImages_[imageIndex_],
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition: TRANSFER_DST -> PRESENT_SRC
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = 0;
    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    return true;
}

void VulkanBackend::SetNearestFiltering(bool nearest) {
    nearestFiltering_ = nearest;
}

void VulkanBackend::PrepareDefaultFramebuffer(int /*width*/, int /*height*/) {
    // In Vulkan, the "default framebuffer" is the swapchain framebuffer.
    // BeginFrame() handles acquiring the image and beginning the render pass.
    // This is a no-op since the render pass clear already handles it.
    if (!frameActive_) {
        BeginFrame();
    }
}

// ============================================================================
// Frame Management
// ============================================================================

VkCommandBuffer VulkanBackend::BeginFrame() {
    if (frameActive_) return commandBuffers_[currentFrame_];
    if (!initialized_) return nullptr;

    // Wait for previous frame using this slot to finish
    vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

    // Acquire next swapchain image
    VkResult result = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
                                             imageAvailableSems_[currentFrame_],
                                             VK_NULL_HANDLE, &imageIndex_);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // Image was NOT acquired, semaphore was NOT signaled - safe to recreate
        RecreateSwapchain();
        return nullptr;
    }
    if (result == VK_SUBOPTIMAL_KHR) {
        // Image WAS acquired, semaphore WAS signaled - use image normally,
        // defer recreation to SwapOnly after submitting the frame
        framebufferResized_ = true;
    }

    vkResetFences(device_, 1, &inFlightFences_[currentFrame_]);

    // Reset and begin command buffer
    vkResetCommandBuffer(commandBuffers_[currentFrame_], 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffers_[currentFrame_], &beginInfo);

    frameActive_ = true;
    swapRenderPassActive_ = false;
    return commandBuffers_[currentFrame_];
}

void VulkanBackend::BeginSwapchainRenderPass() {
    if (!frameActive_ || swapRenderPassActive_) return;
    if (imageIndex_ >= static_cast<uint32_t>(swapFramebuffers_.size())) return;

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass        = renderPass_;
    rpInfo.framebuffer       = swapFramebuffers_[imageIndex_];
    rpInfo.renderArea.offset = {0, 0};
    rpInfo.renderArea.extent = {swapExtentW_, swapExtentH_};

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues    = &clearColor;

    vkCmdBeginRenderPass(commandBuffers_[currentFrame_], &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport and scissor
    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(swapExtentW_);
    viewport.height   = static_cast<float>(swapExtentH_);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffers_[currentFrame_], 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {swapExtentW_, swapExtentH_};
    vkCmdSetScissor(commandBuffers_[currentFrame_], 0, 1, &scissor);

    swapRenderPassActive_ = true;
}

void VulkanBackend::EndFrame() {
    // EndFrame is separate from SwapOnly so the render backend can
    // record additional commands between BeginFrame and SwapOnly.
    // The actual render pass end + submit happens in SwapOnly().
}

// ============================================================================
// Swapchain Management
// ============================================================================

bool VulkanBackend::RecreateSwapchain() {
    // Handle minimization
    int w = 0, h = 0;
    SDL_GetWindowSize(window_, &w, &h);
    while (w == 0 || h == 0) {
        SDL_WaitEvent(nullptr);
        SDL_GetWindowSize(window_, &w, &h);
    }

    vkDeviceWaitIdle(device_);

    // Invalidate current frame state - old swapchain resources are gone
    frameActive_ = false;
    swapRenderPassActive_ = false;
    imageIndex_ = 0;

    CleanupSwapchain();

    if (!CreateSwapchain())    return false;
    if (!CreateImageViews())   return false;
    if (!CreateFramebuffers()) return false;

    // Recreate per-image renderFinished semaphores if image count changed
    uint32_t newCount = static_cast<uint32_t>(swapImages_.size());
    if (renderFinishedSems_.size() != newCount) {
        for (auto sem : renderFinishedSems_) {
            if (sem) vkDestroySemaphore(device_, sem, nullptr);
        }
        renderFinishedSems_.resize(newCount, VK_NULL_HANDLE);
        VkSemaphoreCreateInfo semInfo{};
        semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        for (uint32_t i = 0; i < newCount; i++) {
            vkCreateSemaphore(device_, &semInfo, nullptr, &renderFinishedSems_[i]);
        }
    }

    return true;
}

void VulkanBackend::CleanupSwapchain() {
    if (!device_) return;
    for (auto fb : swapFramebuffers_) {
        if (fb) vkDestroyFramebuffer(device_, fb, nullptr);
    }
    swapFramebuffers_.clear();
    for (auto iv : swapImageViews_) {
        if (iv) vkDestroyImageView(device_, iv, nullptr);
    }
    swapImageViews_.clear();
    if (swapchain_) {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        swapchain_ = nullptr;
    }
}

// ============================================================================
// Vulkan Instance
// ============================================================================

bool VulkanBackend::CreateInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = title_.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 3, 2);
    appInfo.pEngineName        = "Koilo Engine";
    appInfo.engineVersion      = VK_MAKE_VERSION(0, 3, 2);
    appInfo.apiVersion         = VK_API_VERSION_1_1;

    // Get required extensions from SDL
    Uint32 sdlExtCount = 0;
    const char* const* sdlExts = SDL_Vulkan_GetInstanceExtensions(&sdlExtCount);
    if (!sdlExts) {
        KL_ERR("VulkanBackend", "SDL_Vulkan_GetInstanceExtensions failed");
        return false;
    }

    std::vector<const char*> extensions(sdlExts, sdlExts + sdlExtCount);

    // Check for validation layer support
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

    for (const auto& layer : layers) {
        if (strcmp(layer.layerName, kValidationLayer) == 0) {
            validationEnabled_ = true;
            break;
        }
    }

    if (validationEnabled_) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkInstanceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    const char* validationLayers[] = { kValidationLayer };
    if (validationEnabled_) {
        createInfo.enabledLayerCount   = 1;
        createInfo.ppEnabledLayerNames = validationLayers;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance_);
    if (result != VK_SUCCESS) {
        KL_ERR("VulkanBackend", "vkCreateInstance failed: %d", result);
        return false;
    }
    return true;
}

bool VulkanBackend::SetupDebugMessenger() {
    if (!validationEnabled_) return true;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = VulkanDebugCallback;

    auto createFunc = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT");
    if (createFunc) {
        createFunc(instance_, &createInfo, nullptr, &debugMessenger_);
    }
    return true;
}

// ============================================================================
// Surface
// ============================================================================

bool VulkanBackend::CreateSurface() {
    if (!SDL_Vulkan_CreateSurface(window_, instance_, nullptr, &surface_)) {
        KL_ERR("VulkanBackend", "SDL_Vulkan_CreateSurface failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

// ============================================================================
// GPU Enumeration & Selection
// ============================================================================

bool VulkanBackend::EnumerateGPUs() {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance_, &count, nullptr);
    if (count == 0) {
        KL_ERR("VulkanBackend", "No Vulkan-capable GPUs found");
        return false;
    }

    physicalDevices_.resize(count);
    vkEnumeratePhysicalDevices(instance_, &count, physicalDevices_.data());

    gpuInfos_.clear();
    for (uint32_t i = 0; i < count; i++) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(physicalDevices_[i], &props);

        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(physicalDevices_[i], &memProps);

        uint64_t vram = 0;
        for (uint32_t j = 0; j < memProps.memoryHeapCount; j++) {
            if (memProps.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                vram = std::max(vram, memProps.memoryHeaps[j].size);
            }
        }

        VulkanGPUInfo info;
        info.index      = i;
        info.name       = props.deviceName;
        info.vendorID   = props.vendorID;
        info.deviceID   = props.deviceID;
        info.isDiscrete = (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
        info.vramBytes  = vram;
        gpuInfos_.push_back(info);

        const char* type = info.isDiscrete ? "discrete" : "integrated";
        KL_LOG("VulkanBackend", "GPU %u: %s (%s, VRAM: %lu MB)",
               i, info.name.c_str(), type, static_cast<unsigned long>(vram / (1024*1024)));
    }

    return true;
}

bool VulkanBackend::SelectGPU(uint32_t index) {
    if (index >= physicalDevices_.size()) return false;
    physicalDevice_ = physicalDevices_[index];
    selectedGPU_ = index;

    // Find queue families
    uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &familyCount, families.data());

    bool foundGraphics = false, foundPresent = false;
    for (uint32_t i = 0; i < familyCount; i++) {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsFamily_ = i;
            foundGraphics = true;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice_, i, surface_, &presentSupport);
        if (presentSupport) {
            presentFamily_ = i;
            foundPresent = true;
        }

        if (foundGraphics && foundPresent) break;
    }

    if (!foundGraphics || !foundPresent) {
        KL_ERR("VulkanBackend", "GPU %u lacks required queue families", index);
        return false;
    }

    return true;
}

// ============================================================================
// Logical Device
// ============================================================================

bool VulkanBackend::CreateLogicalDevice() {
    // Queue create infos (may be same family for graphics+present)
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    float priority = 1.0f;

    std::vector<uint32_t> uniqueFamilies = { graphicsFamily_ };
    if (presentFamily_ != graphicsFamily_) uniqueFamilies.push_back(presentFamily_);

    for (uint32_t family : uniqueFamilies) {
        VkDeviceQueueCreateInfo qi{};
        qi.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qi.queueFamilyIndex = family;
        qi.queueCount       = 1;
        qi.pQueuePriorities = &priority;
        queueInfos.push_back(qi);
    }

    VkPhysicalDeviceFeatures features{};

    // Check for mutable format extension support (avoids double-gamma with sRGB swapchain)
    uint32_t extCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice_, nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> availableExts(extCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice_, nullptr, &extCount, availableExts.data());

    mutableFormatSupported_ = false;
    bool hasImageFormatList = false;
    bool hasMaintenance2 = false;
    for (const auto& ext : availableExts) {
        if (strcmp(ext.extensionName, VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME) == 0)
            mutableFormatSupported_ = true;
        if (strcmp(ext.extensionName, VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME) == 0)
            hasImageFormatList = true;
        if (strcmp(ext.extensionName, VK_KHR_MAINTENANCE_2_EXTENSION_NAME) == 0)
            hasMaintenance2 = true;
    }
    // VK_KHR_swapchain_mutable_format requires VK_KHR_maintenance2 + VK_KHR_image_format_list
    mutableFormatSupported_ = mutableFormatSupported_ && hasImageFormatList && hasMaintenance2;

    std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    if (mutableFormatSupported_) {
        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_MAINTENANCE_2_EXTENSION_NAME);
    }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueInfos.size());
    createInfo.pQueueCreateInfos       = queueInfos.data();
    createInfo.pEnabledFeatures        = &features;
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VkResult result = vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_);
    if (result != VK_SUCCESS) {
        KL_ERR("VulkanBackend", "vkCreateDevice failed: %d", result);
        return false;
    }

    vkGetDeviceQueue(device_, graphicsFamily_, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, presentFamily_,  0, &presentQueue_);
    return true;
}

// ============================================================================
// Swapchain
// ============================================================================

bool VulkanBackend::CreateSwapchain() {
    // Query surface capabilities
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &caps);

    // Choose surface format (prefer BGRA8 SRGB)
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, formats.data());

    VkSurfaceFormatKHR chosen = formats[0];
    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosen = f;
            break;
        }
    }
    swapImageFormat_ = chosen.format;

    // Compute UNORM view format to bypass sRGB gamma encoding in the render pass.
    // This matches OpenGL behavior where GL_FRAMEBUFFER_SRGB is not enabled.
    swapViewFormat_ = swapImageFormat_;
    if (mutableFormatSupported_) {
        switch (static_cast<VkFormat>(swapImageFormat_)) {
            case VK_FORMAT_B8G8R8A8_SRGB: swapViewFormat_ = VK_FORMAT_B8G8R8A8_UNORM; break;
            case VK_FORMAT_R8G8B8A8_SRGB: swapViewFormat_ = VK_FORMAT_R8G8B8A8_UNORM; break;
            case VK_FORMAT_A8B8G8R8_SRGB_PACK32: swapViewFormat_ = VK_FORMAT_A8B8G8R8_UNORM_PACK32; break;
            default: break; // Already UNORM or unsupported - no conversion needed
        }
    }

    KL_LOG("VulkanBackend", "Swapchain format: %u, view format: %u (mutable: %s)",
           swapImageFormat_, swapViewFormat_,
           mutableFormatSupported_ ? "yes" : "no");

    // Choose present mode
    uint32_t modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &modeCount, nullptr);
    std::vector<VkPresentModeKHR> modes(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &modeCount, modes.data());

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // VSync (always available)
    if (!vsyncEnabled_) {
        for (auto m : modes) {
            if (m == VK_PRESENT_MODE_MAILBOX_KHR) { presentMode = m; break; }
            if (m == VK_PRESENT_MODE_IMMEDIATE_KHR) presentMode = m;
        }
    }

    // Choose extent
    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        swapExtentW_ = caps.currentExtent.width;
        swapExtentH_ = caps.currentExtent.height;
    } else {
        int w, h;
        SDL_GetWindowSizeInPixels(window_, &w, &h);
        swapExtentW_ = std::clamp(static_cast<uint32_t>(w),
                                   caps.minImageExtent.width, caps.maxImageExtent.width);
        swapExtentH_ = std::clamp(static_cast<uint32_t>(h),
                                   caps.minImageExtent.height, caps.maxImageExtent.height);
    }

    // Image count: request one more than min for triple buffering
    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
        imageCount = caps.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface          = surface_;
    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = chosen.format;
    createInfo.imageColorSpace  = chosen.colorSpace;
    createInfo.imageExtent      = { swapExtentW_, swapExtentH_ };
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                      VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    // Enable mutable format to create UNORM image views on sRGB swapchain images.
    // This avoids automatic sRGB gamma encoding, matching OpenGL behavior.
    VkImageFormatListCreateInfoKHR formatList{};
    VkFormat viewFormats[2] = {};
    if (mutableFormatSupported_ && swapViewFormat_ != swapImageFormat_) {
        createInfo.flags = VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR;

        viewFormats[0] = static_cast<VkFormat>(swapImageFormat_);
        viewFormats[1] = static_cast<VkFormat>(swapViewFormat_);
        formatList.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR;
        formatList.viewFormatCount = 2;
        formatList.pViewFormats = viewFormats;
        createInfo.pNext = &formatList;
    }

    if (graphicsFamily_ != presentFamily_) {
        uint32_t families[] = { graphicsFamily_, presentFamily_ };
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = families;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform   = caps.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode    = presentMode;
    createInfo.clipped        = VK_TRUE;
    createInfo.oldSwapchain   = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_);
    if (result != VK_SUCCESS) {
        KL_ERR("VulkanBackend", "vkCreateSwapchainKHR failed: %d", result);
        return false;
    }

    vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr);
    swapImages_.resize(imageCount);
    vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, swapImages_.data());

    return true;
}

// ============================================================================
// Image Views
// ============================================================================

bool VulkanBackend::CreateImageViews() {
    swapImageViews_.resize(swapImages_.size());
    for (size_t i = 0; i < swapImages_.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image    = swapImages_[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format   = static_cast<VkFormat>(swapViewFormat_);
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel   = 0;
        createInfo.subresourceRange.levelCount     = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(device_, &createInfo, nullptr, &swapImageViews_[i]) != VK_SUCCESS) {
            KL_ERR("VulkanBackend", "Failed to create image view %zu", i);
            return false;
        }
    }
    return true;
}

// ============================================================================
// Render Pass
// ============================================================================

bool VulkanBackend::CreateRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format         = static_cast<VkFormat>(swapViewFormat_);
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &colorRef;

    VkSubpassDependency dep{};
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rpInfo{};
    rpInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = 1;
    rpInfo.pAttachments    = &colorAttachment;
    rpInfo.subpassCount    = 1;
    rpInfo.pSubpasses      = &subpass;
    rpInfo.dependencyCount = 1;
    rpInfo.pDependencies   = &dep;

    if (vkCreateRenderPass(device_, &rpInfo, nullptr, &renderPass_) != VK_SUCCESS) {
        KL_ERR("VulkanBackend", "Failed to create render pass");
        return false;
    }
    return true;
}

// ============================================================================
// Framebuffers
// ============================================================================

bool VulkanBackend::CreateFramebuffers() {
    swapFramebuffers_.resize(swapImageViews_.size());
    for (size_t i = 0; i < swapImageViews_.size(); i++) {
        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass      = renderPass_;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments    = &swapImageViews_[i];
        fbInfo.width           = swapExtentW_;
        fbInfo.height          = swapExtentH_;
        fbInfo.layers          = 1;

        if (vkCreateFramebuffer(device_, &fbInfo, nullptr, &swapFramebuffers_[i]) != VK_SUCCESS) {
            KL_ERR("VulkanBackend", "Failed to create framebuffer %zu", i);
            return false;
        }
    }
    return true;
}

// ============================================================================
// Command Pool & Buffers
// ============================================================================

bool VulkanBackend::CreateCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsFamily_;

    if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
        KL_ERR("VulkanBackend", "Failed to create command pool");
        return false;
    }
    return true;
}

bool VulkanBackend::CreateCommandBuffers() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = commandPool_;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = kMaxFramesInFlight;

    if (vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_) != VK_SUCCESS) {
        KL_ERR("VulkanBackend", "Failed to allocate command buffers");
        return false;
    }
    return true;
}

// ============================================================================
// Synchronization
// ============================================================================

bool VulkanBackend::CreateSyncObjects() {
    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled

    for (int i = 0; i < kMaxFramesInFlight; i++) {
        if (vkCreateSemaphore(device_, &semInfo, nullptr, &imageAvailableSems_[i]) != VK_SUCCESS ||
            vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS) {
            KL_ERR("VulkanBackend", "Failed to create sync objects");
            return false;
        }
    }

    // Per-swapchain-image renderFinished semaphores
    uint32_t imageCount = static_cast<uint32_t>(swapImages_.size());
    renderFinishedSems_.resize(imageCount, VK_NULL_HANDLE);
    for (uint32_t i = 0; i < imageCount; i++) {
        if (vkCreateSemaphore(device_, &semInfo, nullptr, &renderFinishedSems_[i]) != VK_SUCCESS) {
            KL_ERR("VulkanBackend", "Failed to create render-finished semaphore %u", i);
            return false;
        }
    }
    return true;
}

// ============================================================================
// Staging Buffer (for CPU framebuffer upload)
// ============================================================================

bool VulkanBackend::CreateStagingBuffer(size_t size) {
    VkBufferCreateInfo bufInfo{};
    bufInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size        = size;
    bufInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device_, &bufInfo, nullptr, &stagingBuffer_) != VK_SUCCESS)
        return false;

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device_, stagingBuffer_, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device_, &allocInfo, nullptr, &stagingMemory_) != VK_SUCCESS) {
        vkDestroyBuffer(device_, stagingBuffer_, nullptr);
        stagingBuffer_ = nullptr;
        return false;
    }

    vkBindBufferMemory(device_, stagingBuffer_, stagingMemory_, 0);
    vkMapMemory(device_, stagingMemory_, 0, size, 0, &stagingMapped_);
    stagingSize_ = size;
    return true;
}

void VulkanBackend::DestroyStagingBuffer() {
    if (stagingMapped_ && stagingMemory_ && device_) {
        vkUnmapMemory(device_, stagingMemory_);
        stagingMapped_ = nullptr;
    }
    if (stagingBuffer_ && device_) {
        vkDestroyBuffer(device_, stagingBuffer_, nullptr);
        stagingBuffer_ = nullptr;
    }
    if (stagingMemory_ && device_) {
        vkFreeMemory(device_, stagingMemory_, nullptr);
        stagingMemory_ = nullptr;
    }
    stagingSize_ = 0;
}

uint32_t VulkanBackend::FindMemoryType(uint32_t typeFilter, uint32_t properties) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProps);

    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return 0; // Fallback (should not happen on conformant drivers)
}

// ============================================================================
// Window Helpers
// ============================================================================

void VulkanBackend::SetTitle(const std::string& title) {
    title_ = title;
    if (window_) SDL_SetWindowTitle(window_, title.c_str());
}

bool VulkanBackend::SetFullscreen(bool fullscreen) {
    if (!window_) return false;
    fullscreen_ = fullscreen;
    return SDL_SetWindowFullscreen(window_, fullscreen);
}

bool VulkanBackend::IsWindowOpen() const {
    return window_ != nullptr && initialized_;
}

void VulkanBackend::GetWindowSize(uint32_t& width, uint32_t& height) const {
    if (window_) {
        int w, h;
        SDL_GetWindowSize(window_, &w, &h);
        width = static_cast<uint32_t>(w);
        height = static_cast<uint32_t>(h);
    } else {
        width = windowWidth_;
        height = windowHeight_;
    }
}

} // namespace koilo
