#pragma once
#include "Frostnux/Core/Window.h"

struct GLFWwindow;

namespace Frostnux {

	class LinuxWindow : public Window
	{
	public:
		LinuxWindow(const WindowProps& props);
		virtual ~LinuxWindow();

		void OnUpdate() override;

		unsigned int GetWidth() const override;
		unsigned int GetHeight() const override;

		// Window attributes
		void SetEventCallback(const EventCallbackFn& callback) override;
		void SetVSync(bool enabled) override;
		bool IsVSync() const override;

		inline virtual void* GetNativeWindow() const override { return m_Window; }

		void Close() override;
		bool ShouldClose() const override;

		void SetUnsemi_transparency(unsigned int value);

		void SetWindowIcon(GLFWwindow* window) override;

		void Maximize() override;
	private:
		void Init(const WindowProps& props);
		void Shutdown();
		
		GLFWwindow* m_Window;

		struct WindowData
		{
			double unsemi_transparency;
			void* WindowPtr;
			EventCallbackFn EventCallback;
			std::string Title;

			unsigned int Width, Height;

			bool VSync;
		};

		WindowData m_Data;

		bool m_IsResizingWindow = false;
		int m_ResizeEdgeType = 0;
		int m_ResizeStartMouseX = 0, m_ResizeStartMouseY = 0;
		int m_ResizeStartWindowX = 0, m_ResizeStartWindowY = 0;
		int m_ResizeStartWindowW = 0, m_ResizeStartWindowH = 0;

		GLFWcursor* m_CursorArrow;
		GLFWcursor* m_CursorHResize;
		GLFWcursor* m_CursorVResize;
		GLFWcursor* m_CursorNWSE;
		GLFWcursor* m_CursorNESW;

		int GetResizeEdge(float mx, float my) const;
		void UpdateResizeCursor(int edge);
		void StartResizeWindow(int edge, int mouseX, int mouseY);
		void DoResizeWindow(int mouseX, int mouseY);
		void StopResizeWindow();
	};

}