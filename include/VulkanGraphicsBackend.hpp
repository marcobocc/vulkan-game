#pragma once
#include <log4cxx/logger.h>
#include <volk.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

#ifndef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#define VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME "VK_KHR_portability_subset"
#endif

class VulkanGraphicsBackend {
public:
    explicit VulkanGraphicsBackend(log4cxx::LoggerPtr logger, bool enableValidation = true)
        : logger_(std::move(logger)),
          enableValidation_(enableValidation) {}

    ~VulkanGraphicsBackend() {
        if (debugMessenger_) vkDestroyDebugUtilsMessengerEXT(instance_, debugMessenger_, nullptr);
        if (commandPool_) vkDestroyCommandPool(device_, commandPool_, nullptr);
        if (device_) vkDestroyDevice(device_, nullptr);
        if (instance_) vkDestroyInstance(instance_, nullptr);
    }

    VulkanGraphicsBackend(const VulkanGraphicsBackend&) = delete;
    VulkanGraphicsBackend& operator=(const VulkanGraphicsBackend&) = delete;
    VulkanGraphicsBackend(VulkanGraphicsBackend&&) = delete;
    VulkanGraphicsBackend& operator=(VulkanGraphicsBackend&&) = delete;

    bool init(VkInstance instanceHint = VK_NULL_HANDLE) {
        if (!createInstance(instanceHint)) return false;
        volkLoadInstance(instance_);
        if (!pickPhysicalDevice()) return false;
        if (!createLogicalDevice()) return false;
        if (!createCommandPool()) return false;
        if (!createDebugMessenger()) return false;
        return true;
    }

private:

// ----------------------------------------------------------
// Instance
// ----------------------------------------------------------

    bool createInstance(VkInstance hint) {
        if (hint != VK_NULL_HANDLE) {
            instance_ = hint;
            return true;
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "GameEngine";
        appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
        appInfo.pEngineName = "GameEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        std::vector<const char*> layers;
        if (enableValidation_)
            layers.push_back("VK_LAYER_KHRONOS_validation");

        const auto extensions = getRequiredExtensions();

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
        createInfo.ppEnabledLayerNames = layers.data();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

#ifdef __APPLE__
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

        const VkResult res = vkCreateInstance(&createInfo, nullptr, &instance_);
        if (res != VK_SUCCESS) {
            LOG4CXX_ERROR(logger_, "Failed to create Vulkan instance: " << res);
            return false;
        }

        LOG4CXX_INFO(logger_, "Vulkan instance created");
        return true;
    }

    std::vector<const char*> getRequiredExtensions() const {
        std::vector<const char*> extensions;
#ifdef __APPLE__
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        if (enableValidation_)
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        return extensions;
    }

// ----------------------------------------------------------
// Physical Device
// ----------------------------------------------------------

    bool pickPhysicalDevice() {
        uint32_t deviceCount = 0;

        if (vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr) != VK_SUCCESS || deviceCount == 0) {
            LOG4CXX_ERROR(logger_, "No Vulkan-capable GPU found");
            return false;
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

        for (const auto device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice_ = device;
                LOG4CXX_INFO(logger_, "Selected physical device");
                return true;
            }
        }

        LOG4CXX_ERROR(logger_, "No suitable physical device found");
        return false;
    }

    bool isDeviceSuitable(VkPhysicalDevice device) const {
        VkPhysicalDeviceProperties props{};
        VkPhysicalDeviceFeatures features{};

        vkGetPhysicalDeviceProperties(device, &props);
        vkGetPhysicalDeviceFeatures(device, &features);

        LOG4CXX_INFO(logger_, "Checking device: "
                     << props.deviceName << " (type " << props.deviceType << ")");

#ifdef __APPLE__
        return props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
               props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
#else
        return features.geometryShader;
#endif
    }

// ----------------------------------------------------------
// Logical Device
// ----------------------------------------------------------

    bool createLogicalDevice() {
        graphicsFamilyIndex_ = findGraphicsQueueFamily();
        if (graphicsFamilyIndex_ == UINT32_MAX) {
            LOG4CXX_ERROR(logger_, "No graphics queue family found");
            return false;
        }

        float priority = 1.0f;

        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = graphicsFamilyIndex_;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &priority;

        VkPhysicalDeviceFeatures features{};

        std::vector<const char*> extensions;

#ifdef __APPLE__
        extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.pQueueCreateInfos = &queueInfo;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pEnabledFeatures = &features;
        deviceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        deviceInfo.ppEnabledExtensionNames = extensions.data();

        VkResult res = vkCreateDevice(physicalDevice_, &deviceInfo, nullptr, &device_);
        if (res != VK_SUCCESS) {
            LOG4CXX_ERROR(logger_, "Failed to create logical device: " << res);
            return false;
        }

        vkGetDeviceQueue(device_, graphicsFamilyIndex_, 0, &graphicsQueue_);
        presentQueue_ = graphicsQueue_;

        LOG4CXX_INFO(logger_, "Logical device created");
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

// ----------------------------------------------------------
// Command Pool
// ----------------------------------------------------------

    bool createCommandPool() {
        VkCommandPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.queueFamilyIndex = graphicsFamilyIndex_;
        info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if (vkCreateCommandPool(device_, &info, nullptr, &commandPool_) != VK_SUCCESS) {
            LOG4CXX_ERROR(logger_, "Failed to create command pool");
            return false;
        }

        LOG4CXX_INFO(logger_, "Command pool created");
        return true;
    }

// ----------------------------------------------------------
// Vulkan Debugger
// ----------------------------------------------------------

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void* userData)
    {
        static const log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("VulkanDebug");
        if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            LOG4CXX_ERROR(logger, "[Vulkan] " << data->pMessage);
        else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            LOG4CXX_WARN(logger, "[Vulkan] " << data->pMessage);
        else
            LOG4CXX_INFO(logger, "[Vulkan] " << data->pMessage);
        return VK_FALSE;
    }

    bool createDebugMessenger()
    {
        if (!enableValidation_)
            return true;

        VkDebugUtilsMessengerCreateInfoEXT info{};
        info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

        info.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        info.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        info.pfnUserCallback = debugCallback;
        return vkCreateDebugUtilsMessengerEXT(
            instance_,
            &info,
            nullptr,
            &debugMessenger_) == VK_SUCCESS;
    }

// ----------------------------------------------------------
// Members
// ----------------------------------------------------------

    log4cxx::LoggerPtr logger_;
    bool enableValidation_{true};

    VkInstance instance_{VK_NULL_HANDLE};
    VkPhysicalDevice physicalDevice_{VK_NULL_HANDLE};
    VkDevice device_{VK_NULL_HANDLE};

    VkQueue graphicsQueue_{VK_NULL_HANDLE};
    VkQueue presentQueue_{VK_NULL_HANDLE};

    VkCommandPool commandPool_{VK_NULL_HANDLE};

    uint32_t graphicsFamilyIndex_{UINT32_MAX};
    VkDebugUtilsMessengerEXT debugMessenger_{VK_NULL_HANDLE};
};