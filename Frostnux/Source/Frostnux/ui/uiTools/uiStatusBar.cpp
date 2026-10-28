#include <fxpch.h>
#include "uiStatusBar.h"
#include "Frostnux/Renderer/Renderer2D.h"
#include "Frostnux/Core/Application.h"
#include "Frostnux/Core/Log.h"
#include "TextRenderer.h"
#include "Frostnux/Managers/Theme.h"

namespace Frostnux
{

	uiStatusBar::uiStatusBar()
		: Layer("StatusBar")
	{
	}

	uiStatusBar::~uiStatusBar()
	{
	}

	void uiStatusBar::OnAttach()
	{
		FX_CORE_INFO("uiStatusBar attached");
	}

	void uiStatusBar::OnDetach()
	{
		FX_CORE_INFO("uiStatusBar detached");
	}

	void uiStatusBar::OnUpdate(float deltaTime)
	{
		Application &app = Application::Get();
		int width = (int)app.GetWindow().GetWidth();
		int height = (int)app.GetWindow().GetHeight();
		if (width == 0 || height == 0)
			return;

		DrawProgress();
		DrawText();
	}

	bool uiStatusBar::OnEvent(Event &event)
	{
		return false;
	}

	void uiStatusBar::SetStatusText(const std::string &text)
	{
		m_StatusText = text;
	}

	void uiStatusBar::SetProgress(float progress)
	{
		m_Progress = std::clamp(progress, -1.0f, 1.0f);
	}

	void uiStatusBar::DrawText()
	{
		Application &app = Application::Get();
		float width = (int)app.GetWindow().GetWidth();
		float height = (int)app.GetWindow().GetHeight();
		float y = height - m_Height;
		float textY = y + (m_Height - TextRenderer::Get().GetTextHeight()) / 2;

		float r, g, b;
		if (ThemeManager::IsDarkTheme())
		{
			r = g = b = 0.85f;
		}
		else
		{
			r = g = b = 0.15f;
		}

		float leftX = 10.0f;
		TextRenderer::Get().DrawTextUTF8(m_StatusText, leftX, textY, r, g, b, 1.0f);
		
		float rightTextOffset = 10.0f;
		GLFWwindow* win = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());
		if (glfwGetWindowAttrib(win, GLFW_MAXIMIZED) != GLFW_TRUE)
		{
			rightTextOffset = 40.0f;

			float gripSize = 25.0f;
			float startX = width - 5.0f - gripSize;
			float startY = textY + m_Height - gripSize;

			Renderer2D::DrawTriangle(startX + 2.5f, textY + m_Height - 15.0f, width - 5.0f, startY - 10.0f, width - 5.0f, textY + m_Height - 15.0f, 0.5f, 0.5f, 0.5f, 1.0f);
		}
	}

	void uiStatusBar::DrawProgress() const
	{
		if (m_Progress < 0.0f)
			return;

		Application &app = Application::Get();
		int width = (int)app.GetWindow().GetWidth();
		int height = (int)app.GetWindow().GetHeight();
		float y = height - m_Height;

		float rightTextWidth = 0;
		float rightX = width - rightTextWidth - 20.0f;
		float leftX = rightX - 150.f;
		float barWidth = rightX - leftX;
		if ((barWidth < 0.0f) || (leftX < (TextRenderer::Get().GetTextWidth(m_StatusText) + 5.0f)))
			return;

		float barHeight = 15.0f;
		float barY = y + (m_Height - barHeight) / 2;

		if (ThemeManager::IsDarkTheme())
		{
			Renderer2D::DrawQuad(leftX, barY, barWidth, barHeight, 0.3f, 0.3f, 0.35f, 0.7f);
		}
		else
		{
			Renderer2D::DrawQuad(leftX, barY, barWidth, barHeight, 0.7f, 0.7f, 0.75f, 0.7f);
		}

		float fillWidth = barWidth * m_Progress;
		Renderer2D::DrawQuad(leftX, barY, fillWidth, barHeight, 0.2f, 0.5f, 0.95f, 1.0f);
	}

}
