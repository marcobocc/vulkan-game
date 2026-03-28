#pragma once

#include <volk.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <log4cxx/logger.h>
#include <string>
#include <vulkan/vulkan.h>
#include "VulkanBuffer.hpp"
#include "VulkanPipeline.hpp"
#include "vulkan/VulkanCommandManager.hpp"
#include "vulkan/VulkanDevice.hpp"
#include "vulkan/VulkanInstance.hpp"
#include "vulkan/VulkanSwapchainManager.hpp"

class VulkanGraphicsBackend {
public:
    VulkanGraphicsBackend() {}
    ~VulkanGraphicsBackend() { cleanup(); }
    bool init(GLFWwindow *window) {
        if (!glfwInit()) {
            return false;
        }
        if (!window) {
            return false;
        }
        volkInitialize();
        window_ = window;
        if (!instance_.init()) {
            return false;
        }
        if (!device_.init(instance_)) {
            return false;
        }
        VkResult surfaceResult = VK_ERROR_INITIALIZATION_FAILED;
        surfaceResult = glfwCreateWindowSurface(instance_.instance(), window, nullptr, &surface_);
        if (surfaceResult != VK_SUCCESS || surface_ == VK_NULL_HANDLE) {
            return false;
        }
        VkBool32 presentSupported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device_.physicalDevice(), device_.graphicsQueueFamilyIndex(), surface_,
                                             &presentSupported);
        if (!presentSupported) {
            return false;
        }
        swapchainManager_ = new VulkanSwapchainManager(device_, surface_);
        if (!swapchainManager_->init()) {
            return false;
        }
        commandManager_ = new VulkanCommandManager(device_);
        if (!commandManager_->init()) {
            return false;
        }
        vertexBuffer_ = new VulkanBuffer(
                device_.device(), device_.physicalDevice(), sizeof(triangleVertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, triangleVertices);
        std::string vertPath = "shaders/triangle.vert.spv";
        std::string fragPath = "shaders/triangle.frag.spv";
        pipeline_ = new VulkanPipeline(device_.device(), swapchainManager_->renderPass(), vertPath.c_str(),
                                       fragPath.c_str());
        return true;
    }
    void renderFrame() {
        uint32_t imageIndex;
        if (!swapchainManager_->acquireNextImage(VK_NULL_HANDLE, imageIndex))
            return;
        if (imageIndex >= swapchainManager_->imageCount()) {
            return;
        }
        commandManager_->beginFrame();
        VkCommandBuffer cmd = commandManager_->allocateCommandBuffer();
        VulkanCommandManager::beginCommandBuffer(cmd);
        VkClearValue clearColor = {{{0.1f, 0.1f, 0.1f, 1.0f}}};
        VkRenderPassBeginInfo rpInfo{};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpInfo.renderPass = swapchainManager_->renderPass();
        rpInfo.framebuffer = swapchainManager_->framebuffer(imageIndex);
        rpInfo.renderArea.offset = {0, 0};
        rpInfo.renderArea.extent = swapchainManager_->extent();
        rpInfo.clearValueCount = 1;
        rpInfo.pClearValues = &clearColor;
        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->pipeline);
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer_->buffer, offsets);
        vkCmdDraw(cmd, 3, 1, 0, 0);
        vkCmdEndRenderPass(cmd);
        VulkanCommandManager::endCommandBuffer(cmd);
        commandManager_->submitCommandBuffer(cmd);
        swapchainManager_->present(device_.presentQueue(), VK_NULL_HANDLE, imageIndex);
        commandManager_->endFrame();
    }
    void cleanup() {
        vkDeviceWaitIdle(device_.device());
        if (pipeline_) {
            pipeline_->destroy(device_.device());
            delete pipeline_;
            pipeline_ = nullptr;
        }
        if (vertexBuffer_) {
            vertexBuffer_->destroy(device_.device());
            delete vertexBuffer_;
            vertexBuffer_ = nullptr;
        }
        if (commandManager_) {
            delete commandManager_;
            commandManager_ = nullptr;
        }
        if (swapchainManager_) {
            delete swapchainManager_;
            swapchainManager_ = nullptr;
        }
        if (surface_ != VK_NULL_HANDLE && instance_.instance()) {
            vkDestroySurfaceKHR(instance_.instance(), surface_, nullptr);
            surface_ = VK_NULL_HANDLE;
        }
    }

private:
    inline static log4cxx::LoggerPtr logger_ = log4cxx::Logger::getLogger("VulkanGraphicsBackend");
    struct Vertex {
        float pos[2];
        float color[3];
    };
    static inline Vertex triangleVertices[3] = {{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                                {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                                                {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};
    VulkanInstance instance_;
    VulkanDevice device_;
    VulkanSwapchainManager *swapchainManager_ = nullptr;
    VulkanCommandManager *commandManager_ = nullptr;
    VulkanBuffer *vertexBuffer_ = nullptr;
    VulkanPipeline *pipeline_ = nullptr;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    GLFWwindow *window_ = nullptr;
    uint32_t frameWidth_ = 800;
    uint32_t frameHeight_ = 600;
};
