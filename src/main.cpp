#include <GLFW/glfw3.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/logger.h>
#include <log4cxx/patternlayout.h>
#include "vulkan/VulkanGraphicsBackend.hpp"

using namespace log4cxx;

auto main() -> int {
    // -------------------------------
    // Logger setup
    // -------------------------------
    const LayoutPtr layout(new PatternLayout("%d [%t] %-5p %c (%F:%L) - %m%n"));
    const AppenderPtr console(new ConsoleAppender(layout));
    Logger::getRootLogger()->addAppender(console);
    static const LoggerPtr logger = Logger::getLogger("GameEngine");

    LOG4CXX_INFO(logger, "Starting GameEngine");

    // -------------------------------
    // Initialize GLFW
    // -------------------------------
    if (!glfwInit()) {
        LOG4CXX_ERROR(logger, "Failed to initialize GLFW");
        return -1;
    }

    glfwSetErrorCallback([](int error, const char* description) {
        LOG4CXX_ERROR(logger, "GLFW Error " << error << ": " << description);
    });

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "GameEngine", nullptr, nullptr);
    if (!window) {
        LOG4CXX_ERROR(logger, "Failed to create GLFW window");
        glfwTerminate();
        return -1;
    }

    // -------------------------------
    // Initialize Graphics Backend
    // -------------------------------
    auto graphicsBackend = VulkanGraphicsBackend();
    if (!graphicsBackend.init()) {
        LOG4CXX_ERROR(logger, "Vulkan graphics backend initialization failed");
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    LOG4CXX_INFO(logger, "Vulkan graphics backend initialized successfully");

    // -------------------------------
    // Main loop
    // -------------------------------
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    // -------------------------------
    // Cleanup
    // -------------------------------
    glfwDestroyWindow(window);
    glfwTerminate();

    LOG4CXX_INFO(logger, "GameEngine terminated cleanly");
    return 0;
}
