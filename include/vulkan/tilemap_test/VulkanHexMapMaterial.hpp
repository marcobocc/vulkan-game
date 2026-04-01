#pragma once
#include <array>
#include <vector>
#include <vulkan/vulkan.h>

class VulkanHexMapMaterial {
public:
    VulkanHexMapMaterial(const VulkanHexMapMaterial&) = delete;
    VulkanHexMapMaterial& operator=(const VulkanHexMapMaterial&) = delete;
    VulkanHexMapMaterial(VulkanHexMapMaterial&&) = delete;
    VulkanHexMapMaterial& operator=(VulkanHexMapMaterial&&) = delete;
    ~VulkanHexMapMaterial();
    explicit VulkanHexMapMaterial(VkDevice device, VkRenderPass renderPass);
    VkPipeline getVkPipeline() const;

private:
    void createPipelineLayout();
    std::array<VkPipelineShaderStageCreateInfo, 2> createShaderStages(const char* vertPath, const char* fragPath);
    void createGraphicsPipeline(VkRenderPass renderPass,
                                const std::array<VkPipelineShaderStageCreateInfo, 2>& shaderStages);
    static std::vector<char> readFile(const char* filename);
    VkShaderModule createShaderModule(const std::vector<char>& code) const;
    VkDevice device_;
    VkPipeline pipeline_{VK_NULL_HANDLE};
    VkPipelineLayout layout_{VK_NULL_HANDLE};
    std::vector<VkShaderModule> shaderModules_;
};
