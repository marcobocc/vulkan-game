#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/logger.h>
#include <log4cxx/patternlayout.h>
#include "ecs/GameEntitiesManager.hpp"
#include "ecs/components/InputComponent.hpp"
#include "ecs/components/MaterialComponent.hpp"
#include "ecs/components/MeshComponent.hpp"
#include "ecs/systems/InputSystem.hpp"
#include "ecs/systems/RenderSystem.hpp"
#include "test_objects/HexMap.hpp"
#include "test_objects/HexMapComponentsBuilder.hpp"
#include "test_objects/TriangleComponentsBuilder.hpp"
#include "vulkan/VulkanGraphicsBackend.hpp"

using namespace log4cxx;

int main() {
    const LayoutPtr layout(new PatternLayout("%d [%t] %-5p %c (%F:%L) - %m%n"));
    const AppenderPtr console(new ConsoleAppender(layout));
    Logger::getRootLogger()->addAppender(console);
    static const LoggerPtr logger = Logger::getLogger("GameEngine");

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "GameEngine", nullptr, nullptr);
    if (!window) return -1;

    {
        auto ecs = GameEntitiesManager();
        auto triangleEntityId = ecs.createEntity();
        ecs.addComponent(triangleEntityId, buildTriangleMesh());
        ecs.addComponent(triangleEntityId, buildTriangleMaterial());

        auto hexMapEntityId = ecs.createEntity();
        ecs.addComponent(hexMapEntityId, buildHexMapMesh(createDemoHexMap()));
        ecs.addComponent(hexMapEntityId, buildHexMapMaterial());

        auto graphicsBackend = VulkanGraphicsBackend(window);
        auto renderSystem = RenderSystem(ecs, graphicsBackend);

        InputComponent playerInput;
        auto inputSystem = InputSystem(window, playerInput);

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            inputSystem.update();

            if (playerInput.keys[GLFW_KEY_ESCAPE]) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            } else {
                LOG4CXX_INFO(logger, "Mouse position: X=" << playerInput.mouseX << " Y=" << playerInput.mouseY);
            }

            renderSystem.update();
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
