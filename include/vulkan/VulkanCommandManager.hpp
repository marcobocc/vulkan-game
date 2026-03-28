#pragma once

#include <log4cxx/logger.h>
#include <vector>
#include <volk.h>
#include <vulkan/vulkan.h>
#include "vulkan/VulkanDevice.hpp"

class VulkanCommandManager {
public:
    explicit VulkanCommandManager(VulkanDevice &device) : device_(device) {}
    ~VulkanCommandManager() {
        for (auto &[commandPool, frameFence]: frames_) {
            if (frameFence) {
                vkWaitForFences(device_.device(), 1, &frameFence, VK_TRUE, UINT64_MAX);
                vkDestroyFence(device_.device(), frameFence, nullptr);
            }
            if (commandPool) {
                vkDestroyCommandPool(device_.device(), commandPool, nullptr);
            }
        }
    }
    VulkanCommandManager(const VulkanCommandManager &) = delete;
    VulkanCommandManager &operator=(const VulkanCommandManager &) = delete;
    VulkanCommandManager(VulkanCommandManager &&) = delete;
    VulkanCommandManager &operator=(VulkanCommandManager &&) = delete;

    bool init() {
        frames_.resize(MAX_FRAMES_IN_FLIGHT);
        for (auto &[commandPool, frameFence]: frames_) {
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.queueFamilyIndex = device_.graphicsQueueFamilyIndex();
            poolInfo.flags = 0;
            if (vkCreateCommandPool(device_.device(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
                return false;
            }
            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            if (vkCreateFence(device_.device(), &fenceInfo, nullptr, &frameFence) != VK_SUCCESS) {
                return false;
            }
        }
        return true;
    }
    uint32_t currentFrameIndex() const { return currentFrame_; }
    void beginFrame() {
        auto &[commandPool, frameFence] = frames_.at(currentFrame_);
        vkWaitForFences(device_.device(), 1, &frameFence, VK_TRUE, UINT64_MAX);
        vkResetFences(device_.device(), 1, &frameFence);
        vkResetCommandPool(device_.device(), commandPool, 0);
    }
    void endFrame() { currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT; }
    VkCommandBuffer allocateCommandBuffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
        auto &[commandPool, frameFence] = frames_.at(currentFrame_);
        VkCommandBufferAllocateInfo alloc{};
        alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc.commandPool = commandPool;
        alloc.level = level;
        alloc.commandBufferCount = 1;
        VkCommandBuffer cmd = nullptr;
        vkAllocateCommandBuffers(device_.device(), &alloc, &cmd);
        return cmd;
    }
    void submitCommandBuffer(VkCommandBuffer cmd, VkSemaphore waitSemaphore = VK_NULL_HANDLE,
                             VkSemaphore signalSemaphore = VK_NULL_HANDLE) {
        auto &[commandPool, frameFence] = frames_.at(currentFrame_);
        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        constexpr VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        if (waitSemaphore) {
            submit.waitSemaphoreCount = 1;
            submit.pWaitSemaphores = &waitSemaphore;
            submit.pWaitDstStageMask = waitStages;
        }
        if (signalSemaphore) {
            submit.signalSemaphoreCount = 1;
            submit.pSignalSemaphores = &signalSemaphore;
        }
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;
        vkQueueSubmit(device_.graphicsQueue(), 1, &submit, frameFence);
    }
    static void beginCommandBuffer(VkCommandBuffer cmd,
                                   VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) {
        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = flags;
        vkBeginCommandBuffer(cmd, &begin);
    }
    static void endCommandBuffer(VkCommandBuffer cmd) { vkEndCommandBuffer(cmd); }

private:
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;
    struct FrameContext {
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkFence frameFence = VK_NULL_HANDLE;
    };
    VulkanDevice &device_;
    std::vector<FrameContext> frames_;
    uint32_t currentFrame_ = 0;
    inline static log4cxx::LoggerPtr logger_ = log4cxx::Logger::getLogger("VulkanCommandManager");
};
