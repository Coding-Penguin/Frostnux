#include <fxpch.h>
#include "TextRenderer.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Frostnux/Core/Log.h"
#include <fstream>
#include <vector>
#include <cstring>

namespace Frostnux {

	static unsigned int DecodeUTF8(const std::string& text, size_t& index)
	{
		if (index >= text.size()) return 0;

		const unsigned char c0 = static_cast<unsigned char>(text[index]);
		if (c0 < 0x80u) { ++index; return c0; }

		unsigned int cp = 0;
		int extra = 0;
		if ((c0 & 0xE0u) == 0xC0u) { cp = c0 & 0x1Fu; extra = 1; }
		else if ((c0 & 0xF0u) == 0xE0u) { cp = c0 & 0x0Fu; extra = 2; }
		else if ((c0 & 0xF8u) == 0xF0u) { cp = c0 & 0x07u; extra = 3; }
		else { ++index; return 0xFFFDu; }

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
		Unload();
	}

	bool TextRenderer::LoadFace(FontFace& face, const std::string& path, float fontSize)
	{
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file)
		{
			FX_CORE_ERROR("Failed to open font file: {}", path);
			return false;
		}

		const std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		face.buffer.resize(static_cast<size_t>(size));
		if (!file.read(reinterpret_cast<char*>(face.buffer.data()), size))
		{
			face.buffer.clear();
			return false;
		}
		file.close();

		if (!stbtt_InitFont(&face.info, face.buffer.data(), 0))
		{
			FX_CORE_ERROR("Failed to init font: {}", path);
			face.buffer.clear();
			return false;
		}

		face.scale = stbtt_ScaleForPixelHeight(&face.info, fontSize);
		stbtt_GetFontVMetrics(&face.info, &face.ascent, &face.descent, &face.lineGap);

		CreateAtlas(face, 16384, 16384);
		return true;
	}

	void TextRenderer::CreateAtlas(FontFace& face, int w, int h)
	{
		face.atlasW = w;
		face.atlasH = h;
		face.atlasX = 1;
		face.atlasY = 1;
		face.atlasRowHeight = 0;

		glGenTextures(1, &face.textureID);
		glBindTexture(GL_TEXTURE_2D, face.textureID);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		std::vector<unsigned char> zero(static_cast<size_t>(w) * h, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, zero.data());

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	bool TextRenderer::LoadFont(const std::string& fontPath, float fontSize)
	{
		Unload();

		m_FontSize = fontSize;
		m_CharWidth = fontSize * 0.5f;

		FontFace face;
		if (!LoadFace(face, fontPath, fontSize))
			return false;

		m_Faces.push_back(std::move(face));
		m_Initialized = true;

		FX_CORE_INFO("Font loaded: {}", fontPath);
		return true;
	}

	bool TextRenderer::AddFallback(const std::string& fontPath)
	{
		if (!m_Initialized)
		{
			FX_CORE_ERROR("AddFallback called before LoadFont");
			return false;
		}

		FontFace face;
		if (!LoadFace(face, fontPath, m_FontSize))
			return false;

		m_Faces.push_back(std::move(face));
		FX_CORE_INFO("Fallback font loaded: {}", fontPath);
		return true;
	}

	std::optional<TextRenderer::CharInfo> TextRenderer::BakeGlyphToFace(FontFace& face, unsigned int cp) const
	{
		if (stbtt_FindGlyphIndex(&face.info, cp) == 0)
			return std::nullopt;

		int advance = 0, lsb = 0;
		stbtt_GetCodepointHMetrics(&face.info, cp, &advance, &lsb);

		int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
		stbtt_GetCodepointBitmapBox(&face.info, cp, face.scale, face.scale,
			&x0, &y0, &x1, &y1);

		const int w = x1 - x0;
		const int h = y1 - y0;

		CharInfo ci{};
		ci.advance = static_cast<float>(advance) * face.scale;
		ci.xoff = static_cast<float>(x0);
		ci.yoff = static_cast<float>(y0) + m_FontSize * 0.64f;
		ci.width = static_cast<float>(w);
		ci.height = static_cast<float>(h);
		ci.x0 = ci.y0 = ci.x1 = ci.y1 = 0.0f;

		if (w > 0 && h > 0)
		{
			const int pad = 1;

			if (face.atlasX + w + pad > face.atlasW)
			{
				face.atlasX = pad;
				face.atlasY += face.atlasRowHeight + pad;
				face.atlasRowHeight = 0;
			}

			if (face.atlasY + h + pad > face.atlasH)
			{
				FX_CORE_ERROR("Font atlas full, cannot bake U+{:04X}", cp);
				return std::nullopt;
			}

			std::vector<unsigned char> bitmap(static_cast<size_t>(w) * static_cast<size_t>(h));
			stbtt_MakeCodepointBitmap(&face.info, bitmap.data(), w, h, w, face.scale, face.scale, cp);

			glBindTexture(GL_TEXTURE_2D, face.textureID);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexSubImage2D(GL_TEXTURE_2D, 0, face.atlasX, face.atlasY, w, h, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap.data());
			glBindTexture(GL_TEXTURE_2D, 0);

			ci.x0 = static_cast<float>(face.atlasX) / face.atlasW;
			ci.y0 = static_cast<float>(face.atlasY) / face.atlasH;
			ci.x1 = static_cast<float>(face.atlasX + w) / face.atlasW;
			ci.y1 = static_cast<float>(face.atlasY + h) / face.atlasH;

			face.atlasX += w + pad;
			if (h > face.atlasRowHeight) face.atlasRowHeight = h;
		}

		return ci;
	}

	const TextRenderer::CharInfo* TextRenderer::GetOrCreateGlyph(unsigned int cp, int& outFaceIdx)
	{
		auto it = m_GlyphCache.find(cp);
		if (it != m_GlyphCache.end())
		{
			outFaceIdx = it->second.faceIdx;
			return &it->second.info;
		}

		for (int i = 0; i < static_cast<int>(m_Faces.size()); ++i)
		{
			auto ci = BakeGlyphToFace(m_Faces[i], cp);
			if (!ci.has_value()) continue;

			CachedGlyph cg;
			cg.faceIdx = i;
			cg.info = *ci;

			auto res = m_GlyphCache.emplace(cp, std::move(cg));
			outFaceIdx = i;
			return &res.first->second.info;
		}

		return nullptr;
	}

	void TextRenderer::DrawTextUTF32(std::u32string_view text, float x, float y,
		float r, float g, float b, float a)
	{
		if (!m_Initialized || m_Faces.empty()) return;
		if (text.empty()) return;

		for (char32_t cp : text)
		{
			if (cp == U'\t' || cp == U'\n' || cp == U'\r') continue;
			int fi;
			GetOrCreateGlyph(static_cast<unsigned int>(cp), fi);
		}

		glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT);
		glPushMatrix();

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor4f(r, g, b, a);

		float curX = x;
		float curY = y;

		int curFace = -1;
		bool quadOpen = false;

		auto flush = [&]()
			{
				if (quadOpen) { glEnd(); quadOpen = false; }
			};

		for (char32_t cp : text)
		{
			if (cp == U'\t') { curX += GetCharWidth('\t'); continue; }
			if (cp == U'\r') continue;
			if (cp == U'\n') { curX = x; curY += GetLineHeight(); continue; }

			int fi = -1;
			const CharInfo* ci = GetOrCreateGlyph(static_cast<unsigned int>(cp), fi);
			if (!ci) continue;

			if (fi != curFace)
			{
				flush();
				glBindTexture(GL_TEXTURE_2D, m_Faces[fi].textureID);
				curFace = fi;
			}

			if (!quadOpen) { glBegin(GL_QUADS); quadOpen = true; }

			if (ci->width > 0.0f && ci->height > 0.0f)
			{
				const float xpos = curX + ci->xoff;
				const float ypos = curY + ci->yoff;

				glTexCoord2f(ci->x0, ci->y0); glVertex2f(xpos, ypos);
				glTexCoord2f(ci->x1, ci->y0); glVertex2f(xpos + ci->width, ypos);
				glTexCoord2f(ci->x1, ci->y1); glVertex2f(xpos + ci->width, ypos + ci->height);
				glTexCoord2f(ci->x0, ci->y1); glVertex2f(xpos, ypos + ci->height);
			}

			curX += ci->advance;
		}

		flush();

		glPopMatrix();
		glPopAttrib();
	}

	void TextRenderer::DrawText(const std::string& text, float x, float y,
		float r, float g, float b, float a)
	{
		if (text.empty()) return;

		std::u32string u32;
		u32.reserve(text.size());
		size_t i = 0;
		while (i < text.size())
		{
			unsigned int cp = DecodeUTF8(text, i);
			if (cp == 0) break;
			u32.push_back(static_cast<char32_t>(cp));
		}
		DrawTextUTF32(u32, x, y, r, g, b, a);
	}

	float TextRenderer::GetTextWidthUTF32(std::u32string_view text) const
	{
		if (!m_Initialized) return 0.0f;
		float w = 0.0f;
		for (char32_t cp : text)
			w += GetAdvance(static_cast<unsigned int>(cp));
		return w;
	}

	float TextRenderer::GetTextWidth(const std::string& text) const
	{
		if (!m_Initialized) return 0.0f;

		float width = 0.0f;
		size_t i = 0;
		while (i < text.size())
		{
			unsigned int cp = DecodeUTF8(text, i);
			if (cp == 0) break;
			width += GetAdvance(cp);
		}
		return width;
	}

	float TextRenderer::GetTextHeight() const
	{
		return m_FontSize;
	}

	float TextRenderer::GetFontSize()
	{
		return Get().m_FontSize;
	}

	float TextRenderer::GetAdvance(unsigned int codepoint) const
	{
		if (!m_Initialized || m_Faces.empty()) return 0.0f;
		if (codepoint == '\t') return static_cast<float>(m_TabWidth) * m_CharWidth;
		if (codepoint == '\n' || codepoint == '\r') return 0.0f;

		auto it = m_GlyphCache.find(codepoint);
		if (it != m_GlyphCache.end()) return it->second.info.advance;

		for (const auto& face : m_Faces)
		{
			if (stbtt_FindGlyphIndex(&face.info, codepoint) != 0)
			{
				int advance = 0, lsb = 0;
				stbtt_GetCodepointHMetrics(&face.info, codepoint, &advance, &lsb);
				return static_cast<float>(advance) * face.scale;
			}
		}
		return 0.0f;
	}

	float TextRenderer::GetLineHeight() const
	{
		if (!m_Initialized || m_Faces.empty()) return m_FontSize;
		const FontFace& main = m_Faces[0];
		return static_cast<float>(main.ascent - main.descent + main.lineGap) * main.scale;
	}

	float TextRenderer::GetCharWidth(char c) const
	{
		if (c == '\t') return static_cast<float>(m_TabWidth) * m_CharWidth;
		if (!m_Initialized) return m_FontSize * 0.5f;

		const unsigned int cp = static_cast<unsigned char>(c);
		const float adv = GetAdvance(cp);
		if (adv > 0.0f) return adv;
		return m_FontSize * 0.5f;
	}

	void TextRenderer::Unload()
	{
		for (auto& face : m_Faces)
		{
			if (face.textureID)
			{
				glDeleteTextures(1, &face.textureID);
				face.textureID = 0;
			}
			face.buffer.clear();
			std::memset(&face.info, 0, sizeof(face.info));
		}
		m_Faces.clear();
		m_GlyphCache.clear();

		m_Initialized = false;
		m_FontSize = 0.0f;
		m_CharWidth = 0.0f;
	}

}
