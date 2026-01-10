#pragma once
#include "swift.hpp"
#include "window.hpp"

class Imgui
{
public:
    Imgui(const std::shared_ptr<Swift::IContext> &context, const Window &window);
    ~Imgui();
    void BeginFrame();
    void Render(const std::shared_ptr<Swift::ICommand> &command);

private:
    void SetupStyle();
};
