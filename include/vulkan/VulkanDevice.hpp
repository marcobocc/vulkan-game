#pragma once

#include <log4cxx/logger.h>
#include <vector>
#include <volk.h>
#include <vulkan/vulkan.h>
#include "vulkan/VulkanErrorHandling.hpp"

class VulkanDevice {
public:
    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;
    VulkanDevice(VulkanDevice&&) = delete;
    VulkanDevice& operator=(VulkanDevice&&) = delete;

    ~VulkanDevice() {
        if (device_) {
            vkDeviceWaitIdle(device_);
            vkDestroyDevice(device_, nullptr);
        }
    }

    explicit VulkanDevice(VkInstance instance) {
        pickPhysicalDevice(instance);
        createLogicalDevice(physicalDevice_);
    }

    VkDevice getVkDevice() const { return device_; }
    VkPhysicalDevice getVkPhysicalDevice() const { return physicalDevice_; }
    uint32_t getGraphicsQueueFamilyIndex() const { return graphicsQueueFamilyIndex_; }
    VkQueue getVkGraphicsQueue() const { return graphicsQueue_; }

private:
    bool createLogicalDevice(VkPhysicalDevice physicalDevice) {
        graphicsQueueFamilyIndex_ = findGraphicsQueueFamily(physicalDevice);
        if (graphicsQueueFamilyIndex_ == UINT32_MAX) {
            return false;
        }
        float priority = 1.0f;
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = graphicsQueueFamilyIndex_;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &priority;
        VkPhysicalDeviceFeatures features{};
        std::vector<const char*> extensions;
        extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

#ifdef __APPLE__
        uint32_t extCount = 0;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, availableExtensions.data());

        auto it = std::find_if(
                availableExtensions.begin(), availableExtensions.end(), [](const VkExtensionProperties& ext) {
                    return std::string_view(ext.extensionName,
                                            strnlen(ext.extensionName, VK_MAX_EXTENSION_NAME_SIZE)) ==
                           "VK_KHR_portability_subset";
                });

        if (it != availableExtensions.end()) {
            extensions.push_back("VK_KHR_portability_subset");
        }
#endif

        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.pQueueCreateInfos = &queueInfo;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pEnabledFeatures = &features;
        deviceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        deviceInfo.ppEnabledExtensionNames = extensions.data();

        throwIfUnsuccessful(vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device_));
        volkLoadDevice(device_);

        vkGetDeviceQueue(device_, graphicsQueueFamilyIndex_, 0, &graphicsQueue_);
        presentQueue_ = graphicsQueue_;
        return true;
    }

    void pickPhysicalDevice(VkInstance instance) {
        uint32_t deviceCount = 0;
        throwIfUnsuccessful(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));

        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());
        for (const auto physicalDevice: physicalDevices) {
            if (isDeviceSuitable(physicalDevice)) {
                physicalDevice_ = physicalDevice;
            }
        }
        if (physicalDevice_ == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to find a suitable GPU");
        }
    }

    static bool isDeviceSuitable(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties props{};
        VkPhysicalDeviceFeatures features{};
        vkGetPhysicalDeviceProperties(device, &props);
        vkGetPhysicalDeviceFeatures(device, &features);
        return props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
               props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
    }

    static uint32_t findGraphicsQueueFamily(VkPhysicalDevice physicalDevice) {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, families.data());
        for (uint32_t i = 0; i < count; ++i) {
            if (families.at(i).queueFlags & VK_QUEUE_GRAPHICS_BIT) return i;
        }
        return UINT32_MAX;
    }

    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("VulkanDevice");

    VkDevice device_{VK_NULL_HANDLE};
    VkPhysicalDevice physicalDevice_{VK_NULL_HANDLE};
    VkQueue graphicsQueue_{VK_NULL_HANDLE};
    VkQueue presentQueue_{VK_NULL_HANDLE};
    uint32_t graphicsQueueFamilyIndex_{UINT32_MAX};
};
