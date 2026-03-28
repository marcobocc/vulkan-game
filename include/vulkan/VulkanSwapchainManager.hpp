#pragma once

#include <algorithm>
#include <log4cxx/logger.h>
#include <vector>
#include <volk.h>
#include <vulkan/vulkan.h>
#include "vulkan/VulkanDevice.hpp"

class VulkanSwapchainManager {
public:
    VulkanSwapchainManager(VulkanDevice &device, VkSurfaceKHR surface) :
        device_(device), surface_(surface) {}
    ~VulkanSwapchainManager() {
        cleanup();
    }
    VulkanSwapchainManager(const VulkanSwapchainManager &) = delete;
    VulkanSwapchainManager &operator=(const VulkanSwapchainManager &) = delete;
    VulkanSwapchainManager(VulkanSwapchainManager &&) = delete;
    VulkanSwapchainManager &operator=(VulkanSwapchainManager &&) = delete;

    bool init() {
        if (!createSwapchain()) {
            return false;
        }
        if (!createImageViews()) {
            return false;
        }
        if (!createRenderPass()) {
            return false;
        }
        if (!createFramebuffers()) {
            return false;
        }
        return true;
    }
    void cleanup() {
        cleanupSwapchain();
        if (renderPass_) {
            vkDestroyRenderPass(device_.device(), renderPass_, nullptr);
            renderPass_ = VK_NULL_HANDLE;
        }
    }
    bool recreate() {
        cleanupSwapchain();
        if (!createSwapchain()) {
            return false;
        }
        if (!createImageViews()) {
            return false;
        }
        if (!createRenderPass()) {
            return false;
        }
        if (!createFramebuffers()) {
            return false;
        }
        return true;
    }
    bool acquireNextImage(VkSemaphore imageAvailableSemaphore, uint32_t &imageIndex) {
        VkResult result = vkAcquireNextImageKHR(device_.device(), swapchain_, UINT64_MAX,
                                                imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        return result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR;
    }
    bool present(VkQueue presentQueue, VkSemaphore renderFinishedSemaphore, uint32_t imageIndex) {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain_;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;
        VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
        return result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR;
    }
    VkExtent2D extent() const { return swapchainExtent_; }
    VkRenderPass renderPass() const { return renderPass_; }
    VkFramebuffer framebuffer(uint32_t imageIndex) const {
        if (imageIndex >= framebuffers_.size()) {
            return VK_NULL_HANDLE;
        }
        return framebuffers_.at(imageIndex);
    }
    VkSwapchainKHR swapchain() const { return swapchain_; }
    uint32_t imageCount() const { return static_cast<uint32_t>(swapchainImages_.size()); }
    VkImage image(uint32_t imageIndex) const {
        if (imageIndex >= swapchainImages_.size()) {
            return VK_NULL_HANDLE;
        }
        return swapchainImages_.at(imageIndex);
    }
    VkImageView imageView(uint32_t imageIndex) const {
        if (imageIndex >= swapchainImageViews_.size()) {
            return VK_NULL_HANDLE;
        }
        return swapchainImageViews_.at(imageIndex);
    }

private:
    inline static log4cxx::LoggerPtr logger_ = log4cxx::Logger::getLogger("VulkanSwapchainManager");
    bool createSwapchain() {
        VkSurfaceCapabilitiesKHR capabilities;
        VkResult capResult =
                vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device_.physicalDevice(), surface_, &capabilities);
        if (capResult != VK_SUCCESS) {
            return false;
        }
        swapchainExtent_ = capabilities.currentExtent;
        uint32_t imageCount = std::clamp(3u, capabilities.minImageCount, capabilities.maxImageCount);
        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device_.physicalDevice(), surface_, &formatCount, nullptr);
        if (formatCount == 0) {
            return false;
        }
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device_.physicalDevice(), surface_, &formatCount, formats.data());
        VkSurfaceFormatKHR chosenFormat = formats[0];
        for (const auto &fmt: formats) {
            if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                chosenFormat = fmt;
                break;
            }
        }
        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device_.physicalDevice(), surface_, &presentModeCount,
                                                  nullptr);
        if (presentModeCount == 0) {
            return false;
        }
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device_.physicalDevice(), surface_, &presentModeCount,
                                                  presentModes.data());
        VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        for (const auto &mode: presentModes) {
            if (mode == VK_PRESENT_MODE_FIFO_KHR) {
                chosenPresentMode = mode;
                break;
            }
        }
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface_;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = chosenFormat.format;
        createInfo.imageColorSpace = chosenFormat.colorSpace;
        createInfo.imageExtent = swapchainExtent_;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = chosenPresentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;
        VkResult swapchainResult = vkCreateSwapchainKHR(device_.device(), &createInfo, nullptr, &swapchain_);
        if (swapchainResult != VK_SUCCESS) {
            return false;
        }
        VkResult getImagesResult = vkGetSwapchainImagesKHR(device_.device(), swapchain_, &imageCount, nullptr);
        if (getImagesResult != VK_SUCCESS) {
            return false;
        }
        swapchainImages_.resize(imageCount);
        getImagesResult =
                vkGetSwapchainImagesKHR(device_.device(), swapchain_, &imageCount, swapchainImages_.data());
        if (getImagesResult != VK_SUCCESS) {
            return false;
        }
        swapchainImageFormat_ = chosenFormat.format;
        return true;
    }
    bool createImageViews() {
        swapchainImageViews_.resize(swapchainImages_.size());
        for (size_t i = 0; i < swapchainImages_.size(); ++i) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = swapchainImages_[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = swapchainImageFormat_;
            viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;
            if (vkCreateImageView(device_.device(), &viewInfo, nullptr, &swapchainImageViews_[i]) !=
                VK_SUCCESS) {
                return false;
            }
        }
        return true;
    }
    bool createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapchainImageFormat_;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        if (vkCreateRenderPass(device_.device(), &renderPassInfo, nullptr, &renderPass_) != VK_SUCCESS) {
            return false;
        }
        return true;
    }
    bool createFramebuffers() {
        framebuffers_.resize(swapchainImageViews_.size());
        for (size_t i = 0; i < swapchainImageViews_.size(); ++i) {
            VkImageView attachments[] = {swapchainImageViews_[i]};
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass_;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapchainExtent_.width;
            framebufferInfo.height = swapchainExtent_.height;
            framebufferInfo.layers = 1;
            if (vkCreateFramebuffer(device_.device(), &framebufferInfo, nullptr, &framebuffers_[i]) !=
                VK_SUCCESS) {
                return false;
            }
        }
        return true;
    }
    void cleanupSwapchain() {
        for (auto fb: framebuffers_) {
            if (fb)
                vkDestroyFramebuffer(device_.device(), fb, nullptr);
        }
        framebuffers_.clear();
        for (auto view: swapchainImageViews_) {
            if (view)
                vkDestroyImageView(device_.device(), view, nullptr);
        }
        swapchainImageViews_.clear();
        if (swapchain_) {
            vkDestroySwapchainKHR(device_.device(), swapchain_, nullptr);
            swapchain_ = VK_NULL_HANDLE;
        }
        swapchainImages_.clear();
    }
    VulkanDevice &device_;
    VkSurfaceKHR surface_;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainImageViews_;
    std::vector<VkFramebuffer> framebuffers_;
    VkFormat swapchainImageFormat_ = VK_FORMAT_UNDEFINED;
    VkExtent2D swapchainExtent_ = {};
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
};
