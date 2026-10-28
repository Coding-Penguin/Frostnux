#include <fxpch.h>
#include "uiLayer.h"
#include "uiTools/ui.h"
#include "Frostnux/Core/Log.h"
#include "Frostnux/Core/Application.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Frostnux/Managers/SettingsManager.h"
#include "Frostnux/Core/FileDialog.h"
#include "Frostnux/BuildSystem/Process.h"
#include "uiTools/TextInputDialog.h"
#include "Frostnux/Renderer/Renderer2D.h"

namespace Frostnux
{

	GLRenderer uiLayer::m_EditorRenderer;
	CodeEditor uiLayer::m_Editor{&m_EditorRenderer};

	uiLayer &uiLayer::Get()
	{
		static uiLayer instance;
		return instance;
	}

	CodeEditor &uiLayer::GetCodeEditor()
	{
		return m_Editor;
	}

	uiLayer::uiLayer() : Layer("UILayer")
	{
	}

	uiLayer::~uiLayer()
	{
		for (auto *win : m_Windows)
			delete win;
		m_Windows.clear();
		delete m_TitleBar;
		delete m_StatusBar;
		delete m_ShortcutBar;
	}

	void uiLayer::LoadSolution(const std::string &path)
	{
		Solution newSol;
		if (newSol.Load(path))
		{
			m_Solution = std::move(newSol);
			if (m_SolutionExplorer)
				m_SolutionExplorer->SetSolution(&m_Solution);

			FX_CORE_INFO("Loaded Solution: {}", m_Solution.name);
			m_ProjectIndex.SetRoot(path);
			m_ProjectIndex.Scan();
			m_Editor.SetProjectIndex(&m_ProjectIndex);
			FX_CORE_INFO("ProjectIndex: indexed {} files", m_ProjectIndex.FileCount());
		}
		else
		{
			FX_CORE_ERROR("Failed to load solution: {}", path);
		}
	}

	void uiLayer::CreateNewSolution(const std::string &path)
	{
		namespace fs = std::filesystem;

		m_Solution = Solution();
		m_Solution.path = path;
		m_Solution.name = fs::path(path).stem().string();

		auto *proj = m_Solution.AddProject(m_Solution.name, ProjectKind::ConsoleApp);

		auto baseDir = fs::path(path).parent_path() / m_Solution.name;
		std::error_code ec;
		fs::create_directories(baseDir / "src", ec);
		fs::create_directories(baseDir / "include", ec);

		auto mainPath = baseDir / "src" / "main.cpp";

		m_Solution.Save(path);

		if (m_SolutionExplorer)
			m_SolutionExplorer->SetSolution(&m_Solution);

		if (auto *tab = m_Editor.openFile(mainPath.string()))
			tab->invalidateHighlight();

		FX_CORE_INFO("Created new Solution at: {}", path);

		m_ProjectIndex.SetRoot(path);
		m_ProjectIndex.Scan();
		m_Editor.SetProjectIndex(&m_ProjectIndex);
		FX_CORE_INFO("ProjectIndex: indexed {} files", m_ProjectIndex.FileCount());
	}

	void uiLayer::OnAttach()
	{
		auto &app = Application::Get();
		int width = app.GetWindow().GetWidth();
		int height = app.GetWindow().GetHeight();

		Renderer2D::Init();

		auto &settings = SettingsManager::Get().GetSettings();

		std::string path = "Resources/Languages/" + LanguageManager::GetLanguageCode() + ".json";
		std::ifstream file(path);
		nlohmann::json j;
		file >> j;
		m_StatusBar = new uiStatusBar();
		m_StatusBar->OnAttach();
		m_StatusBar->SetStatusText(j.value("Ready", "Ready"));

		m_TitleBar = new uiTitleBar();
		m_TitleBar->OnAttach();

		m_ShortcutBar = new uiShortcutBar();
		m_ShortcutBar->OnAttach();
		std::vector<ShortcutItem> editGroup =
			{
				{"undo", "U", j.value("Undo", "Undo"), []()
				 { FX_INFO("Undo"); }},
				{"redo", "R", j.value("Redo", "Redo"), []()
				 { FX_INFO("Redo"); }},
				{"cut", "Ct", j.value("Cut", "Cut"), []()
				 { FX_INFO("Cut"); }},
				{"copy", "Co", j.value("Copy", "Copy"), []()
				 { FX_INFO("Copy"); }},
				{"paste", "P", j.value("Paste", "Paste"), []()
				 { FX_INFO("Paste"); }}
			};
		std::vector<ShortcutItem> buildGroup =
			{
				{"debug", "D", j.value("Debug", "Debug"), []()
				 { FX_INFO("Debug"); }},
				{"build", "B", j.value("Build", "Build"), []()
				 { FX_INFO("Start Build."); }},
				{"rebuild", "RB", j.value("Rebuild", "Rebuild"), []()
				 { FX_INFO("Start Rebuild"); }},
				{"clean", "Cl", j.value("Clean", "Clean"), []()
				 { FX_INFO("Clean"); }},
				{"run", "R", j.value("Run", "Run"), []()
				 { FX_INFO("Run"); }}
			};
		std::vector<ShortcutItem> bookmarkGroup =
			{
				{"findbookmark", "FB", j.value("FindBookmark", "Find Bookmark"), []()
				 { FX_INFO("Find Bookmark"); }},
				{"addbookmark", "AB", j.value("AddBookmark", "Add Bookmark"), []()
				 { FX_INFO("Add Bookmark"); }},
				{"deletebookmark", "DB", j.value("DeleteBookmark", "Delete Bookmark"), []()
				 { FX_INFO("Delete Bookmark"); }},
				{"nextbookmark", "NB", j.value("NextBookmark", "Next Bookmark"), []()
				 { FX_INFO("Next Bookmark"); }},
				{"clearbookmarks", "CB", j.value("ClearBookmarks", "Clear Bookmarks"), []()
				 { FX_INFO("Clear Bookmarks"); }}
			};
		m_ShortcutBar->AddGroup(editGroup, true);
		m_ShortcutBar->AddGroup(buildGroup, true);
		m_ShortcutBar->AddGroup(bookmarkGroup, false);

		uiWindow::InitDockSystem(0.0f, 130.0f, width, height - 130.0f - 35.0f);

		auto *properties = new PropertiesWindow(j.value("Properties", "Properties"));

		m_SolutionExplorer = new SolutionExplorer(j.value("SolutionExplorer", "Solution Explorer"), &m_Solution, properties);

		m_SolutionExplorer->SetFileOpenCallback([this](const std::string &path)
												{
				auto* tab = m_Editor.openFile(path);
				if (tab) tab->invalidateHighlight();
				FX_CORE_INFO("Open File: {}", path); });
		auto *notifications = new uiWindow(j.value("Notifications", "Notifications"));

		m_Windows.push_back(m_SolutionExplorer);
		m_Windows.push_back(properties);

		uiWindow::DockWindow(m_SolutionExplorer, DockRegion::Left);
		uiWindow::DockWindow(properties, DockRegion::Right);

		for (auto *win : m_Windows)
		{
			win->OnAttach();
		}

		auto &fm = FontManager::Get();
		const float codeSize = 20.0f;

		fm.LoadFont("code", "Resources/Fonts/CascadiaCode-Regular.ttf", codeSize, FontStyle::Regular);
		fm.LoadFont("code", "Resources/Fonts/CascadiaCode-Bold.ttf", codeSize, FontStyle::Bold);
		fm.LoadFont("code", "Resources/Fonts/CascadiaCode-Italic.ttf", codeSize, FontStyle::Italic);
		fm.LoadFont("code", "Resources/Fonts/CascadiaCode-BoldItalic.ttf", codeSize, FontStyle::BoldItalic);
		// Fallback
		fm.AddFallback("code", "Resources/Fonts/SourceHanSansSC-Normal.otf", FontStyle::Regular);
		fm.AddFallback("code", "Resources/Fonts/SourceHanSansSC-Bold.otf", FontStyle::Bold);
		fm.AddFallback("code", "Resources/Fonts/SourceHanSansSC-Regular.otf", FontStyle::Italic);
		fm.AddFallback("code", "Resources/Fonts/SourceHanSansSC-Heavy.otf", FontStyle::BoldItalic);

		m_Editor.SetClipboardFunctions([]() -> std::string
		{
			auto* win = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
			if (!win) return {};
			const char* s = glfwGetClipboardString(win);
			return s ? std::string(s) : std::string{};
		},
		[](const std::string &s)
		{
			auto *win = static_cast<GLFWwindow *>(Application::Get().GetWindow().GetNativeWindow());
			if (!win)
				return;
			glfwSetClipboardString(win, s.c_str());
		});
		m_Editor.SetSaveAsDialog([]() -> std::string
			{ return SaveFileDialog("untitled.cpp", "C++ Files|*.cpp;*.h;*.hpp;*.c|All Files|*.*"); });
		m_Editor.SetCtrlStateGetter([]() -> bool
			{
				auto* win = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
				if (!win) return false;
				return glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
			});
		m_Editor.SetOnTabListChanged([this]() { SaveEditorState(); });

		RestoreEditorState();
		SaveEditorState();

		m_WelcomePage.OnAttach();
		m_WelcomePage.SetCallbacks(
			[this]()
			{
				m_Editor.newFile("Untitled.cpp");
			},
			[this]()
			{
				const std::string path = OpenFileDialog("C++ Files|*.cpp;*.h;*.hpp;*.c|All Files|*.*");
				if (!path.empty())
					m_Editor.openFile(path);
			},
			[this]()
			{
				const std::string path = SaveFileDialog("Untitled.frostnux", "Frostnux Solution|*.frostnux");
				if (!path.empty())
					CreateNewSolution(path);
			},
			[this]()
			{
				const std::string path = OpenFileDialog("Frostnux Solution|*.frostnux");
				if (!path.empty())
					LoadSolution(path);
			});
		m_CurrentLangCode = LanguageManager::GetLanguageCode();
	}

	void uiLayer::RestoreEditorState()
	{
		auto &settings = SettingsManager::Get().GetSettings();
		const auto &openFiles = settings.openFiles;
		const std::string &activePath = settings.recentFiles.empty() ? std::string{} : settings.recentFiles.front();

		while (m_Editor.tabCount() > 0)
			m_Editor.closeTab(m_Editor.tabCount() - 1);

		int activeIdx = -1;
		int openIdx = 0;
		for (const auto &path : openFiles)
		{
			Tab *tab = m_Editor.openFile(path);
			if (!tab)
				continue;
			if (path == activePath)
				activeIdx = openIdx;
			++openIdx;
		}

		if (activeIdx >= 0 && activeIdx < m_Editor.tabCount())
			m_Editor.switchTabTo(activeIdx);
	}

	void uiLayer::SaveEditorState()
	{
		auto &settings = SettingsManager::Get().GetSettings();

		settings.openFiles = m_Editor.GetOpenFiles();

		const std::string activePath = m_Editor.GetActiveFilePath();
		if (!activePath.empty())
		{
			auto &recent = settings.recentFiles;
			recent.erase(std::remove(recent.begin(), recent.end(), activePath), recent.end());
			recent.insert(recent.begin(), activePath);
		}

		SettingsManager::Get().Save();
	}

	void uiLayer::OnDetach()
	{
		for (auto *win : m_Windows)
			win->OnDetach();
		if (m_TitleBar)
			m_TitleBar->OnDetach();
		if (m_StatusBar)
			m_StatusBar->OnDetach();
		if (m_ShortcutBar)
			m_ShortcutBar->OnDetach();
		FontManager::Get().UnloadAll();
	}

	void uiLayer::OnUpdate(float deltaTime)
	{
		std::string langCode = LanguageManager::GetLanguageCode();
		if (m_CurrentLangCode != langCode)
		{
			m_CurrentLangCode = langCode;
			RefreshUIText();
		}

		auto &app = Application::Get();
		int width = app.GetWindow().GetWidth();
		int height = app.GetWindow().GetHeight();
		if (width == 0 || height == 0)
			return;

		Renderer2D::Begin((float)width, (float)height);

		double mx, my;
		glfwGetCursorPos(static_cast<GLFWwindow *>(Application::Get().GetWindow().GetNativeWindow()), &mx, &my);

		float centerX = uiWindow::GetCenterX();
		float centerY = uiWindow::GetCenterY();
		float centerW = uiWindow::GetCenterW();
		float centerH = uiWindow::GetCenterH();

		float leftW = uiWindow::GetDynamicLeftWidth();
		float rightW = uiWindow::GetDynamicRightWidth();
		float bottomH = uiWindow::GetDynamicBottomHeight();

		m_Editor.update(deltaTime, glfwGetTime());
		if (m_Editor.tabCount() > 0)
		{
			m_Editor.render(centerX, centerY, centerW, centerH);
		}
		else
		{
			m_WelcomePage.Draw(centerX, centerY, centerW, centerH);
		}

		MouseCircle::Get().OnUpdate(deltaTime);

		uiWindow::DrawDockAreas();
		uiWindow::DrawSplitters();

		if (m_StatusBar)
		{
			m_StatusBar->OnUpdate(deltaTime);
		}

		DockRegion preview = uiWindow::GetPreviewRegion();
		if (preview != DockRegion::None)
		{
			float x = 0, y = 0, w = 0, h = 0;
			float mainW = uiWindow::GetMainW();
			float mainH = uiWindow::GetMainH();
			float centerW = mainW - leftW - rightW;
			float centerH = mainH - bottomH;

			switch (preview)
			{
			case DockRegion::Left:
				x = 0;
				y = 0;
				w = leftW;
				h = centerH;
				break;
			case DockRegion::Right:
				x = mainW - rightW;
				y = 0;
				w = rightW;
				h = centerH;
				break;
			case DockRegion::Bottom:
				x = 0;
				y = mainH - bottomH;
				w = mainW;
				h = bottomH;
				break;
			case DockRegion::Center:
				x = leftW;
				y = 0;
				w = centerW;
				h = centerH;
				break;
			default:
				break;
			}

			x += uiWindow::GetMainX();
			y += uiWindow::GetMainY();

			Renderer2D::DrawQuad(x, y, w, h, 0.2f, 0.5f, 0.8f, 0.5f);
		}

		if (m_TitleBar)
			m_TitleBar->OnUpdate(deltaTime);

		if (m_ShortcutBar)
		{
			m_ShortcutBar->OnUpdate(deltaTime);
			m_ShortcutBar->Draw();
		}

		for (auto *win : m_Windows)
		{
			win->OnUpdate(deltaTime);
		}

		uiWindow::DrawDockPanel(mx, my);

		PopupMenu::Get().Draw();
		TextInputDialog::Get().Draw();

		Renderer2D::End();
	}

	bool uiLayer::OnEvent(Event &event)
	{
		if (PopupMenu::Get().HandleEvent(&event))
			return true;
		if (TextInputDialog::Get().OnEvent(event))
			return true;

		MouseCircle::Get().OnEvent(event);

		if (uiWindow::HandleSplitterEvent(event))
			return true;

		if (m_TitleBar && m_TitleBar->OnEvent(event))
			return true;

		if (m_ShortcutBar && m_ShortcutBar->OnEvent(event))
			return true;

		for (auto *win : m_Windows)
		{
			if (win->OnEvent(event))
				return true;
		}

		if (m_Editor.tabCount() > 0)
		{
			if (m_Editor.OnEvent(event))
				return true;
		}
		else
		{
			if (m_WelcomePage.OnEvent(event))
				return true;
		}

		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent &e)
											   {
			int width = e.GetWidth(), height = e.GetHeight();

			Application& app = Application::Get();
			float leftW = app.GetWindow().GetWidth() * 0.2f,
				rightW = app.GetWindow().GetWidth() * 0.2f,
				bottomH = app.GetWindow().GetHeight() * 0.3f;
			float mainW = (float)width;
			float mainH = (float)height;
			float centerW = mainW - leftW - rightW;
			float centerH = mainH - bottomH;
			float yOffset = 110.0f;

			uiWindow::InitDockSystem(0.0f, 130.0f, width, height - 160.0f);

			return false; });

		return false;
	}

	void uiLayer::AddWindow(uiWindow *window)
	{
		if (window)
		{
			m_Windows.push_back(window);
			window->OnAttach();
		}
	}

	bool uiLayer::IsPointOverAnyWindow(float x, float y)
	{
		auto &instance = Get();
		for (auto *win : instance.m_Windows)
		{
			if (win->IsVisible())
			{
				float wx = win->GetX();
				float wy = win->GetY();
				float ww = win->GetWidth();
				float wh = win->GetHeight();

				if (x >= wx && x <= wx + ww && y >= wy && y <= wy + wh)
				{
					return true;
				}
			}
		}
		return false;
	}

	void uiLayer::RefreshUIText()
	{
		std::string path = "Resources/Languages/" + LanguageManager::GetLanguageCode() + ".json";
		std::ifstream file(path);
		nlohmann::json j;
		if (file)
			file >> j;

		if (m_TitleBar)
		{
			m_TitleBar->RefreshText();
		}

		for (auto *win : m_Windows)
		{
			if (!win) continue;

			if (win)
				win->RefreshText();
		}

		if (m_StatusBar)
		{
			m_StatusBar->SetStatusText(j.value("Ready", "Ready"));
		}
	}

}
