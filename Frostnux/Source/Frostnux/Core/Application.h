#pragma once

#include "Frostnux/Events/Event.h"
#include "Frostnux/Events/ApplicationEvent.h"
#include "Window.h"
#include "LayerStack.h"

namespace Frostnux {

	class Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(class Layer* layer);
		void PushOverlay(class Layer* overlay);

		Window* CreateGLFWWindow(const WindowProps& props);

		inline static Application* GetPointer() { return s_Instance; }
		inline static Application& Get() { return *s_Instance; }
		inline Window& GetWindow() { return *m_MainWindow; }
		inline Window* GetWindowPointer() { return m_MainWindow.get(); }
	private:
		bool OnWindowClose(WindowCloseEvent& e);

		std::unique_ptr<class Window> m_MainWindow;
		std::vector<std::unique_ptr<Window>> m_Windows;
		bool m_Running = true;
		LayerStack m_LayerStack;
		static Application* s_Instance;
		double unsemi_transparency = 0.99f;
	};

	// To be defined in CLIENT 
	Frostnux::Application* CreateApplication();

}
