#pragma once
#include <glad/glad.h>
#include "Frostnux/Managers/FontManager.h"
#include "CodeEditor.h"
#include "Frostnux/Renderer/Renderer2D.h"
#include <string>

namespace Frostnux
{

	class GLRenderer : public Renderer
	{
	public:
		GLRenderer() = default;
		void SetFontName(const std::string &name) { m_FontName = name; }
		std::string GetFont() const { return m_FontName; }

		void drawText(std::u32string_view text, float x, float y, float /*scale*/, Color c, FontStyle style = FontStyle::Regular) override
		{
			if (text.empty())
				return;
			auto &fm = FontManager::Get();
			TextRenderer *font = fm.GetFont(m_FontName, style);
			if (!font)
				font = fm.GetFont(m_FontName, FontStyle::Regular);
			if (!font)
				return;
			font->DrawTextUTF32(text, x, y, c.r, c.g, c.b, c.a);
		}

		void drawRect(float x, float y, float w, float h, Color c) override
		{
			Renderer2D::DrawQuad(x, y, w, h, c.r, c.g, c.b, c.a);
		}

		void drawLine(float x1, float y1, float x2, float y2, float thickness, Color c) override
		{
			Renderer2D::DrawLine(x1, y1, x2, y2, thickness, c.r, c.g, c.b, c.a);
		}

		[[nodiscard]] float measureText(std::u32string_view text, float) const override
		{
			TextRenderer *font = FontManager::Get().GetFont(m_FontName, FontStyle::Regular);
			return font ? font->GetTextWidthUTF32(text) : 0.0f;
		}

		[[nodiscard]] float lineHeight(float) const override
		{
			TextRenderer *font = FontManager::Get().GetFont(m_FontName, FontStyle::Regular);
			return font ? font->GetLineHeight() : 20.0f;
		}

	private:
		std::string m_FontName = "code";
	};
}
