#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/patternlayout.h>
#include <log4cxx/consoleappender.h>
#include <fmt/core.h>
#include <GLFW/glfw3.h>
#include <volk.h>
#include <vector>
#include <vulkan/vulkan.h>

using namespace log4cxx;

int main()
{
    const LayoutPtr layout(new PatternLayout("%d [%t] %-5p %c (%F:%L) - %m%n"));
    const AppenderPtr console(new ConsoleAppender(layout));
    Logger::getRootLogger()->addAppender(console);
    const LoggerPtr logger(Logger::getLogger("GameEngine"));

    LOG4CXX_INFO(logger, "Starting GameEngine");

    if (!glfwInit()) {
        LOG4CXX_ERROR(logger, "Failed to initialize GLFW");
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "GameEngine", nullptr, nullptr);
    if (!window) {
        LOG4CXX_ERROR(logger, "Failed to create GLFW window");
        glfwTerminate();
        return -1;
    }

    if (volkInitialize() != VK_SUCCESS) {
        LOG4CXX_ERROR(logger, "Failed to initialize volk");
        return -1;
    }

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifdef __APPLE__
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "GameEngine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.pEngineName = "GameEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

#ifdef __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkInstance instance;
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        LOG4CXX_ERROR(logger, "Failed to create Vulkan instance");
        return -1;
    }

    volkLoadInstance(instance);
    LOG4CXX_INFO(logger, fmt::format("Vulkan instance created successfully with {} extensions", extensions.size()));

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();

    LOG4CXX_INFO(logger, "GameEngine terminated cleanly");
}