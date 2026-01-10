#include "window.hpp"
#ifdef SWIFT_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#endif
#ifdef SWIFT_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#include "GLFW/glfw3native.h"
#endif

Window::Window()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE , GLFW_TRUE);
    m_window = glfwCreateWindow(1280, 720, "Window", nullptr, nullptr);
}

Window::~Window()
{
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool Window::IsRunning() const
{
    return !glfwWindowShouldClose(m_window);
}

void Window::PollEvents() const
{
    glfwPollEvents();
}

void * Window::GetNativeWindow() const
{
#ifdef SWIFT_WINDOWS
    return glfwGetWin32Window(m_window);
#endif
}

void * Window::GetNativeDisplay() const
{
#ifdef SWIFT_WINDOWS
    return nullptr;
#endif
}

GLFWwindow * Window::GetHandle() const
{
    return m_window;
}

glm::uvec2 Window::GetSize() const
{
    int width, height;
    glfwGetWindowSize(m_window, &width, &height);
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

void Window::LockMouse(const bool toggle)
{
    m_is_locked = toggle;
    if (toggle)
    {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else
    {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}
