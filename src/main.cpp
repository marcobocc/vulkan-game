#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/logger.h>
#include <log4cxx/patternlayout.h>
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
        auto graphicsBackend = VulkanGraphicsBackend(window);
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            graphicsBackend.renderFrame();
        }
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
