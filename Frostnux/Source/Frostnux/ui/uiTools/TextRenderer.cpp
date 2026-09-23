#include <fxpch.h>
#include "TextRenderer.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Frostnux {

	unsigned int DecodeUTF8(const std::string& text, size_t& index)
	{
		if (index >= text.size()) return 0;

		const unsigned char c0 = static_cast<unsigned char>(text[index]);
		if (c0 < 0x80u)
		{
			++index;
			return c0;
		}

		unsigned int cp = 0;
		int extra = 0;
		if ((c0 & 0xE0u) == 0xC0u)
		{
			cp = c0 & 0x1Fu;
			extra = 1;
		}
		else if ((c0 & 0xF0u) == 0xE0u)
		{
			cp = c0 & 0x0Fu;
			extra = 2;
		}
		else if ((c0 & 0xF8u) == 0xF0u)
		{
			cp = c0 & 0x07u;
			extra = 3;
		}
		else
		{
			++index;
			return 0xFFFDu;
		}

		++index;
		for (int k = 0; k < extra; ++k)
		{
			if (index >= text.size()) return 0xFFFDu;
			const unsigned char cc = static_cast<unsigned char>(text[index]);
			if ((cc & 0xC0u) != 0x80u) return 0xFFFDu;
			cp = (cp << 6) | (cc & 0x3Fu);
			++index;
		}
		return cp;
	}

	TextRenderer& TextRenderer::Get()
	{
		static TextRenderer instance;
		return instance;
	}

	TextRenderer::~TextRenderer()
	{
		if (m_TextureID) glDeleteTextures(1, &m_TextureID);
	}

	bool TextRenderer::LoadFont(const std::string& fontPath, float fontSize)
	{
		if (m_Initialized) Unload();

		std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
		if (!file)
		{
			FX_CORE_ERROR("Failed to open font file: {}", fontPath);
			return false;
		}

		const std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		m_FontBuffer.resize(static_cast<size_t>(size));
		if (!file.read(reinterpret_cast<char*>(m_FontBuffer.data()), size))
		{
			FX_CORE_ERROR("Failed to read font file: {}", fontPath);
			m_FontBuffer.clear();
			return false;
		}
		file.close();

		if (!stbtt_InitFont(&m_FontInfo, m_FontBuffer.data(), 0))
		{
			FX_CORE_ERROR("Failed to init font: {}", fontPath);
			m_FontBuffer.clear();
			return false;
		}

		m_Scale = stbtt_ScaleForPixelHeight(&m_FontInfo, fontSize);
		stbtt_GetFontVMetrics(&m_FontInfo, &m_Ascent, &m_Descent, &m_LineGap);

		s_FontSize = fontSize;
		m_FontSize = fontSize;
		m_CharWidth = fontSize * 0.5f;

		m_AtlasWidth = 2048;
		m_AtlasHeight = 2048;
		m_AtlasX = 1;
		m_AtlasY = 1;
		m_AtlasRowHeight = 0;

		glGenTextures(1, &m_TextureID);
		glBindTexture(GL_TEXTURE_2D, m_TextureID);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		std::vector<unsigned char> zeroAtlas(static_cast<size_t>(m_AtlasWidth) * static_cast<size_t>(m_AtlasHeight), 0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, m_AtlasWidth, m_AtlasHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, zeroAtlas.data());

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glBindTexture(GL_TEXTURE_2D, 0);

		m_Chars.clear();
		m_Chars.reserve(512);
		m_Initialized = true;

		FX_CORE_INFO("Font loaded: {}", fontPath);
		return true;
	}

	const TextRenderer::CharInfo* TextRenderer::BakeGlyph(unsigned int codepoint)
	{
		if (!m_Initialized || m_TextureID == 0) return nullptr;

		if (stbtt_FindGlyphIndex(&m_FontInfo, codepoint) == 0)
			return nullptr;

		int advance = 0, lsb = 0;
		stbtt_GetCodepointHMetrics(&m_FontInfo, codepoint, &advance, &lsb);

		int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
		stbtt_GetCodepointBitmapBox(&m_FontInfo, codepoint, m_Scale, m_Scale, &x0, &y0, &x1, &y1);

		const int w = x1 - x0;
		const int h = y1 - y0;

		CharInfo ci{};
		ci.advance = static_cast<float>(advance) * m_Scale;
		ci.xoff = static_cast<float>(x0);
		ci.yoff = static_cast<float>(y0) + m_FontSize * 0.64f;
		ci.width = static_cast<float>(w);
		ci.height = static_cast<float>(h);
		ci.x0 = ci.y0 = ci.x1 = ci.y1 = 0.0f;

		if (w > 0 && h > 0)
		{
			const int pad = 1;

			if (m_AtlasX + w + pad > m_AtlasWidth)
			{
				m_AtlasX = pad;
				m_AtlasY += m_AtlasRowHeight + pad;
				m_AtlasRowHeight = 0;
			}

			if (m_AtlasY + h + pad > m_AtlasHeight)
			{
				FX_CORE_ERROR("Font atlas is full, cannot bake U+{:04X}", codepoint);
				return nullptr;
			}

			std::vector<unsigned char> bitmap(static_cast<size_t>(w) * static_cast<size_t>(h));
			stbtt_MakeCodepointBitmap(&m_FontInfo, bitmap.data(), w, h, w, m_Scale, m_Scale, codepoint);

			glBindTexture(GL_TEXTURE_2D, m_TextureID);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexSubImage2D(GL_TEXTURE_2D, 0, m_AtlasX, m_AtlasY, w, h, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap.data());
			glBindTexture(GL_TEXTURE_2D, 0);

			ci.x0 = static_cast<float>(m_AtlasX) / static_cast<float>(m_AtlasWidth);
			ci.y0 = static_cast<float>(m_AtlasY) / static_cast<float>(m_AtlasHeight);
			ci.x1 = static_cast<float>(m_AtlasX + w) / static_cast<float>(m_AtlasWidth);
			ci.y1 = static_cast<float>(m_AtlasY + h) / static_cast<float>(m_AtlasHeight);

			m_AtlasX += w + pad;
			if (h > m_AtlasRowHeight) m_AtlasRowHeight = h;
		}

		auto res = m_Chars.emplace(codepoint, ci);
		return &res.first->second;
	}

	const TextRenderer::CharInfo* TextRenderer::GetOrCreateGlyph(unsigned int codepoint)
	{
		auto it = m_Chars.find(codepoint);
		if (it != m_Chars.end()) return &it->second;
		return BakeGlyph(codepoint);
	}

	void TextRenderer::DrawText(const std::string& text, float x, float y,
		float r, float g, float b, float a)
	{
		if (!m_Initialized || m_TextureID == 0) return;
		if (text.empty()) return;

		{
			size_t i = 0;
			while (i < text.size())
			{
				const unsigned int cp = DecodeUTF8(text, i);
				if (cp == 0) break;
				if (cp == '\t' || cp == '\n' || cp == '\r') continue;
				GetOrCreateGlyph(cp);
			}
		}

		glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT);
		glPushMatrix();

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glBindTexture(GL_TEXTURE_2D, m_TextureID);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor4f(r, g, b, a);

		float curX = x;
		float curY = y;

		glBegin(GL_QUADS);
		{
			size_t i = 0;
			while (i < text.size())
			{
				const unsigned int cp = DecodeUTF8(text, i);
				if (cp == 0) break;

				if (cp == '\t') { curX += GetCharWidth('\t'); continue; }
				if (cp == '\r') continue;
				if (cp == '\n') { curX = x; curY += GetLineHeight(); continue; }

				auto it = m_Chars.find(cp);
				if (it == m_Chars.end()) continue;
				const CharInfo& ci = it->second;

				if (ci.width > 0.0f && ci.height > 0.0f)
				{
					const float xpos = curX + ci.xoff;
					const float ypos = curY + ci.yoff;

					glTexCoord2f(ci.x0, ci.y0); glVertex2f(xpos, ypos);
					glTexCoord2f(ci.x1, ci.y0); glVertex2f(xpos + ci.width, ypos);
					glTexCoord2f(ci.x1, ci.y1); glVertex2f(xpos + ci.width, ypos + ci.height);
					glTexCoord2f(ci.x0, ci.y1); glVertex2f(xpos, ypos + ci.height);
				}

				curX += ci.advance;
			}
		}
		glEnd();

		glPopMatrix();
		glPopAttrib();
	}

	float TextRenderer::GetTextWidth(const std::string& text) const
	{
		if (this == nullptr) return 0.0f;
		if (!m_Initialized)
		{
			FX_CORE_ERROR("TextRenderer not initialized or no atlas!");
			return 0.0f;
		}

		float width = 0.0f;
		size_t i = 0;
		while (i < text.size())
		{
			const unsigned int cp = DecodeUTF8(text, i);
			if (cp == 0) break;
			width += GetAdvance(cp);
		}
		return width;
	}

	float TextRenderer::GetTextHeight() const
	{
		return s_FontSize;
	}

	float TextRenderer::GetFontSize()
	{
		return s_FontSize;
	}

	float TextRenderer::GetAdvance(unsigned int codepoint) const
	{
		if (!m_Initialized) return 0.0f;
		if (codepoint == '\t') return static_cast<float>(m_TabWidth) * m_CharWidth;
		if (codepoint == '\n' || codepoint == '\r') return 0.0f;
		if (stbtt_FindGlyphIndex(&m_FontInfo, codepoint) == 0) return 0.0f;

		int advance = 0, lsb = 0;
		stbtt_GetCodepointHMetrics(&m_FontInfo, codepoint, &advance, &lsb);
		return static_cast<float>(advance) * m_Scale;
	}

	float TextRenderer::GetLineHeight() const
	{
		if (!m_Initialized) return s_FontSize;
		return static_cast<float>(m_Ascent - m_Descent + m_LineGap) * m_Scale;
	}

	float TextRenderer::GetCharWidth(char c) const
	{
		if (c == '\t') return static_cast<float>(m_TabWidth) * m_CharWidth;
		if (!m_Initialized) return s_FontSize * 0.5f;

		const unsigned int cp = static_cast<unsigned char>(c);
		const float adv = GetAdvance(cp);
		if (adv > 0.0f) return adv;
		return s_FontSize * 0.5f;
	}

	void TextRenderer::Unload()
	{
		if (m_TextureID)
		{
			glDeleteTextures(1, &m_TextureID);
			m_TextureID = 0;
		}

		m_Chars.clear();
		m_FontBuffer.clear();
		std::memset(&m_FontInfo, 0, sizeof(m_FontInfo));

		m_AtlasX = 1;
		m_AtlasY = 1;
		m_AtlasRowHeight = 0;
		m_AtlasWidth = m_AtlasHeight = 0;
		m_Scale = 0.0f;
		m_Ascent = m_Descent = m_LineGap = 0;

		m_Initialized = false;
	}
	
	float TextRenderer::GetTextWidthUTF32(std::u32string_view text) const
	{
		if (!m_Initialized) return 0.0f;
		float width = 0.0f;
		for (char32_t cp : text)
			width += GetAdvance(static_cast<unsigned int>(cp));
		return width;
	}

	void TextRenderer::DrawTextUTF32(std::u32string_view text, float x, float y,
		float r, float g, float b, float a)
	{
		if (!m_Initialized || m_TextureID == 0) return;
		if (text.empty()) return;

		for (char32_t cp : text)
		{
			if (cp == U'\t' || cp == U'\n' || cp == U'\r') continue;
			GetOrCreateGlyph(static_cast<unsigned int>(cp));
		}

		glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT);
		glPushMatrix();

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBindTexture(GL_TEXTURE_2D, m_TextureID);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor4f(r, g, b, a);

		float curX = x;
		float curY = y;

		glBegin(GL_QUADS);
		for (char32_t cp : text)
		{
			if (cp == U'\t')
			{
				curX += GetCharWidth('\t');
				continue;
			}
			if (cp == U'\r')
			{
				continue;
			}
			if (cp == U'\n')
			{
				curX = x;
				curY += GetLineHeight(); continue;
			}

			auto it = m_Chars.find(static_cast<unsigned int>(cp));
			if (it == m_Chars.end()) continue;
			const CharInfo& ci = it->second;

			if (ci.width > 0.0f && ci.height > 0.0f)
			{
				const float xpos = curX + ci.xoff;
				const float ypos = curY + ci.yoff;

				glTexCoord2f(ci.x0, ci.y0); glVertex2f(xpos, ypos);
				glTexCoord2f(ci.x1, ci.y0); glVertex2f(xpos + ci.width, ypos);
				glTexCoord2f(ci.x1, ci.y1); glVertex2f(xpos + ci.width, ypos + ci.height);
				glTexCoord2f(ci.x0, ci.y1); glVertex2f(xpos, ypos + ci.height);
			}

			curX += ci.advance;
		}
		glEnd();

		glPopMatrix();
		glPopAttrib();
	}

}
