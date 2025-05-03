#include "window.hpp"

namespace Swift
{
    int mScreen;
    Display* g_display;
    Window g_window;
    Atom wm_delete_message;

    WindowUtil::WindowUtil()
    {
        g_display = XOpenDisplay(nullptr);
        mScreen = DefaultScreen(g_display);
        g_window = XCreateSimpleWindow(
            g_display,
            RootWindow(g_display, mScreen),
            10,
            10,
            1280,
            720,
            1,
            BlackPixel(g_display, mScreen),
            WhitePixel(g_display, mScreen));
        XSelectInput(g_display, g_window, ExposureMask | KeyPressMask | StructureNotifyMask);
        XMapWindow(g_display, g_window);
        wm_delete_message = XInternAtom(g_display, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(g_display, g_window, &wm_delete_message, 1);
    }

    WindowUtil::~WindowUtil() { XDestroyWindow(g_display, g_window); }

    void WindowUtil::PollEvents()
    {
        while (XPending(g_display))
        {
            XEvent event;
            XNextEvent(g_display, &event);

            switch (event.type)
            {
                case ClientMessage:
                    if (event.xclient.data.l[0] == static_cast<int64_t>(wm_delete_message)) mRunning = false;
                    break;
                default:;
            }
        }
    }

    void WindowUtil::SetWindowTitle(const std::string_view title) const { XStoreName(g_display, g_window, title.data()); }

    Display* WindowUtil::GetDisplay() const { return g_display; }

    Window WindowUtil::GetWindow() const { return g_window; }
}  // namespace Swift