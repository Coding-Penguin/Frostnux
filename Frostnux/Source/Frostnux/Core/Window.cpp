#include <fxpch.h>

#include "Window.h"
#include "Platform/Windows/WindowsWindow.h"
#include "Platform/Linux/LinuxWindow.h"

#include "Log.h"

namespace Frostnux {

    Window* Window::Create(const WindowProps& props)
    {
#ifdef FX_PLATFORM_WINDOWS
        FX_TRACE("Creating Windows window...");
        return new WindowsWindow(props);
#elif defined(FX_PLATFORM_LINUX)
        FX_TRACE("Creating Linux window...");
        return new LinuxWindow(props);
#else
        FX_CORE_ERROR("Frostnux Studio IDE unsupported the platform!");
        return nullptr;
#endif
    }

}