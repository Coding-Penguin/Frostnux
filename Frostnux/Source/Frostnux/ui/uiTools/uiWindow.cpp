#include <fxpch.h>
#include "uiWindow.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Frostnux/Application.h"
#include "Frostnux/Log.h"

#include "Frostnux/Events/Event.h"
#include "Frostnux/Events/ApplicationEvent.h"
#include "Frostnux/Events/KeyEvent.h"
#include "Frostnux/Events/MouseEvent.h"

#include "MouseCircle.h"
#include "uiTitleBar.h"

namespace Frostnux {

	std::unordered_map<DockRegion, uiWindow*> uiWindow::s_DockedWindows;
	std::vector<uiWindow*> uiWindow::s_FloatingWindows;
	uiWindow* uiWindow::s_DraggingWindow = nullptr;
	uiWindow* uiWindow::s_FocusedFloatingWindow = nullptr;
	DockRegion uiWindow::s_PreviewRegion = DockRegion::None;
	float uiWindow::s_MainX = 0, uiWindow::s_MainY = 0, uiWindow::s_MainW = 0, uiWindow::s_MainH = 0;
	float uiWindow::s_DragStartX = 0, uiWindow::s_DragStartY = 0;
	std::unordered_map<DockRegion, uiWindow::DockArea> uiWindow::s_DockAreas;
	static uiWindow* s_FocusedWindow = nullptr;
	float uiWindow::s_LeftWidth = 300.0f;
	float uiWindow::s_RightWidth = 300.0f;
	float uiWindow::s_BottomHeight = 300.0f;
	float uiWindow::s_DynamicLeftWidth = 0.0f;
	float uiWindow::s_DynamicRightWidth = 0.0f;
	float uiWindow::s_DynamicBottomHeight = 0.0f;
	bool uiWindow::s_ShowDockPanel = false;
	DockRegion uiWindow::s_HighlightedButton = DockRegion::None;
	float uiWindow::s_DockSpacing = 7.0f;
	float uiWindow::s_CenterX = 0;
	float uiWindow::s_CenterY = 0;
	float uiWindow::s_CenterW = 0;
	float uiWindow::s_CenterH = 0;

	uiWindow::uiWindow(std::string name)
		:m_name(name)
	{
		Application& app = Application::Get();
		s_LeftWidth = app.GetWindow().GetWidth() * 0.2f;
		s_RightWidth = app.GetWindow().GetWidth() * 0.2f;
		s_BottomHeight = app.GetWindow().GetHeight() * 0.3f;
	}

	uiWindow::~uiWindow()
	{
		for (auto *btn : m_Buttons)
			delete btn;
		m_Buttons.clear();
	}

	void uiWindow::OnAttach()
	{
		FX_CORE_TRACE("uiWindow attached.");
	}

	void uiWindow::OnDetach()
	{
		FX_CORE_TRACE("uiWindow detached.");
	}

	void uiWindow::OnUpdate(float deltaTime)
	{
		if (!m_IsVisible)
			return;

		Application& app = Application::Get();
		s_LeftWidth = app.GetWindow().GetWidth() * 0.2f;
		s_RightWidth = app.GetWindow().GetWidth() * 0.2f;
		s_BottomHeight = app.GetWindow().GetHeight() * 0.3f;
		DrawContent();
	}

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

	static void DrawHighlightedRectBorder(float x, float y, float w, float h, int segmentsPerSide = 100)
	{
		auto& circle = MouseCircle::Get();
		float cx = circle.GetX(), cy = circle.GetY(), radius = circle.GetRadius();

		const float defaultColor[3] = { 0.3f, 0.3f, 0.3f };
		const float highlightColor[3] = { 0.5f, 0.5f, 0.5f };

		glLineWidth(2.0f);
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

		for (int i = 0; i < segmentsPerSide; ++i)
		{
			float t0 = (float)i / segmentsPerSide;
			float t1 = (float)(i + 1) / segmentsPerSide;
			float x0 = x + w * t0;
			float y0 = y;
			float x1 = x + w * t1;
			float y1 = y;
			drawSegment(x0, y0, x1, y1);
		}

		for (int i = 0; i < segmentsPerSide; ++i)
		{
			float t0 = (float)i / segmentsPerSide;
			float t1 = (float)(i + 1) / segmentsPerSide;
			float x0 = x + w * t0;
			float y0 = y + h;
			float x1 = x + w * t1;
			float y1 = y + h;
			drawSegment(x0, y0, x1, y1);
		}

		for (int i = 0; i < segmentsPerSide; ++i)
		{
			float t0 = (float)i / segmentsPerSide;
			float t1 = (float)(i + 1) / segmentsPerSide;
			float x0 = x;
			float y0 = y + h * t0;
			float x1 = x;
			float y1 = y + h * t1;
			drawSegment(x0, y0, x1, y1);
		}

		for (int i = 0; i < segmentsPerSide; ++i)
		{
			float t0 = (float)i / segmentsPerSide;
			float t1 = (float)(i + 1) / segmentsPerSide;
			float x0 = x + w;
			float y0 = y + h * t0;
			float x1 = x + w;
			float y1 = y + h * t1;
			drawSegment(x0, y0, x1, y1);
		}

		glEnd();
	}

	void uiWindow::DrawContent()
	{
		Application &app = Application::Get();
		int width = (int)app.GetWindow().GetWidth();
		int height = (int)app.GetWindow().GetHeight();

		if (width == 0 || height == 0)
			return;

		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

		glViewport(0, 0, width, height);
		glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
		glPushMatrix();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, width, height, 0, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		const int shadowOffset = 5;
		const int shadowSteps = 7;
		const float startAlpha = 0.1f;
		const float endAlpha = 0.0f;

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		for (int i = 0; i < shadowSteps; ++i)
		{
			float t = (float)i / (shadowSteps - 1);
			float alpha = startAlpha * (1.0f - t) + endAlpha * t;
			float offset = shadowOffset * (1.0f - t);

			glColor4f(0.0f, 0.0f, 0.0f, alpha);
			glBegin(GL_POLYGON);

			glVertex2f(m_RectX, m_RectY);
			glVertex2f(m_RectX + m_RectWidth + offset, m_RectY);
			glVertex2f(m_RectX + m_RectWidth + offset, m_RectY + m_RectHeight + offset);
			glVertex2f(m_RectX, m_RectY + m_RectHeight + offset);
			glEnd();
		}

		int segmentsPerSide = std::max(100, (int)(std::max(m_RectWidth, m_RectHeight) / 5.0f));
		DrawHighlightedRectBorder(m_RectX, m_RectY, m_RectWidth, m_RectHeight, segmentsPerSide);

		int scissorX = (int)m_RectX;
		int scissorY = (int)(height - (m_RectY + m_RectHeight));
		int scissorW = (int)m_RectWidth;
		int scissorH = (int)m_RectHeight;
		glEnable(GL_SCISSOR_TEST);
		glScissor(scissorX, scissorY, scissorW, scissorH);

		auto [r, g, b] = ThemeManager::GetBGColor();
		glColor4f(std::min(r + 0.1f, 1.0f), std::min(g + 0.1f, 1.0f), std::min(b + 0.1f, 1.0f), 0.9f);
		m_Color[0] = std::min(r + 0.1f, 1.0f);
		m_Color[1] = std::min(g + 0.1f, 1.0f);
		m_Color[2] = std::min(b + 0.1f, 1.0f);
		m_Color[3] = 0.9f;

		glBegin(GL_POLYGON);
		glVertex2f(m_RectX, m_RectY);
		glVertex2f(m_RectX + m_RectWidth, m_RectY);
		glVertex2f(m_RectX + m_RectWidth, m_RectY + m_RectHeight);
		glVertex2f(m_RectX, m_RectY + m_RectHeight);
		glEnd();

		glColor4f(((m_Color[0] + 0.001f) > 1.0f ? 1.0f : (m_Color[0] + 0.001f)), ((m_Color[1] + 0.001f) > 1.0f ? 1.0f : (m_Color[1] + 0.001f)), ((m_Color[2] + 0.001f) > 1.0f ? 1.0f : (m_Color[2] + 0.001f)), 1.0f);
		glBegin(GL_QUADS);
		glVertex2f(m_RectX, m_RectY);
		glVertex2f(m_RectX + m_RectWidth, m_RectY);
		glVertex2f(m_RectX + m_RectWidth, m_RectY + 35.0f);
		glVertex2f(m_RectX, m_RectY + 35.0f);
		glEnd();

		DrawTitle();
		DrawResizeGrip();

		float closeX = m_RectX + m_RectWidth - m_CloseButtonSize - 5;
		float closeY = m_RectY + (30.0f - m_CloseButtonSize) / 2;

		if (m_CloseButtonHovered)
		{
			glColor4f(1.0f, 0.2f, 0.2f, 0.9f);
			glBegin(GL_QUADS);
			glVertex2f(closeX, closeY);
			glVertex2f(closeX + m_CloseButtonSize, closeY);
			glVertex2f(closeX + m_CloseButtonSize, closeY + m_CloseButtonSize);
			glVertex2f(closeX, closeY + m_CloseButtonSize);
			glEnd();
		}

		float crossR, crossG, crossB;
		if (ThemeManager::IsDarkTheme())
		{
			crossR = 1.0f;
			crossG = 1.0f;
			crossB = 1.0f;
		}
		else
		{
			crossR = 0.0f;
			crossG = 0.0f;
			crossB = 0.0f;
		}
		glColor4f(crossR, crossG, crossB, 1.0f);
		glLineWidth(2.0f);
		glBegin(GL_LINES);
		glVertex2f(closeX + 5, closeY + 5);
		glVertex2f(closeX + m_CloseButtonSize - 5, closeY + m_CloseButtonSize - 5);
		glVertex2f(closeX + m_CloseButtonSize - 5, closeY + 5);
		glVertex2f(closeX + 5, closeY + m_CloseButtonSize - 5);
		glEnd();

		for (auto *btn : m_Buttons)
		{
			btn->OnUpdate(m_RectX, m_RectY, m_IsVisible);
		}

		glDisable(GL_SCISSOR_TEST);

		glPopMatrix();
		glPopAttrib();
	}

	ResizeEdge uiWindow::GetResizeEdge(float mx, float my) const
	{
		const int edge = m_EdgeSize;
		const float left = m_RectX;
		const float right = m_RectX + m_RectWidth;
		const float top = m_RectY;
		const float bottom = m_RectY + m_RectHeight;

		bool onLeft = (mx >= left && mx <= left + edge);
		bool onRight = (mx >= right - edge && mx <= right);
		bool onTop = (my >= top && my <= top + edge);
		bool onBottom = (my >= bottom - edge && my <= bottom);

		if (onLeft && onTop)    return ResizeEdge::TopLeft;
		if (onLeft && onBottom) return ResizeEdge::BottomLeft;
		if (onRight && onTop)   return ResizeEdge::TopRight;
		if (onRight && onBottom)return ResizeEdge::BottomRight;

		const float yRangeLow = top - edge;
		const float yRangeHigh = bottom + edge;
		const float xRangeLow = left - edge;
		const float xRangeHigh = right + edge;

		if (onLeft && my >= yRangeLow && my <= yRangeHigh)
			return ResizeEdge::Left;
		if (onRight && my >= yRangeLow && my <= yRangeHigh)
			return ResizeEdge::Right;
		if (onTop && mx >= xRangeLow && mx <= xRangeHigh)
			return ResizeEdge::Top;
		if (onBottom && mx >= xRangeLow && mx <= xRangeHigh)
			return ResizeEdge::Bottom;

		return ResizeEdge::None;
	}

	bool uiWindow::OnEvent(Event& event)
	{
		if (!m_IsVisible) return false;

		for (auto it = m_Buttons.rbegin(); it != m_Buttons.rend(); ++it)
		{
			if ((*it)->OnEvent(event, m_RectX, m_RectY, m_IsVisible))
			{
				event.m_Handled = true;
				return true;
			}
		}

		OnDockEvent(event);

		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) -> bool
			{
				s_MainW = e.GetWidth(); s_MainH = e.GetHeight();
				if (s_LeftWidth > s_MainW - 200) s_LeftWidth = s_MainW - 200;
				if (s_RightWidth > s_MainW - 200) s_RightWidth = s_MainW - 200;
				UpdateDockLayout();
				return false;
			});
		dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& e) -> bool
			{
				float mx = e.GetMouseX();
				float my = e.GetMouseY();

				bool inTitleBar = (mx >= m_RectX && mx <= m_RectX + m_RectWidth && my >= m_RectY && my <= m_RectY + 30.0f);

				float closeX = m_RectX + m_RectWidth - m_CloseButtonSize - 10;
				float closeY = m_RectY + (30.0f - m_CloseButtonSize) / 2;

				bool inCloseButton = (mx >= closeX && mx <= closeX + m_CloseButtonSize && my >= closeY && my <= closeY + m_CloseButtonSize);

				if (inCloseButton)
				{
					FX_CORE_WARN("Close \"{}\" ui window!", m_name);
					m_IsVisible = false;
					if (m_IsDocked)
					{
						m_IsDocked = false;
						for (auto& pair : s_DockedWindows)
						{
							if (pair.second == this)
							{
								pair.second = nullptr;
								break;
							}
						}
						uiWindow::UpdateDockLayout();
					}
					else
					{
						auto it = std::find(s_FloatingWindows.begin(), s_FloatingWindows.end(), this);
						if (it != s_FloatingWindows.end())
							s_FloatingWindows.erase(it);
					}
					SetDocked(false);
					return true;
				}

				if (inTitleBar && e.GetMouseButton() == GLFW_MOUSE_BUTTON_LEFT)
				{
					if (m_IsFloating)
					{
						auto it = std::find(s_FloatingWindows.begin(), s_FloatingWindows.end(), this);
						if (it != s_FloatingWindows.end())
						{
							s_FloatingWindows.erase(it);
							s_FloatingWindows.push_back(this);
						}
						s_FocusedFloatingWindow = this;
					}
					StartDrag(mx, my);
					return true;
				}

				if (!m_IsDocked)
				{
					ResizeEdge edge = GetResizeEdge(mx, my);
					if (edge != ResizeEdge::None)
					{
						m_IsResizing = true;
						m_ResizeEdge = edge;
						m_ResizeStartX = mx;
						m_ResizeStartY = my;
						m_ResizeStartRectX = m_RectX;
						m_ResizeStartRectY = m_RectY;
						m_ResizeStartWidth = m_RectWidth;
						m_ResizeStartHeight = m_RectHeight;
						return true;
					}

					if (IsInResizeZone(mx, my) && e.GetMouseButton() == GLFW_MOUSE_BUTTON_LEFT)
					{
						m_IsResizing = true;
						m_ResizeStartX = mx;
						m_ResizeStartY = my;
						m_ResizeStartWidth = m_RectWidth;
						m_ResizeStartHeight = m_RectHeight;
						return true;
					}
				}

				return false;
			});

		dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent& e) -> bool
			{
				float mx = e.GetX(), my = e.GetY();

				float closeX = m_RectX + m_RectWidth - m_CloseButtonSize - 5;
				float closeY = m_RectY + (30.0f - m_CloseButtonSize) / 2;
				bool inClose = (mx >= closeX && mx <= closeX + m_CloseButtonSize &&
					my >= closeY && my <= closeY + m_CloseButtonSize);

				m_CloseButtonHovered = inClose;

				if (m_IsDraggingForDock)
				{
					OnDragMove(mx, my);
					return true;
				}

				if (m_IsDragging)
				{
					float dx = mx - m_DragStartX;
					float dy = my - m_DragStartY;
					m_RectX = m_WindowStartX + dx;
					m_RectY = m_WindowStartY + dy;
					return true;
				}

				bool inResizeZone = IsInResizeZone(mx, my);
				SetResizeCursor(inResizeZone);

				if (m_IsResizing)
				{
					float dx = mx - m_ResizeStartX;
					float dy = my - m_ResizeStartY;
					float newX = m_ResizeStartRectX;
					float newY = m_ResizeStartRectY;
					float newW = m_ResizeStartWidth;
					float newH = m_ResizeStartHeight;

					switch (m_ResizeEdge)
					{
					case ResizeEdge::Left:
						newW = m_ResizeStartWidth - dx;
						if (newW >= m_MinWidth)
						{
							newX = m_ResizeStartRectX + dx;
							m_RectWidth = newW;
							m_RectX = newX;
						}
						break;
					case ResizeEdge::Right:
						newW = m_ResizeStartWidth + dx;
						if (newW >= m_MinWidth)
							m_RectWidth = newW;
						break;
					case ResizeEdge::Top:
						newH = m_ResizeStartHeight - dy;
						if (newH >= m_MinHeight)
						{
							newY = m_ResizeStartRectY + dy;
							m_RectHeight = newH;
							m_RectY = newY;
						}
						break;
					case ResizeEdge::Bottom:
						newH = m_ResizeStartHeight + dy;
						if (newH >= m_MinHeight)
							m_RectHeight = newH;
						break;
					case ResizeEdge::TopLeft:
						newW = m_ResizeStartWidth - dx;
						newH = m_ResizeStartHeight - dy;
						if (newW >= m_MinWidth && newH >= m_MinHeight)
						{
							m_RectX = m_ResizeStartRectX + dx;
							m_RectY = m_ResizeStartRectY + dy;
							m_RectWidth = newW;
							m_RectHeight = newH;
						}
						break;
					case ResizeEdge::TopRight:
						newW = m_ResizeStartWidth + dx;
						newH = m_ResizeStartHeight - dy;
						if (newW >= m_MinWidth && newH >= m_MinHeight)
						{
							m_RectY = m_ResizeStartRectY + dy;
							m_RectWidth = newW;
							m_RectHeight = newH;
						}
						break;
					case ResizeEdge::BottomLeft:
						newW = m_ResizeStartWidth - dx;
						newH = m_ResizeStartHeight + dy;
						if (newW >= m_MinWidth && newH >= m_MinHeight)
						{
							m_RectX = m_ResizeStartRectX + dx;
							m_RectWidth = newW;
							m_RectHeight = newH;
						}
						break;
					case ResizeEdge::BottomRight:
						newW = m_ResizeStartWidth + dx;
						newH = m_ResizeStartHeight + dy;
						if (newW >= m_MinWidth && newH >= m_MinHeight)
						{
							m_RectWidth = newW;
							m_RectHeight = newH;
						}
						break;
					default:
						break;
					}
					return true;
				}

				ResizeEdge edge = GetResizeEdge(mx, my);
				UpdateResizeCursor(edge);

				return false;
			});

		dispatcher.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& e) -> bool
			{
				if (e.GetMouseButton() == GLFW_MOUSE_BUTTON_LEFT)
				{
					if (m_IsDraggingForDock)
					{
						EndDrag();
						return true;
					}
					if (m_IsDragging)
					{
						m_IsDragging = false;
						return true;
					}
					if (m_IsResizing)
					{
						m_IsResizing = false;
						UpdateResizeCursor(ResizeEdge::None);
						return true;
					}
				}
				return false;
			});

		return false;
	}

	void uiWindow::SetSize(float x, float y, float width, float height)
	{
		if (this == nullptr)
		{
			FX_CORE_ERROR("SetSize called on null uiWindow!");
			return;
		}
		m_RectX = x;
		m_RectY = y;
		m_RectWidth = width;
		m_RectHeight = height;
	}

	void uiWindow::DrawTitle() const
	{
		float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
		if (!ThemeManager::IsDarkTheme())
		{
			r = 0.0f;
			g = 0.0f;
			b = 0.0f;
		}
		TextRenderer::Get().DrawText(m_name, m_RectX + 12, m_RectY + 8, r, g, b, a);
	}

	bool uiWindow::IsInResizeZone(float mx, float my) const
	{
		float zoneX = m_RectX + m_RectWidth - 15.0f;
		float zoneY = m_RectY + m_RectHeight - 15.0f;

		return (mx >= zoneX && mx <= m_RectX + m_RectWidth &&
				my >= zoneY && my <= m_RectY + m_RectHeight);
	}

	void uiWindow::SetResizeCursor(bool isResizeZone)
	{
		GLFWwindow *glfwWindow = static_cast<GLFWwindow *>(Application::Get().GetWindow().GetNativeWindow());
		if (isResizeZone)
			glfwSetCursor(glfwWindow, glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR));
		else
			glfwSetCursor(glfwWindow, glfwCreateStandardCursor(GLFW_ARROW_CURSOR));
	}

	void uiWindow::UpdateResizeCursor(ResizeEdge edge)
	{
		GLFWwindow* glfwWin = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		GLFWcursor* cursor = nullptr;

		if (m_IsDocked)
		{
			glfwSetCursor(glfwWin, glfwCreateStandardCursor(GLFW_ARROW_CURSOR));
			return;
		}

		switch (edge)
		{
		case ResizeEdge::Left:
		case ResizeEdge::Right:
			cursor = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
			break;
		case ResizeEdge::Top:
		case ResizeEdge::Bottom:
			cursor = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
			break;
		case ResizeEdge::TopLeft:
		case ResizeEdge::BottomRight:
			cursor = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
			break;
		case ResizeEdge::TopRight:
		case ResizeEdge::BottomLeft:
			cursor = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
			break;
		default:
			cursor = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
			break;
		}
		glfwSetCursor(glfwWin, cursor);
	}

	void uiWindow::DrawResizeGrip() const
	{
		if (!m_IsVisible)
			return;

		float gripSize = 12.0f;
		float startX = m_RectX + m_RectWidth - gripSize;
		float startY = m_RectY + m_RectHeight - gripSize;

		glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
		glBegin(GL_TRIANGLES);
		glVertex2f(startX, m_RectY + m_RectHeight);
		glVertex2f(m_RectX + m_RectWidth, startY);
		glVertex2f(m_RectX + m_RectWidth, m_RectY + m_RectHeight);
		glEnd();
	}

	void uiWindow::AddButton(uiButton *button)
	{
		if (button)
		{
			m_Buttons.push_back(button);
		}
	}

	void uiWindow::InitDockSystem(float x, float y, float w, float h)
	{
		s_MainX = x; s_MainY = y; s_MainW = w; s_MainH = h;
		s_DockedWindows.clear();
		s_FloatingWindows.clear();
		UpdateDockLayout();
	}

	void uiWindow::UpdateDockLayout()
	{
		float spacing = s_DockSpacing;
		bool hasLeft = s_DockedWindows[DockRegion::Left] != nullptr;
		bool hasRight = s_DockedWindows[DockRegion::Right] != nullptr;
		bool hasBottom = s_DockedWindows[DockRegion::Bottom] != nullptr;
		bool hasCenter = s_DockedWindows[DockRegion::Center] != nullptr;

		float leftW = (s_DockedWindows[DockRegion::Left] != nullptr) ? s_LeftWidth : 0.0f;
		float rightW = (s_DockedWindows[DockRegion::Right] != nullptr) ? s_RightWidth : 0.0f;
		float bottomH = (s_DockedWindows[DockRegion::Bottom] != nullptr) ? s_BottomHeight : 0.0f;

		float leftOffset = (leftW > 0) ? spacing : 0;
		float rightOffset = (rightW > 0) ? spacing : 0;
		float bottomOffset = (bottomH > 0) ? spacing : 0;

		float maxTotalW = s_MainW * 0.8f;
		float maxTotalH = s_MainH * 0.8f;
		if (leftW + rightW > maxTotalW)
		{
			float ratio = maxTotalW / (leftW + rightW);
			leftW *= ratio;
			rightW *= ratio;
		}
		if (bottomH > maxTotalH) bottomH = maxTotalH;

		float centerX = s_MainX + leftW + (hasLeft ? spacing : 0);
		float centerY = s_MainY;
		float centerW = s_MainW - leftW - rightW - (hasLeft ? spacing : 0) - (hasRight ? spacing : 0);
		float centerH = s_MainH - bottomH - (hasBottom ? spacing : 0);

		if (s_DockedWindows[DockRegion::Left])
		{
			s_DockedWindows[DockRegion::Left]->SetSize(s_MainX, s_MainY, leftW, centerH);
		}
		if (s_DockedWindows[DockRegion::Right])
		{
			s_DockedWindows[DockRegion::Right]->SetSize(s_MainX + s_MainW - rightW, s_MainY, rightW, centerH);
		}
		if (s_DockedWindows[DockRegion::Bottom])
		{
			s_DockedWindows[DockRegion::Bottom]->SetSize(s_MainX, s_MainY + centerH + bottomOffset, s_MainW, bottomH);
		}
		if (s_DockedWindows[DockRegion::Center])
		{
			s_DockedWindows[DockRegion::Center]->SetSize(s_MainX + leftW + leftOffset, s_MainY, centerW, centerH);
		}

		s_CenterX = centerX;
		s_CenterY = centerY;
		s_CenterW = centerW;
		s_CenterH = centerH;

		s_DynamicLeftWidth = leftW;
		s_DynamicRightWidth = rightW;
		s_DynamicBottomHeight = bottomH;
		glfwPostEmptyEvent();
	}

	void uiWindow::DrawDockAreas()
	{
		for (auto& pair : s_DockedWindows)
		{
			if (pair.second && pair.second->IsVisible())
			{
				pair.second->DrawContent();
			}
		}

		for (auto* win : s_FloatingWindows)
		{
			if (win && win->IsVisible()) 
			{
				win->DrawContent();
			}
		}

		if (s_DraggingWindow && s_PreviewRegion != DockRegion::None)
		{
			Application& app = Application::Get();
			float x, y, w, h, leftW = app.GetWindow().GetWidth() * 0.2f, rightW = app.GetWindow().GetWidth() * 0.2f, bottomH = app.GetWindow().GetHeight() * 0.3f;
			
			switch (s_PreviewRegion)
			{
			case DockRegion::Left:
				x = s_MainX; y = s_MainY; w = leftW; h = s_MainH - bottomH;
				break;
			case DockRegion::Right:
				x = s_MainX + s_MainW - rightW; y = s_MainY; w = rightW; h = s_MainH - bottomH;
				break;
			case DockRegion::Bottom:
				x = s_MainX; y = s_MainY + s_MainH - bottomH; w = s_MainW; h = bottomH;
				break;
			case DockRegion::Center:
				x = s_MainX + leftW; y = s_MainY; w = s_MainW - leftW - rightW; h = s_MainH - bottomH;
				break;
			default: return;
			}
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColor4f(0.3f, 0.5f, 0.8f, 0.3f);
			glBegin(GL_QUADS);
			glVertex2f(x, y); glVertex2f(x + w, y);
			glVertex2f(x + w, y + h); glVertex2f(x, y + h);
			glEnd();
			glDisable(GL_BLEND);
		}
	}

	void uiWindow::DrawDockArea(DockRegion region)
	{
		auto* win = s_DockedWindows[region];
		if (!win || !win->IsVisible()) return;

		win->DrawContent();
	}

	bool uiWindow::HandleDockAreaEvent(DockArea& area, Event& event)
	{
		EventDispatcher dispatcher(event);
		bool handled = false;
		dispatcher.Dispatch<MouseButtonPressedEvent>([&](MouseButtonPressedEvent& e) 
			{
			float mx = e.GetMouseX(), my = e.GetMouseY();
			if (my >= area.y && my <= area.y + area.tabHeight)
			{
				float tabWidth = 120.0f;
				float tabX = area.x;
				for (size_t i = 0; i < area.windows.size(); ++i)
				{
					if (mx >= tabX && mx <= tabX + tabWidth)
					{
						float closeX = tabX + tabWidth - 20;
						float closeY = area.y + (area.tabHeight - 12) / 2;
						if (mx >= closeX && mx <= closeX + 12 && my >= closeY && my <= closeY + 12)
						{
							RemoveWindowFromDock(area.windows[i]);
							handled = true;
							return true;
						}
						else
						{
							area.activeWindow = area.windows[i];

							float contentY = area.y + area.tabHeight;
							float contentH = area.h - area.tabHeight;
							area.activeWindow->SetSize(area.x, contentY, area.w, contentH);
							handled = true;
							return true;
						}
					}
					tabX += tabWidth;
				}
			}
			return false;
			});
		if (!handled && area.activeWindow)
		{
			area.activeWindow->OnEvent(event);
		}
		return handled;
	}

	void uiWindow::DockWindowToRegion(uiWindow* window, DockRegion region)
	{
		auto& area = s_DockAreas[region];

		if (area.windows.empty())
		{
			area.activeWindow = window;
		}
		area.windows.push_back(window);
		window->SetDocked(true);

		float contentY = area.y + area.tabHeight;
		float contentH = area.h - area.tabHeight;
		window->SetSize(area.x, contentY, area.w, contentH);
	}

	void uiWindow::AddWindowToRegion(uiWindow* window, DockRegion region)
	{
		DockWindowToRegion(window, region);
	}

	void uiWindow::RemoveWindowFromDock(uiWindow* window)
	{
		for (auto& pair : s_DockAreas)
		{
			auto& area = pair.second;
			auto it = std::find(area.windows.begin(), area.windows.end(), window);
			if (it != area.windows.end())
			{
				area.windows.erase(it);
				if (area.activeWindow == window)
				{
					area.activeWindow = area.windows.empty() ? nullptr : area.windows[0];
					if (area.activeWindow)
					{
						float contentY = area.y + area.tabHeight;
						float contentH = area.h - area.tabHeight;
						area.activeWindow->SetSize(area.x, contentY, area.w, contentH);
					}
				}
				window->SetDocked(false);
				break;
			}
			if (pair.second.GetWindow() == window)
			{
				pair.second.SetActiveWindow(nullptr);
				break;
			}
		}

		for (auto& pair : s_DockAreas)
		{
			if (pair.second.GetWindow() == window)
			{
				pair.second.SetActiveWindow(nullptr);
				break;
			}
		}

		auto it = std::find(s_FloatingWindows.begin(), s_FloatingWindows.end(), window);
		if (it != s_FloatingWindows.end()) s_FloatingWindows.erase(it);
		window->SetDocked(false);
		window->SetFloating(true);
	}

	void uiWindow::StopDragging()
	{
		s_ShowDockPanel = false;
		s_HighlightedButton = DockRegion::None;

		if (!s_DraggingWindow) return;
		if (s_PreviewRegion != DockRegion::None)
		{
			DockWindowToRegion(s_DraggingWindow, s_PreviewRegion);
		}
		else
		{
			s_FloatingWindows.push_back(s_DraggingWindow);
			s_DraggingWindow->SetDocked(false);
		}
		s_DraggingWindow = nullptr;
		s_PreviewRegion = DockRegion::None;
	}

	void uiWindow::DockWindow(uiWindow* window, DockRegion region) 
	{
		if (s_DockedWindows[region])
		{
			UndockWindow(s_DockedWindows[region]);
		}
		s_DockedWindows[region] = window;
		window->SetDockRegion(region);
		window->SetDocked(true);
		window->SetFloating(false);
		window->m_AutoHide = false;
		UpdateDockLayout();
	}

	void uiWindow::UndockWindow(uiWindow* window)
	{
		for (auto& pair : s_DockedWindows)
		{
			if (pair.second == window)
			{
				pair.second = nullptr;
				break;
			}
		}
		window->SetDocked(false);
		window->SetFloating(true);
		s_FloatingWindows.push_back(window);
	}

	void uiWindow::ToggleAutoHide(uiWindow* window)
	{
		// When automatically hidden, the window shrinks to a side bar, and the mouse hovers to expand it
		// Leave blank here, expandable in the future
	}

	void uiWindow::StartDrag(float mouseX, float mouseY)
	{
		m_IsDraggingForDock = true;
		m_DragStartX = mouseX;
		m_DragStartY = mouseY;
		m_WindowStartX = m_RectX;
		m_WindowStartY = m_RectY;
		s_DraggingWindow = this;
		s_ShowDockPanel = true;
		s_HighlightedButton = DockRegion::None;

		if (m_IsDocked)
		{
			for (auto& pair : s_DockedWindows) 
			{
				if (pair.second == this)
				{
					pair.second = nullptr;
					break;
				}
			}
			m_IsDocked = false;
			m_IsFloating = true;
			s_FloatingWindows.push_back(this);
			UpdateDockLayout();
		}
	}

	void uiWindow::OnDragMove(float mouseX, float mouseY)
	{
		if (!m_IsDraggingForDock) return;
		float dx = mouseX - m_DragStartX;
		float dy = mouseY - m_DragStartY;
		SetSize(m_WindowStartX + dx, m_WindowStartY + dy, m_RectWidth, m_RectHeight);
	}

	void uiWindow::EndDrag()
	{
		if (!m_IsDraggingForDock) return;
		m_IsDraggingForDock = false;
		s_DraggingWindow = nullptr;

		float mx = m_RectX + m_RectWidth / 2;
		float my = m_RectY + m_RectHeight / 2;
		for (uiWindow* win : s_FloatingWindows)
		{
			if (win == this) continue;
			if (mx >= win->m_RectX && mx <= win->m_RectX + win->m_RectWidth && my >= win->m_RectY && my <= win->m_RectY + 30)
			{
				break;
			}
		}
	}

	DockRegion uiWindow::DetectDockTarget(float mx, float my)
	{
		const int edge = 50;
		if (mx < edge) return DockRegion::Left;
		if (mx > s_MainW - edge) return DockRegion::Right;
		if (my > s_MainH - edge) return DockRegion::Bottom;
		return DockRegion::Center;
	}

	bool uiWindow::OnDockEvent(Event& event)
	{
		if (event.GetEventType() == EventType::MouseMoved)
		{
			MouseMovedEvent& e = (MouseMovedEvent&)event;
			float mx = e.GetX(), my = e.GetY();
			if (s_ShowDockPanel)
			{
				DockRegion btn = GetButtonAt(mx, my);
				if (btn != DockRegion::None)
				{
					s_PreviewRegion = btn;
				}
				else
				{
					s_PreviewRegion = DockRegion::None;
				}
				return true;
			}
			if (s_DraggingWindow)
			{
				s_PreviewRegion = DockRegion::None;
			}
			return true;
		}
		else if (event.GetEventType() == EventType::MouseButtonReleased)
		{
			if (s_DraggingWindow)
			{
				if (s_PreviewRegion != DockRegion::None)
				{
					DockWindow(s_DraggingWindow, s_PreviewRegion);
				}
				else 
				{
					UndockWindow(s_DraggingWindow);
				}
				s_DraggingWindow = nullptr;
				s_PreviewRegion = DockRegion::None;
				MouseButtonEvent& t_event = (MouseButtonReleasedEvent&)event;
				int action = (event.GetEventType() == EventType::MouseButtonPressed) ? GLFW_PRESS : GLFW_RELEASE;
				StopDragging();
				return true;
			}
		}
		return false;
	}

	void uiWindow::DrawDockPanel(float mx, float my)
	{
		if (!s_ShowDockPanel) return;

		Application& app = Application::Get();
		int width = (int)app.GetWindow().GetWidth();
		int height = (int)app.GetWindow().GetHeight();

		glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
		glPushMatrix();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, width, height, 0, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		float panelW = 300.0f, panelH = 200.0f;
		float panelX = (s_MainW - panelW) / 2;
		float panelY = (s_MainH - panelH) / 2;

		float btnW = 50.0f, btnH = 30.0f;
		float gap = 10.0f;
		float startX = panelX + (panelW - 3 * (btnW + gap) + gap) / 2;
		float startY = panelY + (panelH - 3 * (btnH + gap) + gap) / 2;

		DockRegion regions[4] =
		{
			DockRegion::Left,
			DockRegion::Center,
			DockRegion::Right,
			DockRegion::Bottom
		};

		std::string labels[4] = { "L", "C", "R", "B" };
		for (int i = 0; i < 3; ++i)
		{
			float x = startX + i * (btnW + gap);
			float y = startY;
			bool hovered = (mx >= x && mx <= x + btnW && my >= y && my <= y + btnH);
			if (hovered)
			{
				glColor4f(0.4f, 0.6f, 0.9f, 1.0f);
				s_HighlightedButton = regions[i];
			}
			else
			{
				glColor4f(0.3f, 0.3f, 0.35f, 1.0f);
			}
			glBegin(GL_QUADS);
			glVertex2f(x, y);
			glVertex2f(x + btnW, y);
			glVertex2f(x + btnW, y + btnH);
			glVertex2f(x, y + btnH);
			glEnd();

			float textW = TextRenderer::Get().GetTextWidth(labels[i]);
			float textH = TextRenderer::Get().GetTextHeight();
			float textX = x + (btnW - textW) / 2;
			float textY = y + (btnH - textH) / 2;
			TextRenderer::Get().DrawText(labels[i], textX, textY, 1.0f, 1.0f, 1.0f, 0.9f);
		}

		float x = startX + 1 * (btnW + gap);
		float y = startY + btnH + gap;
		bool hovered = (mx >= x && mx <= x + btnW && my >= y && my <= y + btnH);
		if (hovered)
		{
			glColor4f(0.4f, 0.6f, 0.9f, 1.0f);
			s_HighlightedButton = DockRegion::Bottom;
		}
		else
		{
			glColor4f(0.3f, 0.3f, 0.35f, 1.0f);
		}

		glBegin(GL_QUADS);
		glVertex2f(x, y);
		glVertex2f(x + btnW, y);
		glVertex2f(x + btnW, y + btnH);
		glVertex2f(x, y + btnH);
		glEnd();

		float textW = TextRenderer::Get().GetTextWidth(labels[3]);
		float textH = TextRenderer::Get().GetTextHeight();
		float textX = x + (btnW - textW) / 2;
		float textY = y + (btnH - textH) / 2;
		TextRenderer::Get().DrawText(labels[3], textX, textY, 1.0f, 1.0f, 1.0f, 0.9f);
	}

	DockRegion uiWindow::GetButtonAt(float mx, float my)
	{
		float panelW = 300.0f, panelH = 300.0f;
		float panelX = (s_MainW - panelW) / 2;
		float panelY = (s_MainH - panelH) / 2;
		float btnW = 50.0f, btnH = 30.0f;
		float gap = 10.0f;
		float startX = panelX + (panelW - 3 * (btnW + gap) + gap) / 2;
		float startY = panelY + (panelH - 3 * (btnH + gap) + gap) / 2;
		DockRegion regions[4] = 
		{
			DockRegion::Left,
			DockRegion::Center,
			DockRegion::Right,
			DockRegion::Bottom
		};

		for (int i = 0; i < 3; ++i)
		{
			float x = startX + i * (btnW + gap);
			float y = startY;
			if (mx >= x && mx <= x + btnW && my >= y && my <= y + btnH)
			{
				return regions[i];
			}
		}

		float x = startX + btnW + gap;
		float y = startY + btnH + gap;
		if (mx >= x && mx <= x + btnW && my >= y && my <= y + btnH)
		{
			return regions[3];
		}
		return DockRegion::None;
	}

}
