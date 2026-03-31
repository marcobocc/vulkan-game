#pragma once

#include <vector>
#include <volk.h>
#include <vulkan/vulkan.h>
#include "vulkan/VulkanErrorHandling.hpp"

class VulkanCommandManager {
public:
    VulkanCommandManager(const VulkanCommandManager&) = delete;
    VulkanCommandManager& operator=(const VulkanCommandManager&) = delete;
    VulkanCommandManager(VulkanCommandManager&&) = delete;
    VulkanCommandManager& operator=(VulkanCommandManager&&) = delete;

    explicit VulkanCommandManager(VkDevice device,
                                  uint32_t graphicsQueueFamilyIndex,
                                  VkQueue graphicsQueue,
                                  size_t maxFramesInFlight) :
        device_(device),
        graphicsQueueFamilyIndex_(graphicsQueueFamilyIndex),
        graphicsQueue_(graphicsQueue),
        frames_(maxFramesInFlight),
        maxFramesInFlight_(maxFramesInFlight),
        currentFrame_(0) {
        createCommandPools();
        createFences();
    }

    ~VulkanCommandManager() {
        for (auto& [commandPool, frameFence]: frames_) {
            if (frameFence != VK_NULL_HANDLE) {
                vkWaitForFences(device_, 1, &frameFence, VK_TRUE, UINT64_MAX);
                vkDestroyFence(device_, frameFence, nullptr);
            }
            if (commandPool != VK_NULL_HANDLE) {
                vkDestroyCommandPool(device_, commandPool, nullptr);
            }
        }
    }

    void beginFrame() {
        auto& [commandPool, frameFence] = frames_.at(currentFrame_);
        vkWaitForFences(device_, 1, &frameFence, VK_TRUE, UINT64_MAX);
        vkResetFences(device_, 1, &frameFence);
        vkResetCommandPool(device_, commandPool, 0);
    }

    void endFrame() { currentFrame_ = (currentFrame_ + 1) % maxFramesInFlight_; }

    VkCommandBuffer allocateCommandBuffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
        auto& [commandPool, frameFence] = frames_.at(currentFrame_);
        VkCommandBufferAllocateInfo alloc{};
        alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc.commandPool = commandPool;
        alloc.level = level;
        alloc.commandBufferCount = 1;

        VkCommandBuffer cmd = VK_NULL_HANDLE;
        throwIfUnsuccessful(vkAllocateCommandBuffers(device_, &alloc, &cmd));
        return cmd;
    }

    void submitCommandBuffer(VkCommandBuffer cmd,
                             VkSemaphore waitSemaphore = VK_NULL_HANDLE,
                             VkSemaphore signalSemaphore = VK_NULL_HANDLE) {
        auto& [commandPool, frameFence] = frames_.at(currentFrame_);
        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        std::array<VkPipelineStageFlags, 1> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        if (waitSemaphore != VK_NULL_HANDLE) {
            submit.waitSemaphoreCount = 1;
            submit.pWaitSemaphores = &waitSemaphore;
            submit.pWaitDstStageMask = waitStages.data();
        }

        if (signalSemaphore != VK_NULL_HANDLE) {
            submit.signalSemaphoreCount = 1;
            submit.pSignalSemaphores = &signalSemaphore;
        }

        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;

        throwIfUnsuccessful(vkQueueSubmit(graphicsQueue_, 1, &submit, frameFence));
    }

    static void beginCommandBuffer(VkCommandBuffer cmd,
                                   VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) {
        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = flags;
        throwIfUnsuccessful(vkBeginCommandBuffer(cmd, &begin));
    }

    static void endCommandBuffer(VkCommandBuffer cmd) { throwIfUnsuccessful(vkEndCommandBuffer(cmd)); }

private:
    void createCommandPools() {
        for (auto& [commandPool, frameFence]: frames_) {
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex_;
            poolInfo.flags = 0;
            throwIfUnsuccessful(vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool));
        }
    }

    void createFences() {
        for (auto& [commandPool, frameFence]: frames_) {
            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            throwIfUnsuccessful(vkCreateFence(device_, &fenceInfo, nullptr, &frameFence));
        }
    }

    struct FrameContext {
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkFence frameFence = VK_NULL_HANDLE;
    };

    VkDevice device_;
    uint32_t graphicsQueueFamilyIndex_;
    VkQueue graphicsQueue_;
    std::vector<FrameContext> frames_;
    size_t maxFramesInFlight_;
    size_t currentFrame_;
};
