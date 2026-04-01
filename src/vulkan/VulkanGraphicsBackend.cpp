#include "vulkan/VulkanGraphicsBackend.hpp"
#include <stdexcept>
#include "tilemap_test/HexMapGenerator.hpp"
#include "vulkan/tilemap_test/VulkanHexMapObject.hpp"

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
    pipelinesManager_(device_.getVkDevice(), swapchainManager_.renderPass()),
    triangleObject_(device_.getVkDevice(), device_.getVkPhysicalDevice()),
    hexMapObject_(std::make_unique<VulkanHexMapObject>(
            device_.getVkDevice(), device_.getVkPhysicalDevice(), createDemoHexMap())) {
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

    // ---------------------------------
    // TEST TRIANGLE RENDERING
    // ---------------------------------
    VkPipelineVertexInputStateCreateInfo triangleVertexInputInfo{};
    triangleVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    auto* trianglePipelineObj = pipelinesManager_.createOrGetPipeline(
            "triangle", triangleVertexInputInfo, "shaders/triangle.vert.spv", "shaders/triangle.frag.spv");
    VkPipeline trianglePipeline = trianglePipelineObj->getVkPipeline();
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline);
    std::array<VkDeviceSize, 1> offsets = {0};
    VkBuffer triangleVertexBuffer = triangleObject_.getVkBuffer();
    vkCmdBindVertexBuffers(cmd, 0, 1, &triangleVertexBuffer, offsets.data());
    vkCmdDraw(cmd, 3, 1, 0, 0);

    // ---------------------------------
    // TEST HEXMAP RENDERING
    // ---------------------------------
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(float) * 5; // vec2 pos + vec3 color
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    std::array<VkVertexInputAttributeDescription, 2> attrDescs{};
    attrDescs[0].binding = 0;
    attrDescs[0].location = 0;
    attrDescs[0].format = VK_FORMAT_R32G32_SFLOAT;
    attrDescs[0].offset = 0;
    attrDescs[1].binding = 0;
    attrDescs[1].location = 1;
    attrDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDescs[1].offset = sizeof(float) * 2;
    VkPipelineVertexInputStateCreateInfo hexmapVertexInputInfo{};
    hexmapVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    hexmapVertexInputInfo.vertexBindingDescriptionCount = 1;
    hexmapVertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    hexmapVertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size());
    hexmapVertexInputInfo.pVertexAttributeDescriptions = attrDescs.data();
    auto* hexPipelineObj = pipelinesManager_.createOrGetPipeline(
            "hexmap", hexmapVertexInputInfo, "shaders/hexmap.vert.spv", "shaders/hexmap.frag.spv");
    VkPipeline hexPipeline = hexPipelineObj->getVkPipeline();
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, hexPipeline);
    VkBuffer hexVertexBuffer = hexMapObject_->getVkBuffer();
    vkCmdBindVertexBuffers(cmd, 0, 1, &hexVertexBuffer, offsets.data());
    vkCmdDraw(cmd, static_cast<uint32_t>(hexMapObject_->getVertexCount()), 1, 0, 0);
    // ---------------------------------
    // END OBJECTS RENDERING
    // ---------------------------------

    vkCmdEndRenderPass(cmd);
    VulkanCommandManager::endCommandBuffer(cmd);
    commandManager_.submitCommandBuffer(cmd, imageAvailableSemaphore, renderFinishedSemaphore);
    swapchainManager_.present(device_.getVkGraphicsQueue(), renderFinishedSemaphore, imageIndex);
    commandManager_.endFrame();
    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}
