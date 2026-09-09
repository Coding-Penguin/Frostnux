#include <fxpch.h>
#include "uiTitleBar.h"
#include "PhotoRenderer.h"
#include "TextRenderer.h"
#include "uiButton.h"
#include "Frostnux/Application.h"
#include "Frostnux/Events/Event.h"
#ifdef FX_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "Search.h"

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

		float x = 0, y = 10;
		if (ChannelManager::GetChannel() == Channel::Preview)
		{
			x = 90;
			y = 40;
		}
		else
		{
			x = y = 10;
		}

		float buttonHeight = 40.0f;

		std::string Names[] = { "File", "Edit", "View", "Project", "Build", "Debug", "Tools", "Help", "Search"};
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

		// File
		auto Filebtn = std::make_unique<uiButton>(Names[0], x, y, 70, buttonHeight, ButtonStyles::NoBackgroundOrLine);
		Filebtn->SetCallback([=]() { FX_INFO("Clicked \"File\""); });
		m_Buttons.push_back(std::move(Filebtn));
		x += 80;

		// Edit
		auto Editbtn = std::make_unique<uiButton>(Names[1], x, y, 70, buttonHeight, ButtonStyles::NoBackgroundOrLine);
		Editbtn->SetCallback([=]() { FX_INFO("Clicked \"Edit\""); });
		m_Buttons.push_back(std::move(Editbtn));
		x += 80;

		// View
		auto Viewbtn = std::make_unique<uiButton>(Names[2], x, y, 70, buttonHeight, ButtonStyles::NoBackgroundOrLine);
		Viewbtn->SetCallback([=]() { FX_INFO("Clicked \"View\""); });
		m_Buttons.push_back(std::move(Viewbtn));
		x += 80;

		// Project
		auto Projectbtn = std::make_unique<uiButton>(Names[3], x, y, 90, buttonHeight, ButtonStyles::NoBackgroundOrLine);
		Projectbtn->SetCallback([=]() { FX_INFO("Clicked \"Project\""); });
		m_Buttons.push_back(std::move(Projectbtn));
		x += 100;

		// Build
		auto Buildbtn = std::make_unique<uiButton>(Names[4], x, y, 75, buttonHeight, ButtonStyles::NoBackgroundOrLine);
		Buildbtn->SetCallback([=]() { FX_INFO("Clicked \"Build\""); });
		m_Buttons.push_back(std::move(Buildbtn));
		x += 85;

		// Debug
		auto Debugbtn = std::make_unique<uiButton>(Names[5], x, y, 75, buttonHeight, ButtonStyles::NoBackgroundOrLine);
		Debugbtn->SetCallback([=]() { FX_INFO("Clicked \"Debug\""); });
		m_Buttons.push_back(std::move(Debugbtn));
		x += 85;

		// Tools
		auto Toolsbtn = std::make_unique<uiButton>(Names[6], x, y, 75, buttonHeight, ButtonStyles::NoBackgroundOrLine);
		Toolsbtn->SetCallback([=]() { FX_INFO("Clicked \"Tools\""); });
		m_Buttons.push_back(std::move(Toolsbtn));
		x += 85;

		// Help
		auto Helpbtn = std::make_unique<uiButton>(Names[7], x, y, 70, buttonHeight, ButtonStyles::NoBackgroundOrLine);
		Helpbtn->SetCallback([=]() { FX_INFO("Clicked \"Help\""); });
		m_Buttons.push_back(std::move(Helpbtn));
		x += 80;

		// Search
		x += 10;
		m_SeparateLineX = x;
		m_SeparateLineY = y;
		x += 20;
		auto addSearchButton = [&](const std::string& text, float width)
			{
				auto btn = std::make_unique<uiButton>(text, x, y, width, buttonHeight, ButtonStyles::NoBackgroundOrLine);
				btn->SetCallback([=]() { FX_INFO("Clicked \"Search\"."); });
				m_Buttons.push_back(std::move(btn));
			};
		addSearchButton(Names[8], 150);
	}

	void uiTitleBar::OnDetach()
	{
		m_Logo->Unload();
	}

	void uiTitleBar::OnUpdate(float deltaTime)
	{
		if (ChannelManager::GetChannel() == Channel::Preview)
		{
			auto& app = Application::Get();
			float width = static_cast<float>(app.GetWindow().GetWidth());
			auto [r, g, b] = ThemeManager::GetBGColor();

			glColor4f(std::min(r + 0.01f, 1.0f), std::min(g + 0.01f, 1.0f), std::min(b + 0.03f, 1.0f), 1.0f);
			glBegin(GL_QUADS);
			glVertex2f(0.0f, 40.0f);
			glVertex2f(width, 40.0f);
			glVertex2f(width, 0.0f);
			glVertex2f(0.0f, 0.0f);
			glEnd();

			if (m_Logo && m_Logo->IsLoaded())
			{
				m_Logo->Draw(5, 5, 80, 80);
			}

			float VersionX, VersionY;
			VersionX = width / 2.0f - TextRenderer::Get().GetTextWidth(s_Version) / 2.0f;
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
			TextRenderer::Get().DrawText(s_Version, VersionX, VersionY, color, color, color, 1.0f);
		}

		for (auto& btn : m_Buttons) 
		{
			btn->OnUpdate(0, 0, true);
		}

		// Draw Separate Line
		float color = 0.0f;
		if (ThemeManager::IsDarkTheme())
		{
			color = 0.9f;
		}
		else
		{
			color = 0.1f;
		}
		glColor4f(color, color, color, 0.8f);
		glBegin(GL_LINES);
		glVertex2f(m_SeparateLineX, m_SeparateLineY + 5);
		glVertex2f(m_SeparateLineX, m_SeparateLineY + 30);
		glEnd();

		auto& app = Application::Get();
		int width = app.GetWindow().GetWidth();
		UpdateWindowButtonsPosition(width);

		GLFWwindow* win = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());
		m_IsMaximized = glfwGetWindowAttrib(win, GLFW_MAXIMIZED) == GLFW_TRUE;

		if (ChannelManager::GetChannel() == Channel::Preview)
		{
			DrawMinimizeButton(m_MinimizeRect.x, m_MinimizeRect.y, m_MinimizeRect.w, m_MinimizeRect.h, m_MinimizeHovered);
			DrawMaximizeButton(m_MaximizeRect.x, m_MaximizeRect.y, m_MaximizeRect.w, m_MaximizeRect.h, m_MaximizeHovered, m_IsMaximized);
			DrawCloseButton(m_CloseRect.x, m_CloseRect.y, m_CloseRect.w, m_CloseRect.h, m_CloseHovered);
		}
	}

	bool uiTitleBar::OnEvent(Event &event)
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

			if (ChannelManager::GetChannel() == Channel::Preview)
			{
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

			if (ChannelManager::GetChannel() == Channel::Preview)
			{
				if (mx >= m_MinimizeRect.x && mx <= m_MinimizeRect.x + m_MinimizeRect.w &&
					my >= m_MinimizeRect.y && my <= m_MinimizeRect.y + m_MinimizeRect.h)
				{
					auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
					glfwIconifyWindow(window);
					return true;
				}

				if (mx >= m_MaximizeRect.x && mx <= m_MaximizeRect.x + m_MaximizeRect.w &&
					my >= m_MaximizeRect.y && my <= m_MaximizeRect.y + m_MaximizeRect.h)
				{
					auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
					if (glfwGetWindowAttrib(window, GLFW_MAXIMIZED))
					{
						glfwRestoreWindow(window);
						SettingsManager::Get().SetMaximize(false);
					}
					else
					{
						glfwMaximizeWindow(window);
						SettingsManager::Get().SetMaximize(true);
					}
					return true;
				}

				if (mx >= m_CloseRect.x && mx <= m_CloseRect.x + m_CloseRect.w &&
					my >= m_CloseRect.y && my <= m_CloseRect.y + m_CloseRect.h)
				{
					auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
					glfwSetWindowShouldClose(window, GLFW_TRUE);
					FX_CORE_INFO("Close button clicked!");
					return true;
				}
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

				if (!hitButton && ((mx >= m_MinimizeRect.x && mx <= m_MinimizeRect.x + m_MinimizeRect.w &&
					my >= m_MinimizeRect.y && my <= m_MinimizeRect.y + m_MinimizeRect.h) ||
					(mx >= m_MaximizeRect.x && mx <= m_MaximizeRect.x + m_MaximizeRect.w &&
						my >= m_MaximizeRect.y && my <= m_MaximizeRect.y + m_MaximizeRect.h) ||
					(mx >= m_CloseRect.x && mx <= m_CloseRect.x + m_CloseRect.w &&
						my >= m_CloseRect.y && my <= m_CloseRect.y + m_CloseRect.h)))
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
				glColor4f(0.3f, 0.3f, 0.3f, 1.0f);
			else
				glColor4f(0.7f, 0.7f, 0.7f, 1.0f);
		}
		else
		{
			glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
		}

		glBegin(GL_QUADS);
		glVertex2f(x, y);
		glVertex2f(x + w, y);
		glVertex2f(x + w, y + h);
		glVertex2f(x, y + h);
		glEnd();

		float lineY = y + h / 2;
		float left = x + w * 0.25f;
		float right = x + w * 0.75f;

		if (ThemeManager::IsDarkTheme())
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		else
			glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

		glLineWidth(2.0f);
		glBegin(GL_LINES);
		glVertex2f(left, lineY);
		glVertex2f(right, lineY);
		glEnd();
	}

	void uiTitleBar::DrawMaximizeButton(float x, float y, float w, float h, bool hovered, bool isMaximized)
	{
		if (hovered)
		{
			if (ThemeManager::IsDarkTheme())
				glColor4f(0.3f, 0.3f, 0.3f, 1.0f);
			else
				glColor4f(0.7f, 0.7f, 0.7f, 1.0f);
		}
		else
		{
			glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
		}

		glBegin(GL_QUADS);
		glVertex2f(x, y); glVertex2f(x + w, y);
		glVertex2f(x + w, y + h); glVertex2f(x, y + h);
		glEnd();

		if (ThemeManager::IsDarkTheme())
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		else
			glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

		glLineWidth(2.0f);

		float margin;
		if (isMaximized)
		{
			margin = w * 0.27f;
		}
		else
		{
			margin = w * 0.25f;
		}
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

			glBegin(GL_LINES);

			glVertex2f(outerX, outerY);
			glVertex2f(outerX + outerW, outerY);

			glVertex2f(outerX + outerW, outerY);
			glVertex2f(outerX + outerW, outerY + outerH);
			glEnd();

			float offsetInnerX = -3.4f;
			float offsetInnerY = 3.4f;
			float innerX = outerX + offsetInnerX;
			float innerY = outerY + offsetInnerY;

			glBegin(GL_LINE_LOOP);
			glVertex2f(innerX, innerY);
			glVertex2f(innerX + outerW, innerY);
			glVertex2f(innerX + outerW, innerY + outerH);
			glVertex2f(innerX, innerY + outerH);
			glEnd();
		}
		else
		{
			glBegin(GL_LINE_LOOP);
			glVertex2f(left, top);
			glVertex2f(right, top);
			glVertex2f(right, bottom);
			glVertex2f(left, bottom);
			glEnd();
		}
	}

	void uiTitleBar::DrawCloseButton(float x, float y, float w, float h, bool hovered)
	{
		if (hovered)
			glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
		else
			glColor4f(0.0f, 0.0f, 0.0f, 0.0f);

		glBegin(GL_QUADS);
		glVertex2f(x, y);
		glVertex2f(x + w, y);
		glVertex2f(x + w, y + h);
		glVertex2f(x, y + h);
		glEnd();

		float margin = w * 0.25f;
		float left = x + margin;
		float right = x + w - margin;
		float top = y + margin;
		float bottom = y + h - margin;

		if (ThemeManager::IsDarkTheme())
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		else
			glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

		glLineWidth(2.0f);
		glBegin(GL_LINES);
		glVertex2f(left, top);
		glVertex2f(right, bottom);
		glVertex2f(right, top);
		glVertex2f(left, bottom);
		glEnd();
	}

}
