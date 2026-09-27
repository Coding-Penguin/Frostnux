#pragma once
#include <glad/glad.h>
#include "Frostnux/FontManager.h"
#include "CodeEditor.h"
#include <string>

namespace Frostnux {

	class GLRenderer : public Renderer
	{
	public:
		GLRenderer() = default;

		void SetFontName(const std::string& name) { m_FontName = name; }

		std::string GetFont() const
		{
			return m_FontName;
		}

		void drawText(std::u32string_view text, float x, float y, float /*scale*/, Color c, FontStyle style = FontStyle::Regular) override
		{
			if (text.empty()) return;
			
			auto& fm = FontManager::Get();
			TextRenderer* font = fm.GetFont(m_FontName, style);
			if (!font) font = fm.GetFont(m_FontName, FontStyle::Regular);
			if (!font) return;
			font->DrawTextUTF32(text, x, y, c.r, c.g, c.b, c.a);
		}

		void drawRect(float x, float y, float w, float h, Color c) override
		{
			glDisable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColor4f(c.r, c.g, c.b, c.a);

			glBegin(GL_QUADS);
			glVertex2f(x, y);
			glVertex2f(x + w, y);
			glVertex2f(x + w, y + h);
			glVertex2f(x, y + h);
			glEnd();
		}

		[[nodiscard]] float measureText(std::u32string_view text, float) const override
		{
			TextRenderer* font = FontManager::Get().GetFont(m_FontName, FontStyle::Regular);
			if (!font) return 0.0f;
			return font->GetTextWidthUTF32(text);
		}

		[[nodiscard]] float lineHeight(float) const override
		{
			TextRenderer* font = FontManager::Get().GetFont(m_FontName, FontStyle::Regular);
			if (!font) return 20.0f;
			return font->GetLineHeight();
		}

	private:
		std::string m_FontName = "code";
	};

}
