#pragma once
#include "X11/Xlib.h"
#include "string_view"

namespace Swift
{
    class WindowUtil
    {
       public:
        WindowUtil();
        ~WindowUtil();

        void PollEvents();
        void SetWindowTitle(std::string_view title) const;

        [[nodiscard]] bool IsRunning() const { return mRunning; }
        [[nodiscard]] Display* GetDisplay() const;
        [[nodiscard]] Window GetWindow() const;

       private:
        bool mRunning = true;
    };
}  // namespace Swift