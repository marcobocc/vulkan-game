#include "vulkan/VulkanGraphicsBackend.hpp"
#include <stdexcept>

VulkanGraphicsBackend::~VulkanGraphicsBackend() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (imageAvailableSemaphores_.at(i) != VK_NULL_HANDLE)
            vkDestroySemaphore(device_.getVkDevice(), imageAvailableSemaphores_.at(i), nullptr);
        if (renderFinishedSemaphores_.at(i) != VK_NULL_HANDLE)
            vkDestroySemaphore(device_.getVkDevice(), renderFinishedSemaphores_.at(i), nullptr);
        if (inFlightFences_.at(i) != VK_NULL_HANDLE)
            vkDestroyFence(device_.getVkDevice(), inFlightFences_.at(i), nullptr);
    }
}

VulkanGraphicsBackend::VulkanGraphicsBackend(GLFWwindow* window) :
    window_(window),
    debugMessenger_(instance_.getVkInstance()),
    device_(instance_.getVkInstance()),
    commandManager_(device_.getVkDevice(),
                    device_.getGraphicsQueueFamilyIndex(),
                    device_.getVkGraphicsQueue(),
                    MAX_FRAMES_IN_FLIGHT),
    swapchainManager_(window_, instance_.getVkInstance(), device_.getVkPhysicalDevice(), device_.getVkDevice()),
    triangleObject_(device_.getVkDevice(), device_.getVkPhysicalDevice(), swapchainManager_.renderPass()) {
    if (!window) throw std::runtime_error("Window pointer is null");
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        throwIfUnsuccessful(
                vkCreateSemaphore(device_.getVkDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores_.at(i)));
        throwIfUnsuccessful(
                vkCreateSemaphore(device_.getVkDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores_.at(i)));
        throwIfUnsuccessful(vkCreateFence(device_.getVkDevice(), &fenceInfo, nullptr, &inFlightFences_.at(i)));
    }
}

void VulkanGraphicsBackend::renderFrame() {
    VkSemaphore imageAvailableSemaphore = imageAvailableSemaphores_.at(currentFrame_);
    VkSemaphore renderFinishedSemaphore = renderFinishedSemaphores_.at(currentFrame_);
    VkFence inFlightFence = inFlightFences_.at(currentFrame_);
    VkResult fenceStatus = vkGetFenceStatus(device_.getVkDevice(), inFlightFence);
    if (fenceStatus == VK_NOT_READY) {
        // Previous frame is still in flight, skip rendering this frame to avoid blocking UI
        return;
    }
    vkResetFences(device_.getVkDevice(), 1, &inFlightFence);
    uint32_t imageIndex = 0;
    if (!swapchainManager_.acquireNextImage(imageAvailableSemaphore, imageIndex)) return;
    if (imageIndex >= swapchainManager_.imageCount()) return;
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
    commandManager_.submitCommandBuffer(cmd, imageAvailableSemaphore, renderFinishedSemaphore);
    swapchainManager_.present(device_.getVkGraphicsQueue(), renderFinishedSemaphore, imageIndex);
    commandManager_.endFrame();
    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}
