#pragma once
#include "input.hpp"
#include "glm/mat4x4.hpp"
#include "glm/trigonometric.hpp"

class Camera
{
public:
    void Tick(Window& window, Input& input, float delta_time);

    float m_aspect_ratio = 16.f / 9.f;
    float m_fov = glm::radians(90.f);
    float m_near_plane = 0.1f;
    float m_far_plane = 1000.f;
    glm::mat4 m_view_matrix{};
    glm::mat4 m_proj_matrix{};

    glm::vec3 m_position = {0, 0, 5};
    glm::vec3 m_rotation{};
    float m_move_speed = 10.f;
    float m_look_speed = 1.f;

    glm::vec3 GetUpVector() { return m_world_matrix[1]; }
    glm::vec3 GetRightVector() { return m_world_matrix[0]; }
    glm::vec3 GetForwardVector() { return -m_world_matrix[2]; }

private:
    glm::mat4 m_world_matrix{};
    void UpdateKeyboard(Input& input, float delta_time);
    void UpdateMouse(Window& window, Input& input, float delta_time);
};
