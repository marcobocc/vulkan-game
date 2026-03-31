#pragma once

#include <log4cxx/logger.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "triangle_test/TriangleMaterial.hpp"
#include "vulkan/VulkanCommandManager.hpp"
#include "vulkan/VulkanDebugMessenger.hpp"
#include "vulkan/VulkanDevice.hpp"
#include "vulkan/VulkanInstance.hpp"
#include "vulkan/VulkanSwapchainManager.hpp"
#include "vulkan/triangle_test/TriangleObject.hpp"

static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("VulkanGraphicsBackend");

class VulkanGraphicsBackend {
public:
    VulkanGraphicsBackend(const VulkanGraphicsBackend &) = delete;
    VulkanGraphicsBackend &operator=(const VulkanGraphicsBackend &) = delete;
    VulkanGraphicsBackend(VulkanGraphicsBackend &&) = delete;
    VulkanGraphicsBackend &operator=(VulkanGraphicsBackend &&) = delete;

    ~VulkanGraphicsBackend() {
        if (imageAvailableSemaphore_ != VK_NULL_HANDLE)
            vkDestroySemaphore(device_.getVkDevice(), imageAvailableSemaphore_, nullptr);
        if (renderFinishedSemaphore_ != VK_NULL_HANDLE)
            vkDestroySemaphore(device_.getVkDevice(), renderFinishedSemaphore_, nullptr);
    }

    explicit VulkanGraphicsBackend(GLFWwindow *window) :
        window_(window), debugMessenger_(instance_.getVkInstance()), device_(instance_.getVkInstance()),
        commandManager_(device_.getVkDevice(), device_.getGraphicsQueueFamilyIndex(), device_.getVkGraphicsQueue()),
        swapchainManager_(window_, instance_.getVkInstance(), device_.getVkPhysicalDevice(), device_.getVkDevice(),
                          device_.getGraphicsQueueFamilyIndex()),
        triangleObject_(device_.getVkDevice(), device_.getVkPhysicalDevice(), swapchainManager_.renderPass()) {

        if (!window)
            throw std::runtime_error("Window pointer is null");
    }

    void renderFrame() {
        if (imageAvailableSemaphore_ == VK_NULL_HANDLE || renderFinishedSemaphore_ == VK_NULL_HANDLE) {
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            throwIfUnsuccessful(
                    vkCreateSemaphore(device_.getVkDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphore_));
            throwIfUnsuccessful(
                    vkCreateSemaphore(device_.getVkDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphore_));
        }

        uint32_t imageIndex = 0;
        if (!swapchainManager_.acquireNextImage(imageAvailableSemaphore_, imageIndex))
            return;
        if (imageIndex >= swapchainManager_.imageCount())
            return;

        commandManager_.beginFrame();
        VkCommandBuffer cmd = commandManager_.allocateCommandBuffer();
        VulkanCommandManager::beginCommandBuffer(cmd);

        VkClearValue clearColor = {{{0.1f, 0.1f, 0.1f, 1.0f}}};
        VkRenderPassBeginInfo rpInfo{};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpInfo.renderPass = swapchainManager_.renderPass();
        rpInfo.framebuffer = swapchainManager_.framebuffer(imageIndex);
        rpInfo.renderArea.offset = {0, 0};
        rpInfo.renderArea.extent = swapchainManager_.extent();
        rpInfo.clearValueCount = 1;
        rpInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkPipeline trianglePipeline = triangleObject_.getMaterial()->getVkPipeline();
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline);

        std::array<VkDeviceSize, 1> offsets = {0};
        VkBuffer triangleVertexBuffer = triangleObject_.getVkBuffer();
        vkCmdBindVertexBuffers(cmd, 0, 1, &triangleVertexBuffer, offsets.data());
        vkCmdDraw(cmd, 3, 1, 0, 0);
        vkCmdEndRenderPass(cmd);

        VulkanCommandManager::endCommandBuffer(cmd);
        commandManager_.submitCommandBuffer(cmd, imageAvailableSemaphore_, renderFinishedSemaphore_);
        swapchainManager_.present(device_.getVkGraphicsQueue(), renderFinishedSemaphore_, imageIndex);
        commandManager_.endFrame();
    }

private:
    GLFWwindow *window_ = nullptr; // TODO: Create window wrapper object
    VulkanInstance instance_;
    VulkanDebugMessenger debugMessenger_;
    VulkanDevice device_;
    VulkanCommandManager commandManager_;
    VulkanSwapchainManager swapchainManager_;

    VkSemaphore imageAvailableSemaphore_ = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore_ = VK_NULL_HANDLE;

    TriangleObject triangleObject_; // TODO: Move out of this class
};
