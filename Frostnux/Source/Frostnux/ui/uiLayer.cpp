#include <fxpch.h>
#include "uiLayer.h"
#include "uiTools/ui.h"
#include "Frostnux/Log.h"
#include "Frostnux/Application.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "CodeEditor/CodeEditor.h"
#include "Frostnux/SettingsManager.h"

namespace Frostnux {

	uiLayer& uiLayer::Get()
	{
		static uiLayer instance;
		return instance;
	}

	uiLayer::uiLayer() : Layer("UILayer")
	{
	}

	uiLayer::~uiLayer() 
	{
		for (auto* win : m_Windows)
			delete win;
		m_Windows.clear();
		delete m_TitleBar;
		delete m_StatusBar;
		delete m_ShortcutBar;
		delete m_CodeEditor;
	}

	void uiLayer::OnAttach()
	{
		FX_CORE_INFO("uiLayer attached.");

		auto& app = Application::Get();
		int width = app.GetWindow().GetWidth();
		int height = app.GetWindow().GetHeight();

		auto& settings = SettingsManager::Get().GetSettings();

		m_CodeEditor = new CodeEditor("");

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
		std::vector<ShortcutItem> fileGroup =
		{
			{ "new", "N", j.value("NewFile", "New File"), []() { FX_INFO("New File"); }},
			{ "open", "O", j.value("OpenFile", "Open File"), []() { FX_INFO("Open File"); } },
			{ "save", "S", j.value("SaveFile", "Save File"), []() { FX_INFO("Save File"); } },
			{ "saveas", "SA", j.value("SaveAs", "Save As"), []() { FX_INFO("Save As"); } },
			{ "saveall", "SA", j.value("SaveAllFiles", "Save All Files"), []() { FX_INFO("Save All"); } }
		};
		std::vector<ShortcutItem> editGroup =
		{
			{ "undo", "U", j.value("Undo", "Undo"), []() { FX_INFO("Undo"); }},
			{ "redo", "R", j.value("Redo", "Redo"), []() { FX_INFO("Redo"); } },
			{ "cut", "Ct", j.value("Cut", "Cut"), []() { FX_INFO("Cut"); }},
			{ "copy", "Co", j.value("Copy", "Copy"), []() { FX_INFO("Copy"); } },
			{ "paste", "P", j.value("Paste", "Paste"), []() { FX_INFO("Paste"); } }
		};
		std::vector<ShortcutItem> buildGroup =
		{
			{ "debug", "D", j.value("Debug", "Debug"), []() { FX_INFO("Debug"); } },
			{ "build", "B", j.value("Build", "Build"), []() { FX_INFO("Start Build."); } },
			{ "rebuild", "RB", j.value("Rebuild", "Rebuild"), []() { FX_INFO("Start Rebuild"); } },
			{ "clean", "Cl", j.value("Clean", "Clean"), []() { FX_INFO("Clean"); } },
			{ "run", "R", j.value("Run", "Run"), []() { FX_INFO("Run"); } }
		};
		std::vector<ShortcutItem> bookmarkGroup =
		{
			{ "findbookmark", "FB", j.value("FindBookmark", "Find Bookmark"), []() { FX_INFO("Find Bookmark"); } },
			{ "addbookmark", "AB", j.value("AddBookmark", "Add Bookmark"), []() { FX_INFO("Add Bookmark"); } },
			{ "deletebookmark", "DB", j.value("DeleteBookmark", "Delete Bookmark"), []() { FX_INFO("Delete Bookmark"); } },
			{ "nextbookmark", "NB", j.value("NextBookmark", "Next Bookmark"), []() { FX_INFO("Next Bookmark"); } },
			{ "clearbookmarks", "CB", j.value("ClearBookmarks", "Clear Bookmarks"), []() { FX_INFO("Clear Bookmarks"); } }
		};
		m_ShortcutBar->AddGroup(fileGroup, true);
		m_ShortcutBar->AddGroup(editGroup, true);
		m_ShortcutBar->AddGroup(buildGroup, true);
		m_ShortcutBar->AddGroup(bookmarkGroup, false);

		uiWindow::InitDockSystem(0.0f, 130.0f, width, height - 160.0f);

		auto* properties = new PropertiesWindow();
		auto* fileExplorer = new FileExplorer(s_RootPath, properties);
		fileExplorer->SetFileOpenCallback([this](const std::string& path)
			{
				if (this->m_TabManager)
				{
					this->m_TabManager->OpenFile(path);
				}
				else
				{
					FX_CORE_ERROR("TabManager is null!");
				}
			});
		auto* notifications = new uiWindow("Notifications");
		m_Windows.push_back(fileExplorer);
		uiWindow::DockWindow(fileExplorer, DockRegion::Left);
		for (auto* win : m_Windows)
		{
			win->OnAttach();
		}

		m_TabManager = new EditorTabManager();
		int topOffset = 130;
		int bottomOffset = 40;
		float tabX = 0;
		float tabY = topOffset;
		float tabW = width;
		float tabH = height - topOffset - bottomOffset;
		m_TabManager->SetBounds(tabX, tabY, tabW, tabH);
		for (const auto& filepath : settings.openFiles)
		{
			m_TabManager->OpenFile(filepath, true);
		}
	}

	void uiLayer::OnDetach() 
	{
		for (auto* win : m_Windows)
			win->OnDetach();
		if (m_TitleBar) m_TitleBar->OnDetach();
		if (m_StatusBar) m_StatusBar->OnDetach();
		if (m_ShortcutBar) m_ShortcutBar->OnDetach();
		delete m_CodeEditor;
	}

	void uiLayer::OnUpdate(float deltaTime)
	{
		auto& app = Application::Get();
		int width = app.GetWindow().GetWidth();
		int height = app.GetWindow().GetHeight();
		if (width == 0 || height == 0) return;

		glViewport(0, 0, width, height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, width, height, 0, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		double mx, my;
		glfwGetCursorPos(static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow()), &mx, &my);

		float centerX = uiWindow::GetCenterX();
		float centerY = uiWindow::GetCenterY();
		float centerW = uiWindow::GetCenterW();
		float centerH = uiWindow::GetCenterH();

		float leftW = uiWindow::GetDynamicLeftWidth();
		float rightW = uiWindow::GetDynamicRightWidth();
		float bottomH = uiWindow::GetDynamicBottomHeight();

		MouseCircle::Get().OnUpdate(deltaTime);

		uiWindow::DrawDockAreas();

		if (m_StatusBar)
		{
			m_StatusBar->OnUpdate(deltaTime);
			m_StatusBar->SetStatusText("Ready");
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
				x = 0; y = 0; w = leftW; h = centerH;
				break;
			case DockRegion::Right:
				x = mainW - rightW; y = 0; w = rightW; h = centerH;
				break;
			case DockRegion::Bottom:
				x = 0; y = mainH - bottomH; w = mainW; h = bottomH;
				break;
			case DockRegion::Center:
				x = leftW; y = 0; w = centerW; h = centerH;
				break;
			default: break;
			}

			x += uiWindow::GetMainX();
			y += uiWindow::GetMainY();

			glEnable(GL_BLEND);
			glColor4f(0.2f, 0.5f, 0.8f, 0.5f);
			glBegin(GL_QUADS);
			glVertex2f(x, y);
			glVertex2f(x + w, y);
			glVertex2f(x + w, y + h);
			glVertex2f(x, y + h);
			glEnd();
		}

		if (m_TitleBar) m_TitleBar->OnUpdate(deltaTime);

		if (m_ShortcutBar)
		{
			m_ShortcutBar->OnUpdate(deltaTime);
			m_ShortcutBar->Draw();
		}

		if (m_CodeEditor)
		{
			m_CodeEditor->SetViewBounds(centerX, centerY + 30, centerW, centerH - 30);
			m_CodeEditor->OnUpdate(deltaTime);
		}

		if (m_TabManager)
		{
			m_TabManager->SetBounds(centerX, centerY, centerW, centerH);

			m_TabManager->OnUpdate(deltaTime);
			m_TabManager->Draw();
		}

		for (auto* win : m_Windows)
		{
			win->OnUpdate(deltaTime);
		}

		uiWindow::DrawDockPanel(mx, my);
	}

	bool uiLayer::OnEvent(Event& event)
	{
		MouseCircle::Get().OnEvent(event);

		if (m_TabManager && m_TabManager->OnEvent(event))
			return true;

		if (m_TitleBar && m_TitleBar->OnEvent(event))
			return true;

		if (m_ShortcutBar && m_ShortcutBar->OnEvent(event))
			return true;

		for (auto* win : m_Windows)
		{
			if (win->OnEvent(event))
				return true;
		}

		if (m_CodeEditor) return m_CodeEditor->OnEvent(event);

		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e)
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

			EditorView::Get().SetBounds(leftW, yOffset, centerW, centerH);

			uiWindow::InitDockSystem(0.0f, 130.0f, width, height - 160.0f);

			return false;
			});

		return false;
	}

	void uiLayer::AddWindow(uiWindow* window)
	{
		if (window)
		{
			m_Windows.push_back(window);
			window->OnAttach();
		}
	}

	bool uiLayer::IsPointOverAnyWindow(float x, float y)
	{
		auto& instance = Get();
		for (auto* win : instance.m_Windows)
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

}
