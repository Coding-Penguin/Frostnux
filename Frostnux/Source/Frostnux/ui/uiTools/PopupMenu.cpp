#include <fxpch.h>
#include "PopupMenu.h"
#include "TextRenderer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Frostnux/Core/Application.h"
#include "Frostnux/Core/Log.h"
#include "Frostnux/Events/Event.h"
#include "Frostnux/Events/MouseEvent.h"
#include "Frostnux/Events/KeyEvent.h"
#include "Frostnux/Renderer/Renderer2D.h"

namespace Frostnux {

	PopupMenu& PopupMenu::Get()
	{
		static PopupMenu instance;
		return instance;
	}

	static constexpr float kRowH = 24.0f;
	static constexpr float kPadX = 12.0f;
	static constexpr float kMinW = 160.0f;
	static constexpr float kSepH = 6.0f;

	namespace {

		void MeasureMenu(const std::vector<PopupMenu::Item>& items,
			float& outW, float& outH)
		{
			float w = kMinW;
			float h = 4.0f;
			for (const auto& it : items)
			{
				if (it.separator) { h += kSepH; continue; }
				const float tw = TextRenderer::Get().GetTextWidth(it.label);
				const float sw = it.shortcut.empty() ? 0.0f : TextRenderer::Get().GetTextWidth(it.shortcut);
				float rowW = tw + sw + kPadX * 3 + 40.0f;
				if (!it.submenu.empty()) rowW += 16.0f;
				if (rowW > w) w = rowW;
				h += kRowH;
			}
			outW = w;
			outH = h;
		}

	}

	void PopupMenu::Open(float x, float y, std::vector<Item> items)
	{
		m_Items = std::move(items);
		m_Open = true;
		m_Hovered = -1;
		m_Pressed = -1;
		m_SubmenuOwner = -1;
		CloseSubmenu();

		MeasureMenu(m_Items, m_W, m_H);

		m_X = x;
		m_Y = y;

		auto& app = Application::Get();
		const int winW = app.GetWindow().GetWidth();
		const int winH = app.GetWindow().GetHeight();
		if (m_X + m_W > winW) m_X = winW - m_W - 4.0f;
		if (m_Y + m_H > winH) m_Y = winH - m_H - 4.0f;
		if (m_X < 0) m_X = 4.0f;
		if (m_Y < 0) m_Y = 4.0f;
	}

	void PopupMenu::Close()
	{
		m_Open = false;
		m_Items.clear();
		m_Hovered = -1;
		m_Pressed = -1;
		m_SubmenuOwner = -1;
		CloseSubmenu();
	}

	void PopupMenu::CloseAll()
	{
		Close();
		if (m_Parent) m_Parent->CloseAll();
	}

	void PopupMenu::CloseSubmenu()
	{
		if (m_Submenu) m_Submenu->Close();
		m_SubmenuOwner = -1;
	}

	bool PopupMenu::ContainsPoint(float mx, float my) const
	{
		return m_Open && mx >= m_X && mx <= m_X + m_W && my >= m_Y && my <= m_Y + m_H;
	}

	bool PopupMenu::HandleEvent(void* rawEvent)
	{
		if (!m_Open) return false;

		auto& e = *static_cast<Event*>(rawEvent);

		if (m_Submenu && m_Submenu->IsOpen())
		{
			if (m_Submenu->HandleEvent(rawEvent))
				return true;
		}

		if (e.GetEventType() == EventType::MouseMoved)
		{
			auto& me = static_cast<MouseMovedEvent&>(e);
			const float mx = me.GetX();
			const float my = me.GetY();

			if (!ContainsPoint(mx, my))
			{
				if (!m_Submenu || !m_Submenu->IsOpen())
				{
					m_Hovered = -1;
				}
				return false;
			}

			int idx = -1;
			float y = m_Y + 2.0f;
			for (int i = 0; i < static_cast<int>(m_Items.size()); ++i)
			{
				const auto& it = m_Items[i];
				if (it.separator) { y += kSepH; continue; }
				if (my >= y && my < y + kRowH) { idx = i; break; }
				y += kRowH;
			}

			if (idx != m_Hovered)
			{
				m_Hovered = idx;
				CloseSubmenu();

				if (idx >= 0 && !m_Items[idx].submenu.empty())
				{
					if (!m_Submenu)
						m_Submenu.reset(new PopupMenu());
					m_Submenu->SetParent(this);

					float itemY = m_Y + 2.0f;
					for (int i = 0; i < idx; ++i)
					{
						if (m_Items[i].separator) itemY += kSepH;
						else itemY += kRowH;
					}
					m_Submenu->Open(m_X + m_W - 4.0f, itemY, m_Items[idx].submenu);
					m_SubmenuOwner = idx;
				}
			}
			return true;
		}

		if (e.GetEventType() == EventType::MouseButtonPressed)
		{
			auto& me = static_cast<MouseButtonPressedEvent&>(e);
			if (me.GetMouseButton() != GLFW_MOUSE_BUTTON_LEFT) return true;

			const float mx = me.GetMouseX();
			const float my = me.GetMouseY();

			if (!ContainsPoint(mx, my))
			{
				CloseAll();
				return true;
			}

			m_Pressed = m_Hovered;
			return true;
		}

		if (e.GetEventType() == EventType::MouseButtonReleased)
		{
			auto& me = static_cast<MouseButtonReleasedEvent&>(e);
			if (me.GetMouseButton() != GLFW_MOUSE_BUTTON_LEFT) return true;

			const int release = m_Hovered;
			if (release >= 0 && release == m_Pressed)
			{
				const auto& it = m_Items[release];
				if (it.separator || !it.enabled || it.isHeader) { m_Pressed = -1; return true; }

				if (!it.submenu.empty())
				{
					m_Pressed = -1;
					return true;
				}

				if (it.onClick)
				{
					auto& fn = it.onClick;
					fn();
					CloseAll();
					return true;
				}
			}
			m_Pressed = -1;
			return true;
		}

		if (e.GetEventType() == EventType::KeyPressed)
		{
			auto& ke = static_cast<KeyPressedEvent&>(e);
			if (ke.GetKeyCode() == FX_KEY_ESCAPE)
			{
				if (m_Submenu && m_Submenu->IsOpen())
				{
					m_Submenu->Close();
					m_SubmenuOwner = -1;
				}
				else
				{
					CloseAll();
				}
				return true;
			}
		}

		return false;
	}

	void PopupMenu::Draw()
	{
		if (!m_Open) return;

		Renderer2D::DrawQuad(m_X - 1, m_Y - 1, m_W + 2, m_H + 2, 0.35f, 0.35f, 0.40f, 1.0f);
		Renderer2D::DrawQuad(m_X, m_Y, m_W, m_H, 0.16f, 0.16f, 0.18f, 0.98f);

		float y = m_Y + 2.0f;
		for (int i = 0; i < static_cast<int>(m_Items.size()); ++i)
		{
			const auto& it = m_Items[i];

			if (it.separator)
			{
				Renderer2D::DrawQuad(m_X + 6, y + kSepH * 0.5f - 0.5f, m_W - 12, 1.0f, 0.35f, 0.35f, 0.40f, 1.0f);
				y += kSepH;
				continue;
			}

			if (i == m_Hovered && it.enabled && !it.isHeader)
			{
				Renderer2D::DrawQuad(m_X + 2, y, m_W - 4, kRowH, 0.22f, 0.36f, 0.55f, 1.0f);
			}

			float cr = 0.90f, cg = 0.90f, cb = 0.90f;
			if (it.isHeader) { cr = 0.55f; cg = 0.55f; cb = 0.55f; }
			else if (!it.enabled) { cr = 0.45f; cg = 0.45f; cb = 0.45f; }

			TextRenderer::Get().DrawTextUTF8(it.label, m_X + kPadX, y + 4.0f, cr, cg, cb, 1.0f);

			if (!it.shortcut.empty())
			{
				const float sw = TextRenderer::Get().GetTextWidth(it.shortcut);
				TextRenderer::Get().DrawTextUTF8(it.shortcut, m_X + m_W - sw - kPadX - (it.submenu.empty() ? 0.0f : 14.0f), y + 4.0f, 0.55f, 0.55f, 0.60f, 1.0f);
			}

			if (!it.submenu.empty())
			{
				Renderer2D::DrawTriangle(m_X + m_W - 12, y + kRowH * 0.5f - 4.0f, m_X + m_W - 12, y + kRowH * 0.5f + 4.0f, m_X + m_W - 6, y + kRowH * 0.5f, 0.75f, 0.75f, 0.80f, 1.0f);
			}

			y += kRowH;
		}

		if (m_Submenu && m_Submenu->IsOpen())
			m_Submenu->Draw();
	}

}
