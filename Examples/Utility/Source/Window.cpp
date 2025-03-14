#include "Window.hpp"
#include "Input.hpp"
#include "GLFW/glfw3.h"


namespace
{
    struct WindowData
    {
        GLFWwindow* window;
        glm::uvec2 size;
        glm::vec2 cursorPos;
        std::unordered_map<Input::MouseButton, bool> mButtonsDown;
        std::unordered_map<Input::KeyboardKey, bool> mKeysDown;

        operator GLFWwindow*() const { return window; }
    } gWindow{};
} // namespace

namespace Window
{
    void Init()
    {
        glfwInitHint(GLFW_PLATFORM, GLFW_ANY_PLATFORM);
        if (!glfwInit()) assert(false);
            
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        gWindow.window = glfwCreateWindow(1280, 720, "Example", nullptr, nullptr);
        glfwGetWindowSize(gWindow, reinterpret_cast<int*>(&gWindow.size.x), reinterpret_cast<int*>(&gWindow.size.y));
        glfwSetWindowUserPointer(gWindow, &gWindow);
        glfwSetWindowSizeCallback(gWindow,
                                  [](GLFWwindow* glfwWindow, const int windowWidth, const int windowHeight)
                                  {
                                      auto* window = static_cast<WindowData*>(glfwGetWindowUserPointer(glfwWindow));
                                      window->size = glm::uvec2(windowWidth, windowHeight);
                                  });
    }

    void Shutdown()
    {
        glfwDestroyWindow(gWindow);
        glfwTerminate();
    }
    
    void PollEvents()
    {
        glfwPollEvents();
    }

    glm::uvec2 GetSize() { return gWindow.size; }
    
    GLFWwindow* GetWindow()
    {
        return gWindow.window;
    }

    bool IsRunning() { return !glfwWindowShouldClose(gWindow); }
} // namespace Utility
