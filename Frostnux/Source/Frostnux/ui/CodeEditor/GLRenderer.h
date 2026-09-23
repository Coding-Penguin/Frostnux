#pragma once
#include <glad/glad.h>
#include "../uiTools/TextRenderer.h"
#include "CodeEditor.h"

namespace Frostnux {

	class GLRenderer : public Renderer
	{
	public:
		void drawText(std::u32string_view text, float x, float y, float /*scale*/, Color c) override
		{
			if (text.empty()) return;
			TextRenderer::Get().DrawTextUTF32(text, x, y, c.r, c.g, c.b, c.a);
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
			return TextRenderer::Get().GetTextWidthUTF32(text);
		}

		[[nodiscard]] float lineHeight(float) const override
		{
			return TextRenderer::Get().GetLineHeight();
		}
	};

}
