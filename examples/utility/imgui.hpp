#pragma once
#include "swift.hpp"
#include "window.hpp"

class ImguiBackend
{
public:
    ImguiBackend(Swift::IContext* context, const Window& window);
    void Destroy();
    void BeginFrame();
    void Render(Swift::ICommand* command);

private:
    void SetupStyle();
};
