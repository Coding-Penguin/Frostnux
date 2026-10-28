#include <fxpch.h>
#include "uiTitleBar.h"
#include "PhotoRenderer.h"
#include "TextRenderer.h"
#include "PopupMenu.h"
#include "uiButton.h"
#include "../uiLayer.h"
#include "../CodeEditor/CodeEditor.h"
#include "Frostnux/Core/Application.h"
#include "Frostnux/Events/Event.h"
#ifdef FX_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>
#include "Frostnux/Core/FileDialog.h"

namespace Frostnux
{

	uiTitleBar::uiTitleBar()
	{
	}
	uiTitleBar::~uiTitleBar()
	{
	}

	void uiTitleBar::OnAttach()
	{
		m_Logo.reset(new PhotoRenderer());
		m_Logo->LoadFromFile("Resources/Images/Frostnux.png");

		FX_INFO("Logo loaded: {0}", m_Logo->IsLoaded());

		BuildMenuButtons();
		m_CurrentLangCode = LanguageManager::GetLanguageCode();
	}

	void uiTitleBar::BuildMenuButtons()
	{
		m_Buttons.clear();

		float x = 90, y = 40;
		float buttonHeight = 40.0f;

		std::string Names[] = { "File", "Edit", "View", "Project", "Build", "Debug", "Tools", "Help", "Search" };
		std::string path = "Resources/Languages/" + LanguageManager::GetLanguageCode() + ".json";
		std::ifstream file(path);
		nlohmann::json j;
		file >> j;
		Names[0] = j.value("File", "File");
		Names[1] = j.value("Edit", "Edit");
		Names[2] = j.value("View", "View");
		Names[3] = j.value("Project", "Project");
		Names[4] = j.value("Build", "Build");
		Names[5] = j.value("Debug", "Debug");
		Names[6] = j.value("Tools", "Tools");
		Names[7] = j.value("Help", "Help");
		Names[8] = j.value("Search", "Search");

		CodeEditor& editorRef = uiLayer::GetCodeEditor();
		CodeEditor* editor = &editorRef;

		// File
		float buttonWidth = TextRenderer::Get().GetTextWidth(Names[0]) + 30.0f;
		auto Filebtn = std::make_unique<uiButton>(Names[0], x, y, buttonWidth, buttonHeight, ButtonStyles::NoBackgroundOrLine);
		Filebtn->SetCallback([editor, j, x, y, buttonHeight]() mutable
			{
				auto sNew = j.value("New", "New");
				auto sProject = j.value("Project", "Project");
				auto sFile = j.value("File", "File");
				auto sOpen = j.value("Open", "Open");
				auto sSave = j.value("SaveFile", "Save File");
				auto sSaveAs = j.value("SaveAs", "Save As");
				auto sSaveAll = j.value("SaveAllFiles", "Save All Files");

				std::vector<PopupMenu::Item> newSub;
				{
					PopupMenu::Item i1;
					i1.label = sProject;
					i1.onClick = [editor]() { FX_CORE_INFO("New Project"); };
					newSub.push_back(i1);

					PopupMenu::Item i2;
					i2.label = sFile;
					i2.onClick = [editor]() { FX_CORE_INFO("New File"); editor->newFile("Untitled.cpp"); };
					newSub.push_back(i2);
				}

				std::vector<PopupMenu::Item> openSub;
				{
					PopupMenu::Item i1;
					i1.label = sProject;
					i1.onClick = [editor]() { FX_CORE_INFO("Open Project"); };
					openSub.push_back(i1);

					PopupMenu::Item i2;
					i2.label = sFile;
					i2.onClick = [editor]()
						{
							FX_CORE_INFO("Open File");
							const std::string path = OpenFileDialog("C++ Files|*.cpp;*.h;*.hpp;*.c|All Files|*.*");
							if (!path.empty()) editor->openFile(path);
						};
					openSub.push_back(i2);
				}

				std::vector<PopupMenu::Item> mainMenu;

				PopupMenu::Item itemNew;
				itemNew.label = sNew;
				itemNew.shortcut = "Ctrl+N";
				itemNew.submenu = std::move(newSub);
				mainMenu.push_back(std::move(itemNew));

				PopupMenu::Item itemOpen;
				itemOpen.label = sOpen;
				itemOpen.shortcut = "Ctrl+O";
				itemOpen.submenu = std::move(openSub);
				mainMenu.push_back(std::move(itemOpen));

				PopupMenu::Item itemSave;
				itemSave.label = sSave;
				itemSave.shortcut = "Ctrl+S";
				itemSave.onClick = [editor]() { FX_CORE_INFO("Save"); editor->SaveActiveTab(false); };
				mainMenu.push_back(std::move(itemSave));

				PopupMenu::Item itemSaveAs;
				itemSaveAs.label = sSaveAs;
				itemSaveAs.onClick = [editor]() { FX_CORE_INFO("Save As"); editor->SaveActiveTab(true); };
				mainMenu.push_back(std::move(itemSaveAs));

				PopupMenu::Item itemSaveAll;
				itemSaveAll.label = sSaveAll;
				itemSaveAll.onClick = [editor]() { FX_CORE_INFO("Save All Files"); editor->SaveAllTabs(); };
				mainMenu.push_back(std::move(itemSaveAll));

				PopupMenu::Get().Open(x, y + buttonHeight, std::move(mainMenu));
			});
		m_Buttons.push_back(std::move(Filebtn));
		x += buttonWidth + 10;

		// Edit
		buttonWidth = TextRenderer::Get().GetTextWidth(Names[1]) + 30.0f;
		auto Editbtn = std::make_unique<uiButton>(Names[1], x, y, buttonWidth, buttonHeight, ButtonStyles::NoBackgroundOrLine);
		Editbtn->SetCallback([j, x, y, buttonHeight]() mutable
			{
				FX_INFO("Clicked \"Edit\"");
			});
		m_Buttons.push_back(std::move(Editbtn));
		x += buttonWidth + 10;

		// View
		buttonWidth = TextRenderer::Get().GetTextWidth(Names[2]) + 30.0f;
		auto Viewbtn = std::make_unique<uiButton>(Names[2], x, y, buttonWidth, buttonHeight, ButtonStyles::NoBackgroundOrLine);
		Viewbtn->SetCallback([j, x, y, buttonHeight]() mutable
			{
				FX_INFO("Clicked \"View\"");
			});
		m_Buttons.push_back(std::move(Viewbtn));
		x += buttonWidth + 10;

		// Project
		buttonWidth = TextRenderer::Get().GetTextWidth(Names[3]) + 30.0f;
		auto Projectbtn = std::make_unique<uiButton>(Names[3], x, y, buttonWidth, buttonHeight, ButtonStyles::NoBackgroundOrLine);

		Projectbtn->SetCallback([j, x, y, buttonHeight]() mutable
			{
				std::vector<PopupMenu::Item> mainMenu;

				PopupMenu::Item newProj;
				newProj.label = j.value("NewProject", "New Project");
				newProj.onClick = []()
					{
						const std::string path = SaveFileDialog("Untitled.frostnux", "Frostnux Solution|*.frostnux");
						if (!path.empty()) uiLayer::Get().CreateNewSolution(path);
					};
				mainMenu.push_back(std::move(newProj));

				PopupMenu::Item openProj;
				openProj.label = j.value("OpenProject", "Open Project");
				openProj.onClick = []()
					{
						const std::string path = OpenFileDialog("Frostnux Solution|*.frostnux");
						if (!path.empty()) uiLayer::Get().LoadSolution(path);
					};
				mainMenu.push_back(std::move(openProj));

				PopupMenu::Get().Open(x, y + buttonHeight, std::move(mainMenu));
			});
		m_Buttons.push_back(std::move(Projectbtn));
		x += buttonWidth + 10;

		// Build
		buttonWidth = TextRenderer::Get().GetTextWidth(Names[4]) + 30.0f;
		auto Buildbtn = std::make_unique<uiButton>(Names[4], x, y, buttonWidth, buttonHeight, ButtonStyles::NoBackgroundOrLine);
		Buildbtn->SetCallback([j, x, y, buttonHeight]() mutable
			{
				FX_INFO("Clicked \"Build\"");
			});
		m_Buttons.push_back(std::move(Buildbtn));
		x += buttonWidth + 10;

		// Debug
		buttonWidth = TextRenderer::Get().GetTextWidth(Names[5]) + 30.0f;
		auto Debugbtn = std::make_unique<uiButton>(Names[5], x, y, buttonWidth, buttonHeight, ButtonStyles::NoBackgroundOrLine);
		Debugbtn->SetCallback([j, x, y, buttonHeight]() mutable
			{
				FX_INFO("Clicked \"Debug\"");
			});
		m_Buttons.push_back(std::move(Debugbtn));
		x += buttonWidth + 10;

		// Tools
		buttonWidth = TextRenderer::Get().GetTextWidth(Names[6]) + 30.0f;
		auto Toolsbtn = std::make_unique<uiButton>(Names[6], x, y, buttonWidth, buttonHeight, ButtonStyles::NoBackgroundOrLine);
		Toolsbtn->SetCallback([j, x, y, buttonHeight]() mutable
			{
				std::vector<PopupMenu::Item> themesSub;

				auto addThemeItem = [&](Theme theme, const char* label, const char* shortcut)
					{
						PopupMenu::Item item;
						item.label = label;
						item.shortcut = shortcut;
						item.onClick = [theme]() { ThemeManager::SetTheme(theme); };
						themesSub.push_back(std::move(item));
					};

				addThemeItem(Theme::Dark, "Dark", "");
				addThemeItem(Theme::Light, "Light", "");
				addThemeItem(Theme::Cool_Breeze, "Cool Breeze", "");
				addThemeItem(Theme::Icy_Mint, "Icy Mint", "");
				addThemeItem(Theme::Cool_Slate, "Cool Slate", "");
				addThemeItem(Theme::Moonlight, "Moonlight", "");

				std::vector<PopupMenu::Item> mainMenu;

				PopupMenu::Item itemTheme;
				itemTheme.label = j.value("Theme", "Theme");
				itemTheme.submenu = std::move(themesSub);
				mainMenu.push_back(std::move(itemTheme));

				std::vector<PopupMenu::Item> languagesSub;

				auto addLanguageItem = [&](LanguageType language, const char* label, const char* shortcut)
					{
						PopupMenu::Item item;
						item.label = label;
						item.shortcut = shortcut;
						item.onClick = [language]()
							{
								LanguageManager::SetLanguage(language);
							};
						languagesSub.push_back(std::move(item));
					};

				addLanguageItem(LanguageType::en_US, "English", "");
				addLanguageItem(LanguageType::zh_CN, "简体中文", "");
				addLanguageItem(LanguageType::zh_TW, "繁體中文", "");
				addLanguageItem(LanguageType::ru_RU, "Русский", "");
				addLanguageItem(LanguageType::hu_HU, "Magyar", "");

				PopupMenu::Item itemLanguage;
				itemLanguage.label = j.value("LanguageSwitch", "Language");
				itemLanguage.submenu = std::move(languagesSub);
				mainMenu.push_back(std::move(itemLanguage));

				PopupMenu::Get().Open(x, y + buttonHeight, std::move(mainMenu));
			});
		m_Buttons.push_back(std::move(Toolsbtn));
		x += buttonWidth + 10;

		// Help
		buttonWidth = TextRenderer::Get().GetTextWidth(Names[7]) + 30.0f;
		auto Helpbtn = std::make_unique<uiButton>(Names[7], x, y, buttonWidth, buttonHeight, ButtonStyles::NoBackgroundOrLine);
		Helpbtn->SetCallback([j, x, y, buttonHeight]() mutable
			{
				FX_INFO("Clicked \"Help\"");
			});
		m_Buttons.push_back(std::move(Helpbtn));
		x += buttonWidth + 10;

		// Search
		x += 10;
		m_SeparateLineX = x;
		m_SeparateLineY = y;
		x += 20;
		buttonWidth = TextRenderer::Get().GetTextWidth(Names[8]) + 30.0f;
		auto addSearchButton = [&](const std::string& text, float width)
			{
				auto btn = std::make_unique<uiButton>(text, x, y, width, buttonHeight, ButtonStyles::NoBackgroundOrLine);
				btn->SetCallback([=]() mutable
					{
						FX_INFO("Clicked \"Search\".");
					});
				m_Buttons.push_back(std::move(btn));
			};
		addSearchButton(Names[8], buttonWidth);
	}

	void uiTitleBar::OnDetach()
	{
		m_Logo->Unload();
	}

	void uiTitleBar::OnUpdate(float deltaTime)
	{
		std::string langCode = LanguageManager::GetLanguageCode();
		if (m_CurrentLangCode != langCode)
		{
			m_CurrentLangCode = langCode;
			BuildMenuButtons();
		}

		auto& app = Application::Get();
		float width = static_cast<float>(app.GetWindow().GetWidth());
		auto [r, g, b] = ThemeManager::GetBGColor();

		std::string path = "Resources/Languages/" + LanguageManager::GetLanguageCode() + ".json";
		std::ifstream file(path);
		nlohmann::json j;
		file >> j;

		Renderer2D::DrawQuad(0.0f, 0.0f, width, 40.0f, std::min(r + 0.01f, 1.0f), std::min(g + 0.01f, 1.0f), std::min(b + 0.03f, 1.0f), 1.0f);

		if (m_Logo && m_Logo->IsLoaded())
		{
			m_Logo->Draw(5, 5, 80, 80);
		}

		float VersionX, VersionY;
		VersionX = width / 2.0f - TextRenderer::Get().GetTextWidth(s_DetailedVersion) / 2.0f;
		VersionY = (40.0f - TextRenderer::Get().GetTextHeight()) / 2.0f;
		float color = 0.0f;
		if (ThemeManager::IsDarkTheme())
		{
			color = 1.0f;
		}
		else
		{
			color = 0.0f;
		}
		TextRenderer::Get().DrawTextUTF8(s_DetailedVersion, VersionX, VersionY, color, color, color, 1.0f);

		for (auto& btn : m_Buttons)
		{
			btn->OnUpdate(0, 0, true);
		}

		color = 0.0f;
		if (ThemeManager::IsDarkTheme())
		{
			color = 0.9f;
		}
		else
		{
			color = 0.1f;
		}
		Renderer2D::DrawLine(m_SeparateLineX, m_SeparateLineY + 5.0f, m_SeparateLineX, m_SeparateLineY + 30.0f, 1.0f, color, color, color, 0.8f);

		UpdateWindowButtonsPosition(width);

		GLFWwindow* win = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());
		m_IsMaximized = glfwGetWindowAttrib(win, GLFW_MAXIMIZED) == GLFW_TRUE;
		DrawMinimizeButton(m_MinimizeRect.x, m_MinimizeRect.y, m_MinimizeRect.w, m_MinimizeRect.h, m_MinimizeHovered);
		DrawMaximizeButton(m_MaximizeRect.x, m_MaximizeRect.y, m_MaximizeRect.w, m_MaximizeRect.h, m_MaximizeHovered, m_IsMaximized);
		DrawCloseButton(m_CloseRect.x, m_CloseRect.y, m_CloseRect.w, m_CloseRect.h, m_CloseHovered);
	}

	bool uiTitleBar::OnEvent(Event& event)
	{
		for (auto& btn : m_Buttons)
		{
			if (btn->OnEvent(event, 0, 0, true))
				return true;
		}

		if (event.GetEventType() == EventType::MouseMoved)
		{
			MouseMovedEvent& e = (MouseMovedEvent&)event;
			float mx = e.GetX(), my = e.GetY();

			if (m_DraggingMainWindow)
			{
				auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
				int windowPosX, windowPosY;

				glfwRestoreWindow(window);
				glfwGetWindowPos(window, &windowPosX, &windowPosY);

				int newX = static_cast<int>(windowPosX + mx - m_DragStartX);
				int newY = static_cast<int>(windowPosY + my - m_DragStartY);
				glfwSetWindowPos(window, newX, newY);
				return true;
			}

			auto updateHover = [&](ButtonRect& rect, bool& hover)
				{
					hover = (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h);
				};
			updateHover(m_MinimizeRect, m_MinimizeHovered);
			updateHover(m_MaximizeRect, m_MaximizeHovered);
			updateHover(m_CloseRect, m_CloseHovered);
			return false;
		}
		else if (event.GetEventType() == EventType::MouseButtonPressed)
		{
			MouseButtonPressedEvent& e = (MouseButtonPressedEvent&)event;
			if (e.GetMouseButton() != GLFW_MOUSE_BUTTON_LEFT) return false;
			float mx = e.GetMouseX(), my = e.GetMouseY();

			if (mx >= m_MinimizeRect.x && mx <= m_MinimizeRect.x + m_MinimizeRect.w && my >= m_MinimizeRect.y && my <= m_MinimizeRect.y + m_MinimizeRect.h)
			{
				auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
				glfwIconifyWindow(window);
				return true;
			}

			if (mx >= m_MaximizeRect.x && mx <= m_MaximizeRect.x + m_MaximizeRect.w && my >= m_MaximizeRect.y && my <= m_MaximizeRect.y + m_MaximizeRect.h)
			{
				auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
				if (glfwGetWindowAttrib(window, GLFW_MAXIMIZED))
				{
					glfwRestoreWindow(window);
					SettingsManager::Get().Save();
				}
				else
				{
					glfwMaximizeWindow(window);
					SettingsManager::Get().Save();
				}
				return true;
			}

			if (mx >= m_CloseRect.x && mx <= m_CloseRect.x + m_CloseRect.w && my >= m_CloseRect.y && my <= m_CloseRect.y + m_CloseRect.h)
			{
				auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
				glfwSetWindowShouldClose(window, GLFW_TRUE);
				FX_CORE_INFO("Close button clicked!");
				return true;
			}

			if (my < 50)
			{
				bool hitButton = false;

				for (auto& btn : m_Buttons)
				{
					float bx = btn->GetX(), by = btn->GetY(), bw = btn->GetWidth(), bh = btn->GetHeight();
					if (mx >= bx && mx <= bx + bw && my >= by && my <= by + bh)
					{
						hitButton = true;
						break;
					}
				}

				if (!hitButton && ((mx >= m_MinimizeRect.x && mx <= m_MinimizeRect.x + m_MinimizeRect.w && my >= m_MinimizeRect.y && my <= m_MinimizeRect.y + m_MinimizeRect.h) || (mx >= m_MaximizeRect.x && mx <= m_MaximizeRect.x + m_MaximizeRect.w && my >= m_MaximizeRect.y && my <= m_MaximizeRect.y + m_MaximizeRect.h) || (mx >= m_CloseRect.x && mx <= m_CloseRect.x + m_CloseRect.w && my >= m_CloseRect.y && my <= m_CloseRect.y + m_CloseRect.h)))
				{
					hitButton = true;
				}

				if (!hitButton)
				{
					auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
					glfwGetWindowPos(window, &m_WindowStartX, &m_WindowStartY);
					m_DragStartX = mx;
					m_DragStartY = my;
					m_DraggingMainWindow = true;
					return true;
				}
			}
		}
		else if (event.GetEventType() == EventType::MouseButtonReleased)
		{
			MouseButtonReleasedEvent& e = (MouseButtonReleasedEvent&)event;
			if (e.GetMouseButton() != GLFW_MOUSE_BUTTON_LEFT) return false;
			if (m_DraggingMainWindow)
			{
				m_DraggingMainWindow = false;
				return true;
			}
			return false;
		}

		return false;
	}

	void uiTitleBar::UpdateWindowButtonsPosition(int windowWidth)
	{
		float btnWidth = 45.0f;
		float btnHeight = 45.0f;
		float y = 0;
		float rightMargin = 0;
		float startX = windowWidth - 3 * btnWidth - rightMargin;

		m_MinimizeRect = { startX, y, btnWidth, btnHeight };
		m_MaximizeRect = { startX + btnWidth, y, btnWidth, btnHeight };
		m_CloseRect = { startX + 2 * btnWidth, y, btnWidth, btnHeight };
	}

	void uiTitleBar::OnWindowResize(int width, int height)
	{
		UpdateWindowButtonsPosition(width);
	}

	void uiTitleBar::DrawMinimizeButton(float x, float y, float w, float h, bool hovered)
	{
		if (hovered)
		{
			if (ThemeManager::IsDarkTheme())
				Renderer2D::DrawQuad(x, y, w, h, 0.3f, 0.3f, 0.3f, 1.0f);
			else
				Renderer2D::DrawQuad(x, y, w, h, 0.7f, 0.7f, 0.7f, 1.0f);
		}
		else
		{
			Renderer2D::DrawQuad(x, y, w, h, 0.0f, 0.0f, 0.0f, 0.0f);
		}

		float lineY = y + h / 2;
		float left = x + w * 0.25f;
		float right = x + w * 0.75f;

		if (ThemeManager::IsDarkTheme())
			Renderer2D::DrawLine(left, lineY, right, lineY, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		else
			Renderer2D::DrawLine(left, lineY, right, lineY, 2.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	}

	void uiTitleBar::DrawMaximizeButton(float x, float y, float w, float h, bool hovered, bool isMaximized)
	{
		if (hovered)
		{
			if (ThemeManager::IsDarkTheme())
				Renderer2D::DrawQuad(x, y, w, h, 0.3f, 0.3f, 0.3f, 1.0f);
			else
				Renderer2D::DrawQuad(x, y, w, h, 0.7f, 0.7f, 0.7f, 1.0f);
		}

		float c = ThemeManager::IsDarkTheme() ? 1.0f : 0.0f;

		float margin;
		if (isMaximized)	margin = w * 0.27f;
		else				margin = w * 0.25f;

		float left = x + margin;
		float right = x + w - margin;
		float top = y + margin;
		float bottom = y + h - margin;

		if (isMaximized)
		{
			float offsetOuterX = 1.7f;
			float offsetOuterY = -1.7f;
			float outerX = left + offsetOuterX;
			float outerY = top + offsetOuterY;
			float outerW = right - left;
			float outerH = bottom - top;

			Renderer2D::DrawLine(outerX, outerY, outerX + outerW, outerY, 2.0f, c, c, c, 1.0f);
			Renderer2D::DrawLine(outerX + outerW, outerY, outerX + outerW, outerY + outerH, 2.0f, c, c, c, 1.0f);

			float offsetInnerX = -3.4f;
			float offsetInnerY = 3.4f;
			float innerX = outerX + offsetInnerX;
			float innerY = outerY + offsetInnerY;

			Renderer2D::DrawLine(innerX, innerY, innerX + outerW, innerY, 2.0f, c, c, c, 1.0f);
			Renderer2D::DrawLine(innerX + outerW, innerY, innerX + outerW, innerY + outerH, 2.0f, c, c, c, 1.0f);
			Renderer2D::DrawLine(innerX + outerW, innerY + outerH, innerX, innerY + outerH, 2.0f, c, c, c, 1.0f);
			Renderer2D::DrawLine(innerX, innerY + outerH, innerX, innerY, 2.0f, c, c, c, 1.0f);
		}
		else
		{
			Renderer2D::DrawLine(left, top, right, top, 2.0f, c, c, c, 1.0f);
			Renderer2D::DrawLine(right, top, right, bottom, 2.0f, c, c, c, 1.0f);
			Renderer2D::DrawLine(right, bottom, left, bottom, 2.0f, c, c, c, 1.0f);
			Renderer2D::DrawLine(left, bottom, left, top, 2.0f, c, c, c, 1.0f);
		}
	}

	void uiTitleBar::DrawCloseButton(float x, float y, float w, float h, bool hovered)
	{
		if (hovered)
		{
			Renderer2D::DrawQuad(x, y, w, h, 1.0f, 0.0f, 0.0f, 1.0f);
		}

		float margin = w * 0.25f;
		float left = x + margin;
		float right = x + w - margin;
		float top = y + margin;
		float bottom = y + h - margin;

		float c = ThemeManager::IsDarkTheme() ? 1.0f : 0.0f;

		Renderer2D::DrawLine(left, top, right, bottom, 2.0f, c, c, c, 1.0f);
		Renderer2D::DrawLine(right, top, left, bottom, 2.0f, c, c, c, 1.0f);
	}

	void uiTitleBar::RefreshText()
	{
		BuildMenuButtons();
	}

}
