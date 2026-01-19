#include "input.hpp"

int ToGLFWKey(const KeyboardKey key)
{
    switch (key)
    {
        case KeyboardKey::eNone:
            return GLFW_KEY_UNKNOWN;
        case KeyboardKey::eEscape:
            return GLFW_KEY_ESCAPE;
        case KeyboardKey::eEnter:
            return GLFW_KEY_ENTER;
        case KeyboardKey::eSpace:
            return GLFW_KEY_SPACE;
        case KeyboardKey::eLeftShift:
            return GLFW_KEY_LEFT_SHIFT;
        case KeyboardKey::eRightShift:
            return GLFW_KEY_RIGHT_SHIFT;
        case KeyboardKey::eLeftControl:
            return GLFW_KEY_LEFT_CONTROL;
        case KeyboardKey::eRightControl:
            return GLFW_KEY_RIGHT_CONTROL;
        case KeyboardKey::eLeftAlt:
            return GLFW_KEY_LEFT_ALT;
        case KeyboardKey::eRightAlt:
            return GLFW_KEY_RIGHT_ALT;
        case KeyboardKey::eLeftSuper:
            return GLFW_KEY_LEFT_SUPER;
        case KeyboardKey::eRightSuper:
            return GLFW_KEY_RIGHT_SUPER;
        case KeyboardKey::eBackspace:
            return GLFW_KEY_BACKSPACE;
        case KeyboardKey::eTab:
            return GLFW_KEY_TAB;
        case KeyboardKey::eCapslock:
            return GLFW_KEY_CAPS_LOCK;
        case KeyboardKey::eLeft:
            return GLFW_KEY_LEFT;
        case KeyboardKey::eRight:
            return GLFW_KEY_RIGHT;
        case KeyboardKey::eUp:
            return GLFW_KEY_UP;
        case KeyboardKey::eDown:
            return GLFW_KEY_DOWN;
        case KeyboardKey::eA:
            return GLFW_KEY_A;
        case KeyboardKey::eB:
            return GLFW_KEY_B;
        case KeyboardKey::eC:
            return GLFW_KEY_C;
        case KeyboardKey::eD:
            return GLFW_KEY_D;
        case KeyboardKey::eE:
            return GLFW_KEY_E;
        case KeyboardKey::eF:
            return GLFW_KEY_F;
        case KeyboardKey::eG:
            return GLFW_KEY_G;
        case KeyboardKey::eH:
            return GLFW_KEY_H;
        case KeyboardKey::eI:
            return GLFW_KEY_I;
        case KeyboardKey::eJ:
            return GLFW_KEY_J;
        case KeyboardKey::eK:
            return GLFW_KEY_K;
        case KeyboardKey::eL:
            return GLFW_KEY_L;
        case KeyboardKey::eM:
            return GLFW_KEY_M;
        case KeyboardKey::eN:
            return GLFW_KEY_N;
        case KeyboardKey::eO:
            return GLFW_KEY_O;
        case KeyboardKey::eP:
            return GLFW_KEY_P;
        case KeyboardKey::eQ:
            return GLFW_KEY_Q;
        case KeyboardKey::eR:
            return GLFW_KEY_R;
        case KeyboardKey::eS:
            return GLFW_KEY_S;
        case KeyboardKey::eT:
            return GLFW_KEY_T;
        case KeyboardKey::eU:
            return GLFW_KEY_U;
        case KeyboardKey::eV:
            return GLFW_KEY_V;
        case KeyboardKey::eW:
            return GLFW_KEY_W;
        case KeyboardKey::eX:
            return GLFW_KEY_X;
        case KeyboardKey::eY:
            return GLFW_KEY_Y;
        case KeyboardKey::eZ:
            return GLFW_KEY_Z;

        case KeyboardKey::e0:
            return GLFW_KEY_0;
        case KeyboardKey::e1:
            return GLFW_KEY_1;
        case KeyboardKey::e2:
            return GLFW_KEY_2;
        case KeyboardKey::e3:
            return GLFW_KEY_3;
        case KeyboardKey::e4:
            return GLFW_KEY_4;
        case KeyboardKey::e5:
            return GLFW_KEY_5;
        case KeyboardKey::e6:
            return GLFW_KEY_6;
        case KeyboardKey::e7:
            return GLFW_KEY_7;
        case KeyboardKey::e8:
            return GLFW_KEY_8;
        case KeyboardKey::e9:
            return GLFW_KEY_9;
        case KeyboardKey::eF1:
            return GLFW_KEY_F1;
        case KeyboardKey::eF2:
            return GLFW_KEY_F2;
        case KeyboardKey::eF3:
            return GLFW_KEY_F3;
        case KeyboardKey::eF4:
            return GLFW_KEY_F4;
        case KeyboardKey::eF5:
            return GLFW_KEY_F5;
        case KeyboardKey::eF6:
            return GLFW_KEY_F6;
        case KeyboardKey::eF7:
            return GLFW_KEY_F7;
        case KeyboardKey::eF8:
            return GLFW_KEY_F8;
        case KeyboardKey::eF9:
            return GLFW_KEY_F9;
        case KeyboardKey::eF10:
            return GLFW_KEY_F10;
        case KeyboardKey::eF11:
            return GLFW_KEY_F11;
        case KeyboardKey::eF12:
            return GLFW_KEY_F12;

        default:
            return GLFW_KEY_UNKNOWN;
    }
}

Input::Input(Window& window) : m_window(window)
{
    glfwSetWindowUserPointer(window.GetHandle(), this);
    glfwSetCursorPosCallback(window.GetHandle(),
                             [](GLFWwindow* window, double x_pos, double y_pos)
                             {
                                 auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
                                 input->m_mouse_position = {x_pos, -y_pos};
                             });
    glfwSetKeyCallback(window.GetHandle(),
                       [](GLFWwindow* window, const int key, int scancode, const int action, int mods)
                       {
                           auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
                           input->m_key_states[key] = action;
                       });

    glfwSetMouseButtonCallback(window.GetHandle(),
                               [](GLFWwindow* window, int button, int action, int mods)
                               {
                                   auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
                                   input->m_mouse_states[button] = action;
                               });

    glfwSetWindowSizeCallback(m_window.GetHandle(),
                              [](GLFWwindow* window, int w, int h)
                              {
                                  auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
                                  for (auto& callback : input->m_window.m_resize_callbacks)
                                  {
                                      callback({w, h});
                                  }
                              });
}

void Input::Tick()
{
    m_prev_mouse_position = m_mouse_position;
    m_prev_key_states = m_key_states;
    m_prev_mouse_states = m_mouse_states;

    for (auto& [key, state] : m_key_states)
    {
        m_prev_key_states[key] = state;
    }
    for (auto& [key, state] : m_mouse_states)
    {
        m_prev_mouse_states[key] = state;
    }
}

bool Input::IsKeyPressed(const KeyboardKey key)
{
    const auto v_key = ToGLFWKey(key);
    return m_key_states[v_key] && !m_prev_key_states[v_key];
}

bool Input::IsKeyReleased(const KeyboardKey key)
{
    const auto v_key = ToGLFWKey(key);
    return !m_key_states[v_key] && m_prev_key_states[v_key];
}

bool Input::IsKeyHeld(const KeyboardKey key)
{
    const auto v_key = ToGLFWKey(key);
    return m_key_states[v_key];
}

bool Input::IsMouseButtonPressed(MouseButton button)
{
    return m_mouse_states[static_cast<int>(button)] && !m_prev_mouse_states[static_cast<int>(button)];
}

bool Input::IsMouseButtonReleased(MouseButton button)
{
    return !m_mouse_states[static_cast<int>(button)] && m_prev_mouse_states[static_cast<int>(button)];
}

bool Input::IsMouseButtonHeld(MouseButton button) { return m_mouse_states[static_cast<int>(button)]; }
