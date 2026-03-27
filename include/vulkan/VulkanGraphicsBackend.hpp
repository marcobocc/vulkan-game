#pragma once
#include <log4cxx/logger.h>
#include <string>
#include <volk.h>
#include "vulkan/VulkanDeviceContext.hpp"

class VulkanGraphicsBackend {
public:
    VulkanGraphicsBackend() = default;
    ~VulkanGraphicsBackend() = default;

    VulkanGraphicsBackend(const VulkanGraphicsBackend&) = delete;
    VulkanGraphicsBackend& operator=(const VulkanGraphicsBackend&) = delete;
    VulkanGraphicsBackend(VulkanGraphicsBackend&&) = delete;
    VulkanGraphicsBackend& operator=(VulkanGraphicsBackend&&) = delete;

    bool init() {
        volkInitialize();
        return deviceContext_.init();
    }

private:
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("VulkanGraphicsBackend");
    VulkanDeviceContext deviceContext_;
};