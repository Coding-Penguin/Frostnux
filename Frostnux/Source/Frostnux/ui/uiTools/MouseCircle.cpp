#include <fxpch.h>
#include "MouseCircle.h"
#include "Frostnux/Events/MouseEvent.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Frostnux/Renderer/Renderer2D.h"

namespace Frostnux
{

	MouseCircle::MouseCircle()
	{
	}
	MouseCircle::~MouseCircle()
	{
	}

	MouseCircle& MouseCircle::Get()
	{
		static MouseCircle instance;
		return instance;
	}

	void MouseCircle::OnUpdate(float deltaTime)
	{
		(void)deltaTime;
		Renderer2D::DrawCircle(mx, my, m_Radius, 0.5f, 0.5f, 0.5f, 0.005f);
	}

	bool MouseCircle::OnEvent(Event& event)
	{
		if (event.GetEventType() == EventType::MouseMoved)
		{
			MouseMovedEvent& e = (MouseMovedEvent&)event;
			mx = e.GetX();
			my = e.GetY();
			return false;
		}
		return false;
	}

	bool MouseCircle::IsIntersectingRect(float x, float y, float w, float h) const
	{
		float closestX = std::max(x, std::min(mx, x + w));
		float closestY = std::max(y, std::min(my, y + h));
		float dx = mx - closestX;
		float dy = my - closestY;
		return (dx * dx + dy * dy) <= (m_Radius * m_Radius);
	}

}