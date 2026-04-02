#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/logger.h>
#include <log4cxx/patternlayout.h>
#include "ecs/components/MaterialComponent.hpp"
#include "ecs/components/MeshComponent.hpp"
#include "tilemap_test/HexMapGenerator.hpp"
#include "vulkan/VulkanGraphicsBackend.hpp"
#include "vulkan/test_objects/HexMapBuilder.hpp"
#include "vulkan/test_objects/TriangleBuilder.hpp"

using namespace log4cxx;

int main() {
    const LayoutPtr layout(new PatternLayout("%d [%t] %-5p %c (%F:%L) - %m%n"));
    const AppenderPtr console(new ConsoleAppender(layout));
    Logger::getRootLogger()->addAppender(console);
    static const LoggerPtr logger = Logger::getLogger("GameEngine");

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "GameEngine", nullptr, nullptr);
    if (!window)return -1;

    {
        auto graphicsBackend = VulkanGraphicsBackend(window);

        auto triangleMesh = buildTriangleMesh();
        auto triangleMaterial = buildTriangleMaterial();
        auto hexMapMesh = buildHexMapMesh(createDemoHexMap());
        auto hexMapMaterial = buildHexMapMaterial();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            graphicsBackend.draw(triangleMesh, triangleMaterial);
            graphicsBackend.draw(hexMapMesh, hexMapMaterial);
            graphicsBackend.renderFrame();
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
