#include <fxpch.h>
#include "uiButton.h"
#include "MouseCircle.h"
#include "TextRenderer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

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

		if (m_Hovered)
		{
			r = std::min(r + 0.05f, 1.0f);
			g = std::min(g + 0.05f, 1.0f);
			b = std::min(b + 0.05f, 1.0f);
		}

		if (m_ButtonStyle != ButtonStyles::NoBackgroundOrLine)
		{
			if (m_ButtonStyle != ButtonStyles::NoBackground)
			{
				glColor4f(r, g, b, m_Color[3]);
				glBegin(GL_QUADS);
				glVertex2f(absX, absY);
				glVertex2f(absX + m_Width, absY);
				glVertex2f(absX + m_Width, absY + m_Height);
				glVertex2f(absX, absY + m_Height);
				glEnd();
			}
			else
			{
				if (m_Hovered)
				{
					glColor4f(r, g, b, m_Color[3] - 0.1f);
					glBegin(GL_QUADS);
					glVertex2f(absX, absY);
					glVertex2f(absX + m_Width, absY);
					glVertex2f(absX + m_Width, absY + m_Height);
					glVertex2f(absX, absY + m_Height);
					glEnd();
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
				glLineWidth(1.0f);
				glBegin(GL_LINES);

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

						glColor4f(highlightColor[0], highlightColor[1], highlightColor[2], alpha);
						glVertex2f(x1, y1);
						glVertex2f(x2, y2);
					};

				for (int i = 0; i < segments; ++i)
				{
					float t0 = (float)i / segments;
					float t1 = (float)(i + 1) / segments;
					float x0 = absX + m_Width * t0;
					float y0 = absY;
					float x1 = absX + m_Width * t1;
					float y1 = absY;
					drawSegment(x0, y0, x1, y1);
				}

				for (int i = 0; i < segments; ++i)
				{
					float t0 = (float)i / segments;
					float t1 = (float)(i + 1) / segments;
					float x0 = absX + m_Width * t0;
					float y0 = absY + m_Height;
					float x1 = absX + m_Width * t1;
					float y1 = absY + m_Height;
					drawSegment(x0, y0, x1, y1);
				}

				for (int i = 0; i < segments; ++i)
				{
					float t0 = (float)i / segments;
					float t1 = (float)(i + 1) / segments;
					float x0 = absX;
					float y0 = absY + m_Height * t0;
					float x1 = absX;
					float y1 = absY + m_Height * t1;
					drawSegment(x0, y0, x1, y1);
				}

				for (int i = 0; i < segments; ++i)
				{
					float t0 = (float)i / segments;
					float t1 = (float)(i + 1) / segments;
					float x0 = absX + m_Width;
					float y0 = absY + m_Height * t0;
					float x1 = absX + m_Width;
					float y1 = absY + m_Height * t1;
					drawSegment(x0, y0, x1, y1);
				}

				glEnd();
			}
			else
			{
				auto& circle = MouseCircle::Get();
				bool highlighted = circle.IsIntersectingRect(absX, absY, m_Width, m_Height);
				if (highlighted)
				{
					glColor4f(0.5f, 0.5f, 0.5f, 0.3f);
				}
				else
				{
					glColor4f(0.3f, 0.3f, 0.3f, 0.05f);
				}
				glLineWidth(1.0f);
				glBegin(GL_LINE_LOOP);
				glVertex2f(absX, absY);
				glVertex2f(absX + m_Width, absY);
				glVertex2f(absX + m_Width, absY + m_Height);
				glVertex2f(absX, absY + m_Height);
				glEnd();
			}
		}
		else
		{
			if (m_Hovered)
			{
				glColor4f(r, g, b, m_Color[3] - 0.1f);
				glBegin(GL_QUADS);
				glVertex2f(absX, absY);
				glVertex2f(absX + m_Width, absY);
				glVertex2f(absX + m_Width, absY + m_Height);
				glVertex2f(absX, absY + m_Height);
				glEnd();
			}

			auto& circle = MouseCircle::Get();
			float cx = circle.GetX(), cy = circle.GetY(), radius = circle.GetRadius();

			const float defaultColor[3] = { 0.3f, 0.3f, 0.3f };
			const float highlightColor[3] = { 0.5f, 0.5f, 0.5f };

			int segments = std::max(30, (int)(std::max(m_Width, m_Height) / 5.0f));
			glLineWidth(1.0f);
			glBegin(GL_LINES);

			auto drawSegment = [&](float x1, float y1, float x2, float y2)
				{
					bool intersect = IsSegmentIntersectCircle(x1, y1, x2, y2, cx, cy, radius);
					if (intersect)
						glColor4f(highlightColor[0], highlightColor[1], highlightColor[2], 0.5f);
					else
						glColor4f(defaultColor[0], defaultColor[1], defaultColor[2], 0.05f);
					glVertex2f(x1, y1);
					glVertex2f(x2, y2);
				};

			for (int i = 0; i < segments; ++i)
			{
				float t0 = (float)i / segments;
				float t1 = (float)(i + 1) / segments;
				float x0 = absX + m_Width * t0;
				float y0 = absY;
				float x1 = absX + m_Width * t1;
				float y1 = absY;
				drawSegment(x0, y0, x1, y1);
			}

			for (int i = 0; i < segments; ++i)
			{
				float t0 = (float)i / segments;
				float t1 = (float)(i + 1) / segments;
				float x0 = absX + m_Width * t0;
				float y0 = absY + m_Height;
				float x1 = absX + m_Width * t1;
				float y1 = absY + m_Height;
				drawSegment(x0, y0, x1, y1);
			}

			for (int i = 0; i < segments; ++i)
			{
				float t0 = (float)i / segments;
				float t1 = (float)(i + 1) / segments;
				float x0 = absX;
				float y0 = absY + m_Height * t0;
				float x1 = absX;
				float y1 = absY + m_Height * t1;
				drawSegment(x0, y0, x1, y1);
			}

			for (int i = 0; i < segments; ++i)
			{
				float t0 = (float)i / segments;
				float t1 = (float)(i + 1) / segments;
				float x0 = absX + m_Width;
				float y0 = absY + m_Height * t0;
				float x1 = absX + m_Width;
				float y1 = absY + m_Height * t1;
				drawSegment(x0, y0, x1, y1);
			}

			glEnd();
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
		TextRenderer::Get().DrawText(m_Text, textX, textY, tr, tr, tr, 1.0f);
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
			if (e.GetMouseButton() == GLFW_MOUSE_BUTTON_LEFT && IsPointInside(e.GetMouseX(), e.GetMouseY(), parentX, parentY))
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

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0.1f, 0.1f, 0.1f, 0.7f);
		glBegin(GL_QUADS);
		glVertex2f(tooltipX, tooltipY);
		glVertex2f(tooltipX + tooltipW, tooltipY);
		glVertex2f(tooltipX + tooltipW, tooltipY + tooltipH);
		glVertex2f(tooltipX, tooltipY + tooltipH);
		glEnd();

		float textX = tooltipX + padding;
		float textY = tooltipY + (tooltipH - textHeight) / 2;
		TextRenderer::Get().DrawText(m_Tooltip, textX, textY, 1.0f, 1.0f, 1.0f, 0.9f);
	}

}