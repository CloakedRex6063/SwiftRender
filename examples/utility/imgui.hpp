#pragma once
#include "swift.hpp"
#include "window.hpp"

class ImguiBackend
{
public:
    ImguiBackend(const std::shared_ptr<Swift::IContext>& context, const Window& window);
    ~ImguiBackend();
    void BeginFrame();
    void Render(const std::shared_ptr<Swift::ICommand>& command);

private:
    void SetupStyle();
};
