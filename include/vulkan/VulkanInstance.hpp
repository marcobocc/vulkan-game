#pragma once

#include <volk.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <log4cxx/logger.h>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#ifndef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
#define VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME "VK_KHR_portability_enumeration"
#endif
#ifndef VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#define VK_EXT_DEBUG_UTILS_EXTENSION_NAME "VK_EXT_debug_utils"
#endif

class VulkanInstance {
public:
    VulkanInstance() = default;

    ~VulkanInstance() {
        if (debugMessenger_)
            vkDestroyDebugUtilsMessengerEXT(instance_, debugMessenger_, nullptr);
        if (instance_)
            vkDestroyInstance(instance_, nullptr);
    }

    bool init() {
        if (!createInstance()) {
            return false;
        }
        volkLoadInstance(instance_);
        if (!createDebugMessenger()) {
            return false;
        }
        return true;
    }

    VkInstance instance() const { return instance_; }

    static std::vector<const char *> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
#ifdef __APPLE__
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        extensions.push_back("VK_KHR_get_physical_device_properties2");
#endif
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        return extensions;
    }

private:
    bool createInstance() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "GameEngine";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "GameEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;
        std::vector<const char *> layers;
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
            return false;
        }
        return true;
    }

    bool createDebugMessenger() {
        VkDebugUtilsMessengerCreateInfoEXT info{};
        info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        info.pfnUserCallback = debugCallback;
        return vkCreateDebugUtilsMessengerEXT(instance_, &info, nullptr, &debugMessenger_) == VK_SUCCESS;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                        VkDebugUtilsMessageTypeFlagsEXT type,
                                                        const VkDebugUtilsMessengerCallbackDataEXT *data,
                                                        void *userData) {
        std::string severityStr;
        if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
            severityStr = "VERBOSE";
        else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
            severityStr = "INFO";
        else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            severityStr = "WARNING";
        else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            severityStr = "ERROR";
        else
            severityStr = "UNKNOWN";

        std::string typeStr;
        if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
            typeStr += "GENERAL ";
        if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
            typeStr += "VALIDATION ";
        if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
            typeStr += "PERFORMANCE ";

        std::string message = std::string("[Vulkan][") + severityStr + "] [" + typeStr + "] " +
                              (data && data->pMessage ? data->pMessage : "");
        if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            LOGGER->error(message);
        } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            LOGGER->warn(message);
        } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            LOGGER->info(message);
        } else {
            LOGGER->debug(message);
        }
        return VK_FALSE;
    }

    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("VulkanInstance");

    VkInstance instance_{VK_NULL_HANDLE};
    VkDebugUtilsMessengerEXT debugMessenger_{VK_NULL_HANDLE};
};
