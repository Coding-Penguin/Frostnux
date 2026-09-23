#pragma once

#include "Core.h"
#include "Events/Event.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Frostnux {

	struct WindowProps
	{
		std::string Title;
		unsigned int Width, Height;

		WindowProps(const std::string& title = "Frostnux Studio IDE", unsigned int width = 1720, unsigned int height = 1000, bool isToolWindow = false)
			: Title(title), Width(width), Height(height)
		{
		}
	};

	class Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() {}
		virtual void OnUpdate() = 0;
		virtual unsigned int GetWidth() const = 0;
		virtual unsigned int GetHeight() const = 0;

		// Window attributes
		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;

		virtual void* GetNativeWindow() const = 0;

		virtual void SetUnsemi_transparency(double value) = 0;

		virtual void SetWindowIcon(GLFWwindow* window) = 0;

		virtual void Close() = 0;
		virtual bool ShouldClose() const = 0;

		virtual void Maximize() = 0;

		static Window* Create(const WindowProps& props = WindowProps());
	};
}
