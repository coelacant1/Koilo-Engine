// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file vulkanbackend.hpp
 * @brief Vulkan display backend with GPU acceleration.
 *
 * Implements IGPUDisplayBackend using Vulkan + SDL3.
 * Manages Vulkan instance, device, swapchain, and presentation.
 *
 * @date 12/22/2025
 * @author Coela
 */

#pragma once

#include <koilo/systems/display/igpu_display_backend.hpp>
#include <string>
#include <vector>
#include <functional>
#include "../../../../registry/reflect_macros.hpp"

// Forward declarations
struct SDL_Window;
struct VkInstance_T;
struct VkPhysicalDevice_T;
struct VkDevice_T;
struct VkQueue_T;
struct VkSurfaceKHR_T;
struct VkSwapchainKHR_T;
struct VkImage_T;
struct VkImageView_T;
struct VkCommandPool_T;
struct VkCommandBuffer_T;
struct VkSemaphore_T;
struct VkFence_T;
struct VkRenderPass_T;
struct VkFramebuffer_T;
struct VkBuffer_T;
struct VkDeviceMemory_T;
struct VkDebugUtilsMessengerEXT_T;

typedef VkInstance_T*                VkInstance;
typedef VkPhysicalDevice_T*         VkPhysicalDevice;
typedef VkDevice_T*                 VkDevice;
typedef VkQueue_T*                  VkQueue;
typedef VkSurfaceKHR_T*             VkSurfaceKHR;
typedef VkSwapchainKHR_T*           VkSwapchainKHR;
typedef VkImage_T*                  VkImage;
typedef VkImageView_T*              VkImageView;
typedef VkCommandPool_T*            VkCommandPool;
typedef VkCommandBuffer_T*          VkCommandBuffer;
typedef VkSemaphore_T*              VkSemaphore;
typedef VkFence_T*                  VkFence;
typedef VkRenderPass_T*             VkRenderPass;
typedef VkFramebuffer_T*            VkFramebuffer;
typedef VkBuffer_T*                 VkBuffer;
typedef VkDeviceMemory_T*           VkDeviceMemory;
typedef VkDebugUtilsMessengerEXT_T* VkDebugUtilsMessengerEXT;

namespace koilo {

/** GPU information for selection and display. */
struct VulkanGPUInfo {
    uint32_t    index;
    std::string name;
    uint32_t    vendorID;
    uint32_t    deviceID;
    bool        isDiscrete;
    uint64_t    vramBytes;

    KL_BEGIN_FIELDS(VulkanGPUInfo)
        KL_FIELD(VulkanGPUInfo, index, "Index", 0, 4294967295),
        KL_FIELD(VulkanGPUInfo, name, "Name", 0, 0),
        KL_FIELD(VulkanGPUInfo, vendorID, "Vendor id", 0, 4294967295),
        KL_FIELD(VulkanGPUInfo, deviceID, "Device id", 0, 4294967295),
        KL_FIELD(VulkanGPUInfo, isDiscrete, "Is discrete", 0, 1),
        KL_FIELD(VulkanGPUInfo, vramBytes, "Vram bytes", 0, 18446744073709551615ULL)
    KL_END_FIELDS

    KL_BEGIN_METHODS(VulkanGPUInfo)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(VulkanGPUInfo)
        /* No reflected ctors. */
    KL_END_DESCRIBE(VulkanGPUInfo)

};

static constexpr int kMaxFramesInFlight = 2;

/**
 * @class VulkanBackend
 * @brief GPU-accelerated display backend using Vulkan and SDL3.
 *
 * Creates a native window with a Vulkan surface and manages the
 * swapchain, command buffers, and synchronization for presentation.
 * Can enumerate and select GPUs at runtime.
 */
class VulkanBackend : public IGPUDisplayBackend {
public:
    VulkanBackend(uint32_t width, uint32_t height,
                  const std::string& title = "KoiloEngine",
                  bool fullscreen = false,
                  bool resizable = true);
    ~VulkanBackend() override;

    // === IDisplayBackend Interface ===
    bool Initialize() override;
    void Shutdown() override;
    bool IsInitialized() const override;

    DisplayInfo GetInfo() const override;
    bool HasCapability(DisplayCapability cap) const override;

    bool Present(const Framebuffer& fb) override;
    void WaitVSync() override;
    bool Clear() override;

    bool SetRefreshRate(uint32_t hz) override;
    bool SetOrientation(Orientation orient) override;
    bool SetBrightness(uint8_t brightness) override;
    bool SetVSyncEnabled(bool enabled) override;

    // === IGPUDisplayBackend Interface ===
    void SwapOnly() override;
    bool PresentNoSwap(const Framebuffer& fb) override;
    void SetNearestFiltering(bool nearest) override;
    void PrepareDefaultFramebuffer(int width, int height) override;

    // === Vulkan Specific ===

    /** Enumerate available GPUs. Call after Initialize(). */
    const std::vector<VulkanGPUInfo>& GetAvailableGPUs() const { return gpuInfos_; }

    /** Get the index of the currently selected GPU. */
    uint32_t GetSelectedGPUIndex() const { return selectedGPU_; }

    /** Get Vulkan handles for the render backend. */
    VkInstance       GetInstance()       const { return instance_; }
    VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice_; }
    VkDevice         GetDevice()         const { return device_; }
    VkQueue          GetGraphicsQueue()  const { return graphicsQueue_; }
    uint32_t         GetGraphicsFamily() const { return graphicsFamily_; }
    VkRenderPass     GetRenderPass()     const { return renderPass_; }
    uint32_t         GetSwapchainWidth() const { return swapExtentW_; }
    uint32_t         GetSwapchainHeight()const { return swapExtentH_; }
    uint32_t         GetSwapchainFormat()const { return swapViewFormat_; }
    uint32_t         GetCurrentFrame()   const { return currentFrame_; }

    /** Begin a render pass targeting the current swapchain image. */
    VkCommandBuffer BeginFrame();
    void BeginSwapchainRenderPass();

    /** End the render pass and submit. Does NOT present (call SwapOnly). */
    void EndFrame();

    /** Check if a frame is currently in progress. */
    bool IsFrameActive() const { return frameActive_; }

    /** Recreate swapchain (e.g., on resize). */
    bool RecreateSwapchain();

    /** Notify the backend that the window was resized. */
    void NotifyResized() { framebufferResized_ = true; }

    void SetTitle(const std::string& title);
    bool SetFullscreen(bool fullscreen);
    bool IsWindowOpen() const;
    void GetWindowSize(uint32_t& width, uint32_t& height) const;

private:
    // SDL
    SDL_Window* window_ = nullptr;

    // Window config
    uint32_t windowWidth_;
    uint32_t windowHeight_;
    std::string title_;
    bool fullscreen_;
    bool resizable_;
    bool initialized_ = false;
    bool vsyncEnabled_ = true;
    bool nearestFiltering_ = false;

    // Vulkan core
    VkInstance       instance_       = nullptr;
    VkSurfaceKHR    surface_        = nullptr;
    VkPhysicalDevice physicalDevice_ = nullptr;
    VkDevice         device_         = nullptr;

    VkQueue graphicsQueue_ = nullptr;
    VkQueue presentQueue_  = nullptr;
    uint32_t graphicsFamily_ = 0;
    uint32_t presentFamily_  = 0;

    // Debug
    VkDebugUtilsMessengerEXT debugMessenger_ = nullptr;
    bool validationEnabled_ = false;

    // Swapchain
    VkSwapchainKHR swapchain_ = nullptr;
    std::vector<VkImage>       swapImages_;
    std::vector<VkImageView>   swapImageViews_;
    std::vector<VkFramebuffer> swapFramebuffers_;
    uint32_t swapExtentW_ = 0;
    uint32_t swapExtentH_ = 0;
    uint32_t swapImageFormat_ = 0;
    uint32_t swapViewFormat_ = 0;   // UNORM equivalent for image views/render pass
    bool     mutableFormatSupported_ = false;
    uint32_t imageIndex_ = 0;

    // Render pass (simple clear-to-present)
    VkRenderPass renderPass_ = nullptr;

    // Command buffers
    VkCommandPool commandPool_ = nullptr;
    VkCommandBuffer commandBuffers_[kMaxFramesInFlight] = {};

    // Synchronization
    VkSemaphore imageAvailableSems_[kMaxFramesInFlight] = {};
    std::vector<VkSemaphore> renderFinishedSems_;  // per-swapchain-image
    VkFence     inFlightFences_[kMaxFramesInFlight]     = {};
    uint32_t    currentFrame_ = 0;
    bool        frameActive_ = false;
    bool        swapRenderPassActive_ = false;
    bool        transferOnlyFrame_ = false;
    bool        framebufferResized_ = false;

    // Staging buffer for CPU framebuffer upload
    VkBuffer       stagingBuffer_  = nullptr;
    VkDeviceMemory stagingMemory_  = nullptr;
    size_t         stagingSize_    = 0;
    void*          stagingMapped_  = nullptr;

    // GPU enumeration
    std::vector<VulkanGPUInfo>     gpuInfos_;
    std::vector<VkPhysicalDevice>  physicalDevices_;
    uint32_t selectedGPU_ = 0;

    DisplayInfo info_;

    // Init helpers
    bool CreateInstance();
    bool SetupDebugMessenger();
    bool CreateSurface();
    bool EnumerateGPUs();
    bool SelectGPU(uint32_t index);
    bool CreateLogicalDevice();
    bool CreateSwapchain();
    bool CreateImageViews();
    bool CreateRenderPass();
    bool CreateFramebuffers();
    bool CreateCommandPool();
    bool CreateCommandBuffers();
    bool CreateSyncObjects();

    // Cleanup helpers
    void CleanupSwapchain();

    // Staging buffer
    bool CreateStagingBuffer(size_t size);
    void DestroyStagingBuffer();

    // Utility
    uint32_t FindMemoryType(uint32_t typeFilter, uint32_t properties);

    KL_BEGIN_FIELDS(VulkanBackend)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(VulkanBackend)
        KL_METHOD_AUTO(VulkanBackend, Shutdown, "Shutdown"),
        KL_METHOD_AUTO(VulkanBackend, IsInitialized, "Is initialized"),
        KL_METHOD_AUTO(VulkanBackend, GetInfo, "Get info"),
        KL_METHOD_AUTO(VulkanBackend, HasCapability, "Has capability"),
        KL_METHOD_AUTO(VulkanBackend, Present, "Present"),
        KL_METHOD_AUTO(VulkanBackend, WaitVSync, "Wait vsync"),
        KL_METHOD_AUTO(VulkanBackend, Clear, "Clear"),
        KL_METHOD_AUTO(VulkanBackend, SetRefreshRate, "Set refresh rate"),
        KL_METHOD_AUTO(VulkanBackend, SetOrientation, "Set orientation"),
        KL_METHOD_AUTO(VulkanBackend, SetBrightness, "Set brightness"),
        KL_METHOD_AUTO(VulkanBackend, SetVSyncEnabled, "Set vsync enabled"),
        KL_METHOD_AUTO(VulkanBackend, PresentNoSwap, "Present no swap"),
        KL_METHOD_AUTO(VulkanBackend, SetNearestFiltering, "Set nearest filtering"),
        KL_METHOD_AUTO(VulkanBackend, PrepareDefaultFramebuffer, "Prepare default framebuffer"),
        KL_METHOD_AUTO(VulkanBackend, GetSelectedGPUIndex, "Get selected gpuindex"),
        KL_METHOD_AUTO(VulkanBackend, GetInstance, "Get instance"),
        KL_METHOD_AUTO(VulkanBackend, GetPhysicalDevice, "Get physical device"),
        KL_METHOD_AUTO(VulkanBackend, GetDevice, "Get device"),
        KL_METHOD_AUTO(VulkanBackend, GetGraphicsQueue, "Get graphics queue"),
        KL_METHOD_AUTO(VulkanBackend, GetGraphicsFamily, "Get graphics family"),
        KL_METHOD_AUTO(VulkanBackend, GetRenderPass, "Get render pass"),
        KL_METHOD_AUTO(VulkanBackend, GetSwapchainWidth, "Get swapchain width"),
        KL_METHOD_AUTO(VulkanBackend, GetSwapchainHeight, "Get swapchain height"),
        KL_METHOD_AUTO(VulkanBackend, GetSwapchainFormat, "Get swapchain format"),
        KL_METHOD_AUTO(VulkanBackend, GetCurrentFrame, "Get current frame"),
        KL_METHOD_AUTO(VulkanBackend, BeginFrame, "Begin frame"),
        KL_METHOD_AUTO(VulkanBackend, BeginSwapchainRenderPass, "Begin swapchain render pass"),
        KL_METHOD_AUTO(VulkanBackend, SetTitle, "Set title"),
        KL_METHOD_AUTO(VulkanBackend, SetFullscreen, "Set fullscreen"),
        KL_METHOD_AUTO(VulkanBackend, IsWindowOpen, "Is window open"),
        KL_METHOD_AUTO(VulkanBackend, GetWindowSize, "Get window size")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(VulkanBackend)
        KL_CTOR(VulkanBackend, uint32_t, uint32_t, const std::string&, bool, bool)
    KL_END_DESCRIBE(VulkanBackend)

};

} // namespace koilo
