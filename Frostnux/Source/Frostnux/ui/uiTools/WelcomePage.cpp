#include <fxpch.h>
#include "WelcomePage.h"
#include "TextRenderer.h"
#include "PhotoRenderer.h"
#include "Frostnux/Events/Event.h"
#include "Frostnux/Events/MouseEvent.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Frostnux {

	WelcomePage::WelcomePage() {}
	WelcomePage::~WelcomePage() {}

	void WelcomePage::OnAttach()
	{
		m_Logo.reset(new PhotoRenderer());
		m_Logo->LoadFromFile("Resources/Images/Frostnux.png");

		m_Buttons.resize(4);
		m_Buttons[0].label = "New File";
		m_Buttons[0].desc = "Create a new source file";
		m_Buttons[1].label = "Open File...";
		m_Buttons[1].desc = "Open an existing file";
		m_Buttons[2].label = "New Project";
		m_Buttons[2].desc = "Create a new C++ project";
		m_Buttons[3].label = "Open Project...";
		m_Buttons[3].desc = "Open a .frostnux or .fxproject";
	}

	void WelcomePage::SetCallbacks(
		std::function<void()> onNewFile,
		std::function<void()> onOpenFile,
		std::function<void()> onNewProject,
		std::function<void()> onOpenProject)
	{
		m_Buttons[0].onClick = onNewFile;
		m_Buttons[1].onClick = onOpenFile;
		m_Buttons[2].onClick = onNewProject;
		m_Buttons[3].onClick = onOpenProject;
	}

	void WelcomePage::Layout(float x, float y, float w, float h)
	{
		const float contentW = 400.0f;
		const float contentH = 400.0f;
		const float startX = x + (w - contentW) / 2.0f;
		const float startY = y + (h - contentH) / 2.0f;

		const float btnW = 400.0f;
		const float btnH = 50.0f;
		const float gap = 10.0f;

		float currentY = startY + 180.0f;
		for (size_t i = 0; i < m_Buttons.size(); ++i)
		{
			m_Buttons[i].rect = { startX, currentY, btnW, btnH };
			currentY += btnH + gap;
		}
	}

	void WelcomePage::Draw(float x, float y, float w, float h)
	{
		Layout(x, y, w, h);

		if (ThemeManager::IsDarkTheme())
			DrawRect(x, y, w, h, { 0.10f, 0.10f, 0.12f, 1.0f });
		else
			DrawRect(x, y, w, h, { 1.0f, 0.95f, 0.95f, 1.0f });

		if (m_Logo && m_Logo->IsLoaded())
		{
			float logoSize = 100.0f;
			float lx = x + (w - logoSize) / 2.0f;
			float ly = y + h / 2.0f - 250.0f;
			m_Logo->Draw(lx, ly, logoSize, logoSize);
		}

		std::string title = "Welcome to Frostnux!";
		float titleW = TextRenderer::Get().GetTextWidth(title);
		float tx = x + (w - titleW) / 2.0f;
		float ty = y + h / 2.0f - 120.0f;
		TextRenderer::Get().DrawText(title, tx, ty, 1.0f, 1.0f, 1.0f, 1.0f);

		for (const auto& btn : m_Buttons)
		{
			Color bgColor = btn.hovered ? Color{ 0.22f, 0.36f, 0.55f, 1.0f } : Color{ 0.15f, 0.15f, 0.17f, 1.0f };

			DrawRect(btn.rect.x - 1, btn.rect.y - 1, btn.rect.w + 2, btn.rect.h + 2, { 0.35f, 0.35f, 0.40f, 1.0f });

			DrawRect(btn.rect.x, btn.rect.y, btn.rect.w, btn.rect.h, bgColor);

			float textY = btn.rect.y + (btn.rect.h - TextRenderer::Get().GetTextHeight()) / 2.0f - 6.0f;
			TextRenderer::Get().DrawText(btn.label, btn.rect.x + 16.0f, textY, 0.95f, 0.95f, 0.95f, 1.0f);

			TextRenderer::Get().DrawText(btn.desc, btn.rect.x + 160.0f, textY + 2.0f, 0.55f, 0.55f, 0.60f, 1.0f);
		}
	}

	bool WelcomePage::OnEvent(Event& event)
	{
		if (event.GetEventType() == EventType::MouseMoved)
		{
			auto& me = static_cast<MouseMovedEvent&>(event);
			bool handled = false;
			for (auto& btn : m_Buttons)
			{
				btn.hovered = btn.rect.contains(me.GetX(), me.GetY());
				if (btn.hovered) handled = true;
			}
			return handled;
		}

		if (event.GetEventType() == EventType::MouseButtonPressed)
		{
			auto& me = static_cast<MouseButtonPressedEvent&>(event);
			if (me.GetMouseButton() != GLFW_MOUSE_BUTTON_LEFT) return false;

			for (auto& btn : m_Buttons)
			{
				if (btn.rect.contains(me.GetMouseX(), me.GetMouseY()))
				{
					if (btn.onClick) btn.onClick();
					return true;
				}
			}
		}

		return false;
	}

	void WelcomePage::DrawRect(float x, float y, float w, float h, Color c)
	{
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(c.r, c.g, c.b, c.a);
		glBegin(GL_QUADS);
		glVertex2f(x, y);
		glVertex2f(x + w, y);
		glVertex2f(x + w, y + h);
		glVertex2f(x, y + h);
		glEnd();
	}

}
