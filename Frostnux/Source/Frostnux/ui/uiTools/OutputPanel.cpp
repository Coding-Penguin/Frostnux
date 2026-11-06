#include "fxpch.h"
#include "OutputPanel.h"
#include "TextRenderer.h"
#include "Frostnux/Renderer/Renderer2D.h"
#include "Frostnux/Core/Application.h"
#include "Frostnux/Events/MouseEvent.h"
#include "../CodeEditor/CodeEditor.h"
#include <glad/glad.h>

namespace Frostnux {

	OutputPanel::OutputPanel(const std::string& name)
		: uiWindow(name) {}

	OutputPanel::~OutputPanel() {}

	void OutputPanel::AppendLine(const std::string& text)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Lines.push_back(text);

		if (m_Lines.size() > 5000)
			m_Lines.erase(m_Lines.begin(), m_Lines.begin() + 1000);

		if (m_ScrollY >= 0.0f)
			m_ScrollY += m_LineHeight;
	}

	void OutputPanel::Clear()
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Lines.clear();
		m_ScrollY = -1.0f;
	}

	void OutputPanel::OnUpdate(float deltaTime)
	{
		DrawContent();
	}

	void OutputPanel::DrawContent()
	{
		uiWindow::DrawContent();

		float contentX = m_RectX + 5;
		float contentY = m_RectY + 30;
		float contentW = m_RectWidth - 10;
		float contentH = m_RectHeight - 35;

		Application& app = Application::Get();
		int winHeight = app.GetWindow().GetHeight();

		glEnable(GL_SCISSOR_TEST);
		glScissor((int)contentX, winHeight - (int)(contentY + contentH), (int)contentW, (int)contentH);

		std::lock_guard<std::mutex> lock(m_Mutex);

		m_TotalHeight = m_Lines.size() * m_LineHeight;
		float maxScroll = std::max(0.0f, m_TotalHeight - contentH);

		if (m_ScrollY < 0.0f)
			m_ScrollY = maxScroll;

		m_ScrollY = std::clamp(m_ScrollY, 0.0f, maxScroll);

		float startY = contentY - m_ScrollY;

		bool isDark = ThemeManager::IsDarkTheme();
		Color textColor = isDark ? Color { 0.8f, 0.8f, 0.8f, 1.0f } : Color { 0.2f, 0.2f, 0.2f, 1.0f };

		for (size_t i = 0; i < m_Lines.size(); ++i)
		{
			float y = startY + i * m_LineHeight;
			if (y + m_LineHeight < contentY) continue;
			if (y > contentY + contentH) break;

			TextRenderer::Get().DrawTextUTF8(m_Lines[i], contentX + 2, y + 2, textColor.r, textColor.g, textColor.b, textColor.a);
		}

		glDisable(GL_SCISSOR_TEST);
	}

}
