#include <fxpch.h>
#include "TextInputDialog.h"
#include "TextRenderer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Frostnux/Core/Application.h"
#include "Frostnux/Events/MouseEvent.h"
#include "Frostnux/Events/KeyEvent.h"
#include "Frostnux/Renderer/Renderer2D.h"

namespace Frostnux {

	TextInputDialog& TextInputDialog::Get()
	{
		static TextInputDialog instance;
		return instance;
	}

	void TextInputDialog::Open(const std::string& title, const std::string& defaultText, std::function<void(const std::string&)> onConfirm)
	{
		m_Title = title;
		m_Text = defaultText;
		m_Caret = static_cast<int>(m_Text.size());
		m_OnConfirm = std::move(onConfirm);
		m_IsOpen = true;

		auto& app = Application::Get();
		m_X = (app.GetWindow().GetWidth() - m_W) / 2.0f;
		m_Y = (app.GetWindow().GetHeight() - m_H) / 2.0f;
	}

	void TextInputDialog::Close()
	{
		m_IsOpen = false;
		m_OnConfirm = nullptr;
	}

	bool TextInputDialog::OnEvent(Event& event)
	{
		if (!m_IsOpen) return false;

		if (event.GetEventType() == EventType::Char)
		{
			auto& e = static_cast<CharEvent&>(event);
			if (e.GetCharCode() >= 32 && e.GetCharCode() <= 126)
			{
				m_Text.insert(m_Text.begin() + m_Caret, static_cast<char>(e.GetCharCode()));
				m_Caret++;
			}
			return true;
		}

		if (event.GetEventType() == EventType::KeyPressed)
		{
			auto& e = static_cast<KeyPressedEvent&>(event);
			int key = e.GetKeyCode();

			if (key == GLFW_KEY_ESCAPE)
			{
				Close();
				return true;
			}
			if (key == GLFW_KEY_ENTER)
			{
				if (!m_Text.empty() && m_OnConfirm)
				{
					auto& fn = m_OnConfirm;
					std::string text = m_Text;
					Close();
					fn(text);
				}
				else Close();
				return true;
			}
			if (key == GLFW_KEY_BACKSPACE)
			{
				if (m_Caret > 0)
				{
					m_Text.erase(m_Caret - 1, 1);
					m_Caret--;
				}
				return true;
			}
			if (key == GLFW_KEY_LEFT)
			{
				if (m_Caret > 0) m_Caret--;
				return true;
			}
			if (key == GLFW_KEY_RIGHT)
			{
				if (m_Caret < m_Text.size()) m_Caret++;
				return true;
			}
			return true;
		}

		if (event.GetEventType() == EventType::MouseButtonPressed || event.GetEventType() == EventType::MouseMoved)
		{
			return true;
		}

		return true;
	}

	void TextInputDialog::Draw()
	{
		if (!m_IsOpen) return;

		auto& app = Application::Get();
		float winW = (float)app.GetWindow().GetWidth();
		float winH = (float)app.GetWindow().GetHeight();
		Renderer2D::DrawQuad(0.0f, 0.0f, winW, winH, 0.0f, 0.0f, 0.0f, 0.4f);

		nlohmann::json j;
		std::string path = "Resources/Languages/" + LanguageManager::GetLanguageCode() + ".json";
		std::ifstream file(path);
		j << file;

		Renderer2D::DrawQuad(m_X - 2, m_Y - 2, m_W + 4, m_H + 4, 0.35f, 0.35f, 0.40f, 1.0f);
		Renderer2D::DrawQuad(m_X, m_Y, m_W, m_H, 0.16f, 0.16f, 0.18f, 1.0f);

		TextRenderer::Get().DrawTextUTF8(m_Title, m_X + 20, m_Y + 20, 0.9f, 0.9f, 0.9f, 1.0f);

		float inputY = m_Y + 60;
		Renderer2D::DrawQuad(m_X + 20, inputY, m_W - 40, 30.0f, 0.1f, 0.1f, 0.1f, 1.0f);

		TextRenderer::Get().DrawTextUTF8(m_Text, m_X + 25, inputY + 8, 1.0f, 1.0f, 1.0f, 1.0f);

		float caretX = m_X + 25 + TextRenderer::Get().GetTextWidth(m_Text.substr(0, m_Caret));
		if (std::fmod(glfwGetTime(), 1.0) < 0.5)
		{
			Renderer2D::DrawQuad(caretX, inputY + 6.0f, 2.0f, 20.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		}

		TextRenderer::Get().DrawTextUTF8(j.value("Input Tip", "Press Enter to confirm, Esc to cancel."), m_X + 20, m_Y + 110, 0.5f, 0.5f, 0.5f, 1.0f);
	}

}
