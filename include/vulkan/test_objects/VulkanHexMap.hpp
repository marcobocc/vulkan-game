#pragma once
#include <cmath>
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>
#include "ecs/components/HexMapComponent.hpp"
#include "vulkan/VulkanBuffer.hpp"
#include "vulkan/VulkanPipelinesManager.hpp"
#include "vulkan/VulkanVertexBuffersManager.hpp"

class VulkanHexMap {
public:
    VulkanHexMap(const HexMapComponent& hexMap,
                 VulkanPipelinesManager* pipelinesManager,
                 VulkanVertexBuffersManager* vertexBuffersManager) :
        pipelinesManager_(pipelinesManager),
        vertexBuffersManager_(vertexBuffersManager) {

        generateVertices(hexMap);
        vertexCount_ = vertices_.size() / 5; // x,y + r,g,b
    }

    ~VulkanHexMap() = default;

    size_t getVertexCount() const { return vertexCount_; }

    void updateVertices(const HexMapComponent& hexMap) {
        generateVertices(hexMap);
        // TODO: update buffer (map/unmap or recreate)
    }

    void render(VkCommandBuffer cmd) const {
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
        auto* hexPipelineObj = pipelinesManager_->createOrGetPipeline(
                "hexmap", hexmapVertexInputInfo, "shaders/hexmap.vert.spv", "shaders/hexmap.frag.spv");
        VkPipeline hexPipeline = hexPipelineObj->getVkPipeline();
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, hexPipeline);

        VkBuffer vertexBuffer =
                vertexBuffersManager_
                        ->createOrGetVertexBuffer("hexmap", vertices_.data(), sizeof(float) * vertices_.size())
                        ->getVkBuffer();

        std::array<VkDeviceSize, 1> offsets = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, offsets.data());
        vkCmdDraw(cmd, static_cast<uint32_t>(vertexCount_), 1, 0, 0);
    }

private:
    static constexpr float HEX_SIZE = 0.1f;
    static constexpr int HEX_VERTICES = 6;

    static std::vector<glm::vec2> hexCorners(float size) {
        std::vector<glm::vec2> corners;
        for (int i = 0; i < HEX_VERTICES; ++i) {
            float angle = glm::radians(60.0f * i - 30.0f);
            corners.emplace_back(size * std::cos(angle), size * std::sin(angle));
        }
        return corners;
    }

    static glm::vec2 cubeToWorld(const CubeCoords& c, float size) {
        float x = size * (std::sqrt(3.0f) * c.vec().x + std::sqrt(3.0f) / 2.0f * c.vec().z);
        float y = size * (3.0f / 2.0f * c.vec().z);
        return {x, y};
    }

    void generateVertices(const HexMapComponent& hexMap) {
        vertices_.clear();
        auto corners = hexCorners(HEX_SIZE);

        for (const auto& [cube, cell]: hexMap.tiles) {
            glm::vec2 center = cubeToWorld(cube, HEX_SIZE);

            for (int i = 0; i < HEX_VERTICES; ++i) {
                glm::vec2 p0 = center;
                glm::vec2 p1 = center + corners[i];
                glm::vec2 p2 = center + corners[(i + 1) % HEX_VERTICES];

                for (const auto& p: {p0, p1, p2}) {
                    vertices_.push_back(p.x);
                    vertices_.push_back(p.y);
                    vertices_.push_back(cell.color.r / 255.0f);
                    vertices_.push_back(cell.color.g / 255.0f);
                    vertices_.push_back(cell.color.b / 255.0f);
                }
            }
        }

        vertexCount_ = vertices_.size() / 5;
    }

    VulkanPipelinesManager* pipelinesManager_;
    VulkanVertexBuffersManager* vertexBuffersManager_;
    std::vector<float> vertices_;
    size_t vertexCount_ = 0;
};
