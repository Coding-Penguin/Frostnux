#include <fxpch.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include "Application.h"
#include "Events/ApplicationEvent.h"
#include "Log.h"
#include "Window.h"
#include "ui/uiTools/uiWindow.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <glad/glad.h>

#ifdef FX_PLATFORM_WINDOWS
#include "Platform/Windows/WindowsWindow.h"
#elif define(FX_PLATFORM_LINUX)
#include "Platform/Linux/LinuxWindow.h"
#endif

#include "Input.h"
#include "SettingsManager.h"

namespace Frostnux {

#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		FX_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		auto& settings = SettingsManager::Get().GetSettings();

		Log::Init();
		FX_CORE_INFO("Initilized log!");

		SettingsManager::Get().Load();

		if (settings.themeIndex < 0 || settings.themeIndex >= ThemeManager::GetThemeCount())
		{
			FX_CORE_WARN("Invalid theme index in settings: {}. Defaulting to Dark theme.", settings.themeIndex);
			ThemeManager::SetTheme(Theme::Dark);
			SettingsManager::Get().SetThemeIndex((int)Theme::Dark);
		}
		else
		{
			ThemeManager::SetTheme((Theme)settings.themeIndex);
		}
		ChannelManager::SetChannel((Channel)settings.channelIndex);
		LanguageManager::SetLanguage((LanguageType)settings.languageIndex);

		FX_CORE_INFO("Frostnux is running on channel: {}, theme: {}, language: {}, version: {}",
			((ChannelManager::GetChannel() == Channel::Current) ? "Current" : "Preview"),
			ThemeManager::GetThemeName(),
			LanguageManager::GetLanguageName(),
			s_Version);

		std::string path = "Resources/Languages/" + LanguageManager::GetLanguageCode() + ".json";
		std::ifstream file(path);
		nlohmann::json j;
		file >> j;
		std::string Name = j.value("Name", "Frostnux");
		std::string WindowName = Name + " - " + s_Version;
		if (ChannelManager::GetChannel() == Channel::Preview)
		{
			WindowName += " [" + j.value("Preview", "Preview") + "]";
		}
		float width = settings.WindowWidth, height = settings.WindowHeight;
		if (width <= 0 || height <= 0)
		{
			width = 1280;
			height = 720;
		}
		WindowProps props(WindowName, width, height);
		m_MainWindow = std::unique_ptr<Window>(Window::Create(props));
		m_MainWindow->SetEventCallback(BIND_EVENT_FN(OnEvent));
		m_MainWindow->SetVSync(true);
		if (SettingsManager::Get().GetSettings().IsMaximize)
		{
			m_MainWindow->Maximize();
		}

		Input::Init();

		if (!s_FontLoaded)
		{
			FX_CORE_INFO("Loading font...");
			TextRenderer::Get().LoadFont("Resources/Fonts/SourceHanSansSC-Normal.otf", s_FontSize);
			s_FontLoaded = true;
			FX_CORE_INFO("Font loaded successfully.");
		}
	}

	Application::~Application()
	{
		TextRenderer::Get().Unload();

		m_MainWindow.reset();

		FX_INFO("Application destructor called.");
		FX_CORE_WARN("Shutting down Frostnux Studio...");
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* overlay)
	{
		m_LayerStack.PushOverlay(overlay);
		overlay->OnAttach();
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));
		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
		{
			(*--it)->OnEvent(e);
			if (e.m_Handled)
				break;
		}
	}

	void Application::Run()
	{
		FX_TRACE("Frostnux Studio initialized and running.");

		float lastTime = (float)glfwGetTime();
		do 
		{
			float currentTime = (float)glfwGetTime();
			float deltaTime = currentTime - lastTime;
			lastTime = currentTime;

			auto [r, g, b] = ThemeManager::GetBGColor();
			glClearColor(r, g, b, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			for (Layer* layer : m_LayerStack)
			{
				if (layer)
				{
					layer->OnUpdate(deltaTime);
				}
			}

			if (m_MainWindow)
			{
				m_MainWindow->SetUnsemi_transparency(unsemi_transparency);
				m_MainWindow->OnUpdate();
			}
			else
			{
				FX_CORE_ERROR("Main window is null!");
				m_Running = false;
			}

			std::vector<size_t> closedIndices;
			for (size_t i = 0; i < m_Windows.size(); ++i)
			{
				auto& win = m_Windows[i];

				if (win->ShouldClose())
				{
					closedIndices.push_back(i);
					continue;
				}

				win->OnUpdate();
			}

			for (size_t i = closedIndices.size(); i > 0; --i)
			{
				size_t idx = closedIndices[i - 1];

				m_Windows.erase(m_Windows.begin() + idx);
			}
		} while (m_Running && !m_MainWindow->ShouldClose());

		for (auto& win : m_Windows)
		{
			win->Close();
		}

		m_Windows.clear();
		m_MainWindow = nullptr;
	}

	bool Application::OnWindowClose(WindowCloseEvent& e) 
	{
		FX_WARN("Window close event received. Shutting down Application...");
		m_Running = false;
		SettingsManager::Get().Save();
		return true;
	}

	Window* Application::CreateGLFWWindow(const WindowProps& props)
	{
		auto newWindow = std::unique_ptr<Window>(Window::Create(props));

		newWindow->SetEventCallback(BIND_EVENT_FN(OnEvent));
		Window* windowPtr = newWindow.get();
		m_Windows.push_back(std::move(newWindow));
		return windowPtr;
	}

}
