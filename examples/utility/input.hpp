#pragma once
#include "unordered_map"
#include "window.hpp"
#include "glm/vec2.hpp"

enum class KeyboardKey
{
    eNone,
    eEscape,
    eEnter,
    eSpace,
    eLeftShift,
    eRightShift,
    eLeftControl,
    eRightControl,
    eLeftAlt,
    eRightAlt,
    eLeftSuper,
    eRightSuper,
    eBackspace,
    eTab,
    eCapslock,
    eLeft,
    eRight,
    eUp,
    eDown,
    eA,
    eB,
    eC,
    eD,
    eE,
    eF,
    eG,
    eH,
    eI,
    eJ,
    eK,
    eL,
    eM,
    eN,
    eO,
    eP,
    eQ,
    eR,
    eS,
    eT,
    eU,
    eV,
    eW,
    eX,
    eY,
    eZ,
    e0,
    e1,
    e2,
    e3,
    e4,
    e5,
    e6,
    e7,
    e8,
    e9,
    eF1,
    eF2,
    eF3,
    eF4,
    eF5,
    eF6,
    eF7,
    eF8,
    eF9,
    eF10,
    eF11,
    eF12,
};

enum class MouseButton
{
    eLeft,
    eRight,
    eMiddle,
    eThumb1,
    eThumb2,
};

class Input
{
public:
    explicit Input(Window& window);
    void Tick();

    [[nodiscard]] bool IsKeyPressed(KeyboardKey key);
    [[nodiscard]] bool IsKeyReleased(KeyboardKey key);
    [[nodiscard]] bool IsKeyHeld(KeyboardKey key);
    [[nodiscard]] bool IsMouseButtonPressed(MouseButton button);
    [[nodiscard]] bool IsMouseButtonReleased(MouseButton button);
    [[nodiscard]] bool IsMouseButtonHeld(MouseButton button);

    [[nodiscard]] glm::vec2 GetMouseDelta() const { return m_mouse_position - m_prev_mouse_position; }

private:
    Window& m_window;
    glm::vec2 m_mouse_position{};
    glm::vec2 m_prev_mouse_position{};
    std::unordered_map<int, bool> m_mouse_states{};
    std::unordered_map<int, bool> m_prev_mouse_states{};
    std::unordered_map<int, bool> m_key_states{};
    std::unordered_map<int, bool> m_prev_key_states{};
};
