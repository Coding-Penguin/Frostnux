#include <fxpch.h>
#include "uiButton.h"
#include "MouseCircle.h"
#include "TextRenderer.h"
#include "Frostnux/Renderer/Renderer2D.h"
#include "Frostnux/Core/MouseButtonCodes.h"

namespace Frostnux {

	// uiButton
	uiButton::uiButton(const std::string& text, float x, float y, float width, float height, ButtonStyles style)
		: m_Text(text), m_X(x), m_Y(y), m_Width(width), m_Height(height), m_ButtonStyle(style)
	{
		if (ThemeManager::IsDarkTheme())
		{
			m_Color[0] = 0.1f;
			m_Color[1] = 0.1f;
			m_Color[2] = 0.1f;
			m_Color[3] = 1.0f;
		}
		else
		{
			m_Color[0] = 0.9f;
			m_Color[1] = 0.9f;
			m_Color[2] = 0.9f;
			m_Color[3] = 1.0f;
		}
	}

	uiButton::~uiButton() {}

	static bool IsSegmentIntersectCircle(float x1, float y1, float x2, float y2,
		float cx, float cy, float radius)
	{
		float dx = x2 - x1;
		float dy = y2 - y1;
		float t = ((cx - x1) * dx + (cy - y1) * dy) / (dx * dx + dy * dy);
		t = std::max(0.0f, std::min(1.0f, t));
		float closestX = x1 + t * dx;
		float closestY = y1 + t * dy;
		float distSq = (closestX - cx) * (closestX - cx) + (closestY - cy) * (closestY - cy);
		return distSq <= radius * radius;
	}

	void uiButton::Draw(float parentX, float parentY) const
	{
		float absX = parentX + m_X;
		float absY = parentY + m_Y;

		float r = m_Color[0], g = m_Color[1], b = m_Color[2];

		if (m_UseThemeColor)
		{
			if (ThemeManager::IsDarkTheme())
				r = g = b = 0.10f;
			else
				r = g = b = 0.90f;
		}

		if (m_Hovered)
		{
			float delta = ThemeManager::IsDarkTheme() ? 0.05f : -0.05f;
			r = std::clamp(r + delta, 0.0f, 1.0f);
			g = std::clamp(g + delta, 0.0f, 1.0f);
			b = std::clamp(b + delta, 0.0f, 1.0f);
		}

		if (m_ButtonStyle != ButtonStyles::NoBackgroundOrLine)
		{
			if (m_ButtonStyle != ButtonStyles::NoBackground)
			{
				Renderer2D::DrawQuad(absX, absY, m_Width, m_Height, r, g, b, m_Color[3]);
			}
			else
			{
				if (m_Hovered)
				{
					Renderer2D::DrawQuad(absX, absY, m_Width, m_Height, r, g, b, m_Color[3] - 0.1f);
				}
			}

			if (m_ButtonStyle != ButtonStyles::NoLine)
			{
				auto& circle = MouseCircle::Get();
				float cx = circle.GetX(), cy = circle.GetY();
				float radius = circle.GetRadius();

				const float highlightColor[3] = { 0.3f, 0.3f, 0.3f };
				const float minAlpha = 0.01f;
				const float maxAlpha = 0.3f;

				int segments = std::max(4, (int)(std::max(m_Width, m_Height) / 8.0f));

				auto drawSegment = [&](float x1, float y1, float x2, float y2)
					{
						float dx = x2 - x1;
						float dy = y2 - y1;
						float lenSq = dx * dx + dy * dy;
						float t = 0.0f;
						if (lenSq > 0.0f)
						{
							t = ((cx - x1) * dx + (cy - y1) * dy) / lenSq;
							t = std::max(0.0f, std::min(1.0f, t));
						}
						float closestX = x1 + t * dx;
						float closestY = y1 + t * dy;
						float dist = std::sqrt((closestX - cx) * (closestX - cx) + (closestY - cy) * (closestY - cy));

						float alpha;
						if (dist >= radius)
						{
							alpha = minAlpha;
						}
						else
						{
							float factor = 1.0f - (dist / radius);
							alpha = minAlpha + factor * (maxAlpha - minAlpha);
						}
						Renderer2D::DrawLine(x1, y1, x2, y2, 1.0f, highlightColor[0], highlightColor[1], highlightColor[2], alpha);
					};
				for (int i = 0; i < segments; ++i)
				{
					float t0 = (float)i / segments;
					float t1 = (float)(i + 1) / segments;

					drawSegment(absX + m_Width * t0, absY, absX + m_Width * t1, absY);
					drawSegment(absX + m_Width * t0, absY + m_Height, absX + m_Width * t1, absY + m_Height);
					drawSegment(absX, absY + m_Height * t0, absX, absY + m_Height * t1);
					drawSegment(absX + m_Width, absY + m_Height * t0, absX + m_Width, absY + m_Height * t1);
				}
			}
			else
			{
				auto& circle = MouseCircle::Get();
				bool highlighted = circle.IsIntersectingRect(absX, absY, m_Width, m_Height);

				float cr, cg, cb, ca;
				if (highlighted)
				{
					cr = 0.5f; cg = 0.5f; cb = 0.5f; ca = 0.3f;
				}
				else
				{
					cr = 0.3f; cg = 0.3f; cb = 0.3f; ca = 0.05f;
				}

				Renderer2D::DrawLine(absX, absY, absX + m_Width, absY, 1.0f, cr, cg, cb, ca);
				Renderer2D::DrawLine(absX, absY + m_Height, absX + m_Width, absY + m_Height, 1.0f, cr, cg, cb, ca);
				Renderer2D::DrawLine(absX, absY, absX, absY + m_Height, 1.0f, cr, cg, cb, ca);
				Renderer2D::DrawLine(absX + m_Width, absY, absX + m_Width, absY + m_Height, 1.0f, cr, cg, cb, ca);
			}
		}
		else
		{
			if (m_Hovered)
			{
				Renderer2D::DrawQuad(absX, absY, m_Width, m_Height, r, g, b, m_Color[3] - 0.1f);
			}

			auto& circle = MouseCircle::Get();
			float cx = circle.GetX(), cy = circle.GetY(), radius = circle.GetRadius();

			const float defaultColor[3] = { 0.3f, 0.3f, 0.3f };
			const float highlightColor[3] = { 0.5f, 0.5f, 0.5f };

			int segments = std::max(30, (int)(std::max(m_Width, m_Height) / 5.0f));

			auto drawSegment = [&](float x1, float y1, float x2, float y2)
				{
					bool intersect = IsSegmentIntersectCircle(x1, y1, x2, y2, cx, cy, radius);

					if (intersect)
						Renderer2D::DrawLine(x1, y1, x2, y2, 1.0f, highlightColor[0], highlightColor[1], highlightColor[2], 0.5f);
					else
						Renderer2D::DrawLine(x1, y1, x2, y2, 1.0f, defaultColor[0], defaultColor[1], defaultColor[2], 0.05f);
				};

			for (int i = 0; i < segments; ++i)
			{
				float t0 = (float)i / segments;
				float t1 = (float)(i + 1) / segments;

				drawSegment(absX + m_Width * t0, absY, absX + m_Width * t1, absY);
				drawSegment(absX + m_Width * t0, absY + m_Height, absX + m_Width * t1, absY + m_Height);
				drawSegment(absX, absY + m_Height * t0, absX, absY + m_Height * t1);
				drawSegment(absX + m_Width, absY + m_Height * t0, absX + m_Width, absY + m_Height * t1);
			}
		}

		float textWidth = TextRenderer::Get().GetTextWidth(m_Text);
		float textHeight = TextRenderer::Get().GetTextHeight();
		float textX = absX + (m_Width - textWidth) / 2;
		float textY = absY + (m_Height - textHeight) / 2;

		float tr, tg, tb;
		if (ThemeManager::IsDarkTheme())
		{
			tr = tg = tb = 0.95f;
		}
		else 
		{
			tr = tg = tb = 0.05f;
		}
		TextRenderer::Get().DrawTextUTF8(m_Text, textX, textY, tr, tr, tr, 1.0f);
	}

	bool uiButton::IsPointInside(float px, float py, float parentX, float parentY) const
	{
		float absX = parentX + m_X;
		float absY = parentY + m_Y;
		return (px >= absX && px <= absX + m_Width && py >= absY && py <= absY + m_Height);
	}

	void uiButton::OnUpdate(float parentX, float parentY, bool parentVisible)
	{
		if (!parentVisible) return;
		Draw(parentX, parentY);
	}

	bool uiButton::OnEvent(Event& event, float parentX, float parentY, bool parentVisible)
	{
		if (!parentVisible) return false;

		if (event.GetEventType() == EventType::MouseMoved)
		{
			MouseMovedEvent& e = (MouseMovedEvent&)event;
			bool inside = IsPointInside(e.GetX(), e.GetY(), parentX, parentY);
			if (inside != m_Hovered)
			{
				m_Hovered = inside;
				if (inside)
				{
					m_HoverTimer = 0.0f;
					m_ShowTooltip = false;
				}
				else
				{
					m_ShowTooltip = false;
				}
			}
			return false;
		}
		else if (event.GetEventType() == EventType::MouseButtonPressed)
		{
			MouseButtonPressedEvent& e = (MouseButtonPressedEvent&)event;
			if (e.GetMouseButton() == FX_MOUSE_BUTTON_LEFT && IsPointInside(e.GetMouseX(), e.GetMouseY(), parentX, parentY))
			{
				if (m_Callback) m_Callback();
				return true;
			}
		}
		return false;
	}

	void uiButton::OnTooltipUpdate(float deltaTime) 
	{
		if (m_Hovered && !m_ShowTooltip)
		{
			m_HoverTimer += deltaTime;
			if (m_HoverTimer >= 0.5f)
			{
				m_ShowTooltip = true;
			}
		}
	}

	void uiButton::DrawTooltip(float parentX, float parentY) const
	{
		if (!m_ShowTooltip || m_Tooltip.empty()) return;
		float absX = parentX + m_X;
		float absY = parentY + m_Y;

		float textWidth = TextRenderer::Get().GetTextWidth(m_Tooltip);
		float textHeight = TextRenderer::Get().GetTextHeight();
		float padding = 8.0f;
		float tooltipW = textWidth + padding * 2;
		float tooltipH = textHeight + padding * 2;
		float tooltipX = absX + m_Width / 2 - tooltipW / 2;
		float tooltipY = absY + tooltipH - 5;

		if (tooltipY < 0) tooltipY = absY + m_Height + 5;

		const bool isDark = ThemeManager::IsDarkTheme();
		if (isDark)
			Renderer2D::DrawQuad(tooltipX, tooltipY, tooltipW, tooltipH, 0.10f, 0.10f, 0.10f, 0.85f);
		else
			Renderer2D::DrawQuad(tooltipX, tooltipY, tooltipW, tooltipH, 0.98f, 0.98f, 0.98f, 0.95f);

		float textCol = isDark ? 1.0f : 0.05f;
		float textX = tooltipX + padding;
		float textY = tooltipY + (tooltipH - textHeight) / 2;
		TextRenderer::Get().DrawTextUTF8(m_Tooltip, textX, textY, textCol, textCol, textCol, 0.9f);
	}

}