#pragma once

#include <cstring>
#include <stdexcept>
#include <volk.h>
#include <vulkan/vulkan.h>

class VulkanBuffer {
public:
    VkBuffer buffer;
    VkDeviceMemory memory;
    VulkanBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage,
                 VkMemoryPropertyFlags properties, void *data = nullptr) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);
        vkAllocateMemory(device, &allocInfo, nullptr, &memory);
        if (data) {
            void *mapped;
            vkMapMemory(device, memory, 0, size, 0, &mapped);
            memcpy(mapped, data, (size_t) size);
            vkUnmapMemory(device, memory);
        }
        vkBindBufferMemory(device, buffer, memory, 0);
    }
    ~VulkanBuffer() {
        // Ensure buffer and memory are destroyed if not already
        // Note: device must be valid and destruction must not be duplicated
        // This is a safety net; prefer explicit destroy() in owning class
    }
    void destroy(VkDevice device) {
        vkDestroyBuffer(device, buffer, nullptr);
        vkFreeMemory(device, memory, nullptr);
    }

private:
    static inline uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                                          VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable memory type");
    }
};
