#pragma once

#include <log4cxx/logger.h>
#include <vector>
#include <volk.h>
#include <vulkan/vulkan.h>
#include "VulkanInstance.hpp"

class VulkanDevice {
public:
    VulkanDevice() = default;
    ~VulkanDevice() {
        if (device_) {
            vkDeviceWaitIdle(device_);
        }
        if (commandPool_)
            vkDestroyCommandPool(device_, commandPool_, nullptr);
        if (device_)
            vkDestroyDevice(device_, nullptr);
    }
    bool init(VulkanInstance &instance) {
        instance_ = instance.instance();
        if (!pickPhysicalDevice()) {
            return false;
        }
        if (!createLogicalDevice()) {
            return false;
        }
        return true;
    }
    VkDevice device() const { return device_; }
    VkPhysicalDevice physicalDevice() const { return physicalDevice_; }
    VkQueue graphicsQueue() const { return graphicsQueue_; }
    VkQueue presentQueue() const { return presentQueue_; }
    uint32_t graphicsQueueFamilyIndex() const { return graphicsFamilyIndex_; }

private:
    bool pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        if (vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr) != VK_SUCCESS || deviceCount == 0) {
            return false;
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());
        for (const auto device: devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice_ = device;
                return true;
            }
        }
        return false;
    }
    static bool isDeviceSuitable(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties props{};
        VkPhysicalDeviceFeatures features{};
        vkGetPhysicalDeviceProperties(device, &props);
        vkGetPhysicalDeviceFeatures(device, &features);
        return props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
               props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
    }
    bool createLogicalDevice() {
        graphicsFamilyIndex_ = findGraphicsQueueFamily();
        if (graphicsFamilyIndex_ == UINT32_MAX) {
            return false;
        }
        float priority = 1.0f;
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = graphicsFamilyIndex_;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &priority;
        VkPhysicalDeviceFeatures features{};
        std::vector<const char *> extensions;
#ifdef __APPLE__
        extensions.push_back("VK_KHR_portability_subset");
#endif
        extensions.push_back("VK_KHR_swapchain");
        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.pQueueCreateInfos = &queueInfo;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pEnabledFeatures = &features;
        deviceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        deviceInfo.ppEnabledExtensionNames = extensions.data();
        VkResult res = vkCreateDevice(physicalDevice_, &deviceInfo, nullptr, &device_);
        if (res != VK_SUCCESS) {
            return false;
        }
        volkLoadDevice(device_);
        vkGetDeviceQueue(device_, graphicsFamilyIndex_, 0, &graphicsQueue_);
        presentQueue_ = graphicsQueue_;
        return true;
    }

    uint32_t findGraphicsQueueFamily() const {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &count, families.data());
        for (uint32_t i = 0; i < count; ++i) {
            if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                return i;
        }
        return UINT32_MAX;
    }
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("VulkanDevice");
    VkInstance instance_{VK_NULL_HANDLE};
    VkPhysicalDevice physicalDevice_{VK_NULL_HANDLE};
    VkDevice device_{VK_NULL_HANDLE};
    VkQueue graphicsQueue_{VK_NULL_HANDLE};
    VkQueue presentQueue_{VK_NULL_HANDLE};
    VkCommandPool commandPool_{VK_NULL_HANDLE};
    uint32_t graphicsFamilyIndex_{UINT32_MAX};
};
