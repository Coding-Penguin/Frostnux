#pragma once
#include <fxpch.h>
#include "Frostnux/Events/Event.h"
#include "Frostnux/Events/MouseEvent.h"
#include "PhotoRenderer.h"

namespace Frostnux {

	enum class ButtonStyles
	{
		Normal = 0,
		NoBackground,
		NoLine,
		NoBackgroundOrLine
	};

	class uiButton
	{
	public:
		uiButton(const std::string& text, float x, float y, float width, float height, ButtonStyles style = ButtonStyles::Normal);
		~uiButton();

		void OnUpdate(float parentX, float parentY, bool parentVisible);
		bool OnEvent(Event& event, float parentX, float parentY, bool parentVisible);

		void SetCallback(std::function<void()> callback) { m_Callback = callback; }
		void SetText(const std::string& text) { m_Text = text; }
		void SetColor(float r, float g, float b, float a) { m_Color[0] = r; m_Color[1] = g; m_Color[2] = b; m_Color[3] = a; m_UseThemeColor = false; }

		void SetPosition(float x, float y) { m_X = x; m_Y = y; }

		void SetTooltip(const std::string& tooltip) { m_Tooltip = tooltip; }
		void OnTooltipUpdate(float deltaTime);
		void DrawTooltip(float parentX, float parentY) const;

		float GetX() const { return m_X; }
		float GetY() const { return m_Y; }
		float GetWidth() const { return m_Width; }
		float GetHeight() const { return m_Height; }

		std::string GetText() const { return m_Text; }
	private:
		std::string m_Text;
		std::string m_Tooltip;
		float m_HoverTimer = 0.0f;

		bool m_ShowTooltip = false;
		float m_X, m_Y, m_Width, m_Height;
		float m_Color[4];
		bool m_Hovered = false;
		std::function<void()> m_Callback;
		ButtonStyles m_ButtonStyle;

		bool m_UseThemeColor = true;

		void Draw(float parentX, float parentY) const;
		bool IsPointInside(float px, float py, float parentX, float parentY) const;
	};

}