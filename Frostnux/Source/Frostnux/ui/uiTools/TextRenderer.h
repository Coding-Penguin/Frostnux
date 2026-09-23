#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <stb_truetype.h>
#include <string_view>

namespace Frostnux {

	class TextRenderer
	{
	public:
		TextRenderer() = default;
		~TextRenderer();

		static TextRenderer& Get();

		bool LoadFont(const std::string& fontPath, float fontSize);
		void DrawText(const std::string& text, float x, float y, float r, float g, float b, float a);
		float GetTextWidth(const std::string& text) const;
		float GetTextHeight() const;

		static float GetFontSize();

		void Unload();

		bool IsInitialized() const { return m_Initialized; }

		float GetCharWidth(char c) const;

		float GetTextWidthUTF32(std::u32string_view text) const;
		void  DrawTextUTF32(std::u32string_view text, float x, float y, float r, float g, float b, float a);

		float GetAdvance(unsigned int codepoint) const;
		float GetLineHeight() const;
	private:
		struct CharInfo
		{
			float advance;
			float width, height;
			float x0, y0;
			float x1, y1;
			float xoff, yoff;
		};

		const CharInfo* BakeGlyph(unsigned int codepoint);
		const CharInfo* GetOrCreateGlyph(unsigned int codepoint);

		bool m_Initialized = false;
		unsigned int m_TextureID = 0;
		int m_AtlasWidth = 0, m_AtlasHeight = 0;

		std::unordered_map<unsigned int, CharInfo> m_Chars;

		int m_TabWidth = 4;
		float m_CharWidth = 0.0f;

		std::vector<unsigned char> m_FontBuffer;
		stbtt_fontinfo m_FontInfo{};
		float m_Scale = 0.0f;
		int   m_Ascent = 0, m_Descent = 0, m_LineGap = 0;
		float m_FontSize = 0.0f;

		int m_AtlasX = 1, m_AtlasY = 1, m_AtlasRowHeight = 0;
	};

}
