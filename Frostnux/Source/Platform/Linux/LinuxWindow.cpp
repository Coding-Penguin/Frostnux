#include <fxpch.h>
#include "LinuxWindow.h"

#ifdef FX_PLATFORM_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#define _NET_WM_WINDOW_TYPE
#define _NET_WM_WINDOW_TYPE_UTILITY
#define _NET_WM_WINDOW_TYPE_TOOLBAR
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#ifdef FX_PLATFORM_LINUX
	#include <X11/Xlib.h>
	#include <X11/Xatom.h>
#endif

#include "Frostnux/Log.h"
#include "Frostnux/Events/ApplicationEvent.h"
#include "Frostnux/Events/KeyEvent.h"
#include "Frostnux/Events/MouseEvent.h"

#include "Frostnux/Application.h"

#include <stb_image.h>

namespace Frostnux {

	static bool s_GLFWInitialized = false;

	static void GLFWErrorCallback(int error, const char* description)
	{
		FX_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}

	LinuxWindow::LinuxWindow(const WindowProps& props)
	{
		Init(props);
	}

	LinuxWindow::~LinuxWindow()
	{
		Shutdown();
	}

	void LinuxWindow::Init(const WindowProps& props)
	{
		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;

		m_Data.WindowPtr = this;

		m_CursorArrow = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
		m_CursorHResize = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
		m_CursorVResize = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
		m_CursorNWSE = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
		m_CursorNESW = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);

		FX_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

		if (!s_GLFWInitialized)
		{
			int success = glfwInit();
			FX_CORE_ASSERT(success, "Could not initialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);
			s_GLFWInitialized = true;
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
		if (ChannelManager::GetChannel() == Channel::Preview)
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

		GLFWwindow* sharedContext = nullptr;

		auto* app = Application::GetPointer();
		if (app && app->GetWindow().GetNativeWindow())
		{
			sharedContext = static_cast<GLFWwindow*>(app->GetWindow().GetNativeWindow());
		}
		m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, m_Data.Title.c_str(), nullptr, sharedContext);
		glfwMakeContextCurrent(m_Window);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		FX_CORE_ASSERT(status, "Could not initialize Glad!");
		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetVSync(true);

		SetWindowIcon(m_Window);

		// Set GLFW callbacks
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				data.Width = width;
				data.Height = height;
				WindowResizeEvent event(width, height);
				if (data.EventCallback)
					data.EventCallback(event);
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				WindowCloseEvent event;
				if (data.EventCallback)
					data.EventCallback(event);
			});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				switch (action)
				{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(key, 0, mods);
					if (data.EventCallback) data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					if (data.EventCallback) data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, 1, mods);
					if (data.EventCallback) data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				LinuxWindow* self = (LinuxWindow*)data.WindowPtr;
				double xpos, ypos;
				glfwGetCursorPos(window, &xpos, &ypos);

				if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
				{
					int edge = self->GetResizeEdge((float)xpos, (float)ypos);
					if (edge != 0)
					{
						self->StartResizeWindow(edge, (int)xpos, (int)ypos);
						return;
					}
				}
				else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
				{
					if (self->m_IsResizingWindow)
					{
						self->StopResizeWindow();
						return;
					}
				}


				switch (action)
				{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button, xpos, ypos);
					if (data.EventCallback)
						data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button, xpos, ypos);
					if (data.EventCallback)
						data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				int mods = 0;
				if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
					mods |= GLFW_MOD_CONTROL;
				MouseScrolledEvent event((float)xOffset, (float)yOffset, mods);
				if (data.EventCallback)
					data.EventCallback(event);
			});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				LinuxWindow* self = (LinuxWindow*)data.WindowPtr;
				if (self->m_IsResizingWindow)
				{
					self->DoResizeWindow((int)xPos, (int)yPos);
				}
				else
				{
					int edge = self->GetResizeEdge((float)xPos, (float)yPos);
					self->UpdateResizeCursor(edge);
				}

				int edge = self->GetResizeEdge((float)xPos, (float)yPos);
				self->UpdateResizeCursor(edge);

				MouseMovedEvent event((float)xPos, (float)yPos);
				if (data.EventCallback) data.EventCallback(event);
			});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int codepoint)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				CharEvent event(codepoint);
				if (data.EventCallback)
					data.EventCallback(event);
			});
	}

	void LinuxWindow::Shutdown()
	{
		glfwDestroyWindow(m_Window);
	}

	void LinuxWindow::OnUpdate()
	{
		glfwPollEvents();
		glfwSwapBuffers(m_Window);
	}

	unsigned int LinuxWindow::GetWidth() const { return m_Data.Width; }
	unsigned int LinuxWindow::GetHeight() const { return m_Data.Height; }

	void LinuxWindow::SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; }

	void LinuxWindow::SetVSync(bool enabled)
	{
		m_Data.VSync = enabled;
		glfwSwapInterval(enabled ? 1 : 0);
	}

	bool LinuxWindow::IsVSync() const { return m_Data.VSync; }

	void LinuxWindow::SetUnsemi_transparency(unsigned int value)
	{
#ifdef FX_PLATFORM_LINUX
		m_Data.unsemi_transparency = value;

		Display* display = glfwGetX11Display();
		Window x11Window = glfwGetX11Window(m_Window);
		if (!display || !x11Window) return;

		unsigned long Value = (unsigned long)(value * 0xFFFFFFFF);

		Atom atom = XInternAtom(display, "_NET_WM_WINDOW_OPACITY", False);
		if (atom != None)
		{
			XChangeProperty(display, x11Window, atom, XA_CARDINAL, 32,
				PropModeReplace, (unsigned char*)&Value, 1);
			XFlush(display);
		}
#endif
	}

	int LinuxWindow::GetResizeEdge(float mx, float my) const
	{
		if (glfwGetWindowAttrib(m_Window, GLFW_MAXIMIZED))
			return 0;

		const int edgeSize = 5;
		int w = m_Data.Width, h = m_Data.Height;
		bool onLeft = (mx <= edgeSize);
		bool onRight = (mx >= w - edgeSize);
		bool onTop = (my <= edgeSize);
		bool onBottom = (my >= h - edgeSize);

		if (onTop && my < 30) return 0;

		if (onLeft && onTop) return 5;
		if (onRight && onTop) return 6;
		if (onLeft && onBottom) return 7;
		if (onRight && onBottom) return 8;

		if (onLeft) return 1;
		if (onRight) return 2;
		if (onTop) return 3;
		if (onBottom) return 4;
		return 0;
	}

	void LinuxWindow::UpdateResizeCursor(int edge)
	{
		GLFWcursor* cursor = m_CursorArrow;
		switch (edge)
		{
		case 1: case 2: cursor = m_CursorHResize; break;
		case 3: case 4: cursor = m_CursorVResize; break;
		case 5: case 8: cursor = m_CursorNWSE; break;
		case 6: case 7: cursor = m_CursorNESW; break;
		default: cursor = m_CursorArrow; break;
		}
		glfwSetCursor(m_Window, cursor);
	}

	void LinuxWindow::StartResizeWindow(int edge, int mouseX, int mouseY)
	{
		m_IsResizingWindow = true;
		m_ResizeEdgeType = edge;
		m_ResizeStartMouseX = mouseX;
		m_ResizeStartMouseY = mouseY;
		glfwGetWindowPos(m_Window, &m_ResizeStartWindowX, &m_ResizeStartWindowY);
		m_ResizeStartWindowW = m_Data.Width;
		m_ResizeStartWindowH = m_Data.Height;
	}

	void LinuxWindow::DoResizeWindow(int mouseX, int mouseY)
	{
		if (!m_IsResizingWindow) return;
		int dx = mouseX - m_ResizeStartMouseX;
		int dy = mouseY - m_ResizeStartMouseY;
		int newX = m_ResizeStartWindowX;
		int newY = m_ResizeStartWindowY;
		int newW = m_ResizeStartWindowW;
		int newH = m_ResizeStartWindowH;

		switch (m_ResizeEdgeType)
		{
		case 1:
			newW = m_ResizeStartWindowW - dx;
			newX = m_ResizeStartWindowX + dx;
			break;
		case 2:
			newW = m_ResizeStartWindowW + dx;
			break;
		case 3:
			newH = m_ResizeStartWindowH - dy;
			newY = m_ResizeStartWindowY + dy;
			break;
		case 4:
			newH = m_ResizeStartWindowH + dy;
			break;
		case 5:
			newW = m_ResizeStartWindowW - dx;
			newH = m_ResizeStartWindowH - dy;
			newX = m_ResizeStartWindowX + dx;
			newY = m_ResizeStartWindowY + dy;
			break;
		case 6:
			newW = m_ResizeStartWindowW + dx;
			newH = m_ResizeStartWindowH - dy;
			newY = m_ResizeStartWindowY + dy;
			break;
		case 7:
			newW = m_ResizeStartWindowW - dx;
			newH = m_ResizeStartWindowH + dy;
			newX = m_ResizeStartWindowX + dx;
			break;
		case 8:
			newW = m_ResizeStartWindowW + dx;
			newH = m_ResizeStartWindowH + dy;
			break;
		}

		glfwSetWindowSize(m_Window, newW, newH);

		if (newX != m_ResizeStartWindowX || newY != m_ResizeStartWindowY)
		{
			glfwSetWindowPos(m_Window, newX, newY);
		}
		SettingsManager::Get().SetWindowSize(newW, newH);
	}

	void LinuxWindow::StopResizeWindow()
	{
		m_IsResizingWindow = false;
	}

	void LinuxWindow::SetWindowIcon(GLFWwindow* window)
	{
		int width, height, channels;
		std::string iconPath;
		iconPath = "Resources/Images/Frostnux.png";

		unsigned char* data = stbi_load(iconPath.c_str(), &width, &height, &channels, 4);

		if (!data)
		{
			FX_CORE_WARN("Failed to load icon!");
			return;
		}

		GLFWimage image;
		image.width = width;
		image.height = height;
		image.pixels = data;
		glfwSetWindowIcon(window, 1, &image);
	}

	void LinuxWindow::Close()
	{
		glfwSetWindowShouldClose(m_Window, GLFW_TRUE);
	}

	bool LinuxWindow::ShouldClose() const
	{
		return glfwWindowShouldClose(m_Window) == GLFW_TRUE;
	}

	void LinuxWindow::Maximize()
	{
		glfwMaximizeWindow(m_Window);
	}

}
