#pragma once
#include "GLFW/glfw3.h"
#include "glm/vec2.hpp"
#include "array"
#include "functional"

class Window
{
public:
    Window();
    ~Window();

    [[nodiscard]] bool IsRunning() const;

    void PollEvents() const;
    [[nodiscard]] void* GetNativeWindow() const;
    [[nodiscard]] void* GetNativeDisplay() const;
    [[nodiscard]] GLFWwindow* GetHandle() const;
    [[nodiscard]] glm::uvec2 GetSize() const;
    void AddResizeCallback(const std::function<void(glm::uvec2)>& callback) { m_resize_callbacks.push_back(callback); }

    void LockMouse(bool toggle);
    [[nodiscard]] bool IsMouseLocked() const { return m_is_locked; }

private:
    friend class Input;
    std::vector<std::function<void(glm::uvec2)>> m_resize_callbacks;
    GLFWwindow* m_window;
    bool m_is_locked;
};
