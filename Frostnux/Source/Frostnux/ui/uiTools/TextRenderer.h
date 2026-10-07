#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <optional>
#include <stb_truetype.h>

namespace Frostnux {

	class TextRenderer
	{
	public:
		TextRenderer() = default;
		~TextRenderer();

		static TextRenderer& Get();

		bool LoadFont(const std::string& fontPath, float fontSize);

		bool AddFallback(const std::string& fontPath);

		void DrawText(const std::string& text, float x, float y,
			float r, float g, float b, float a);
		void DrawTextUTF32(std::u32string_view text, float x, float y,
			float r, float g, float b, float a);

		float GetTextWidth(const std::string& text) const;
		float GetTextWidthUTF32(std::u32string_view text) const;
		float GetTextHeight() const;

		static float GetFontSize();

		void Unload();

		bool IsInitialized() const { return m_Initialized; }

		float GetCharWidth(char c) const;
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

		struct FontFace
		{
			std::vector<unsigned char> buffer;
			stbtt_fontinfo info{};
			float scale = 0.0f;
			int   ascent = 0, descent = 0, lineGap = 0;

			unsigned int textureID = 0;
			int atlasW = 0, atlasH = 0;
			int atlasX = 1, atlasY = 1, atlasRowHeight = 0;
		};

		struct CachedGlyph
		{
			int			faceIdx = -1;
			CharInfo	info{};
		};

		bool LoadFace(FontFace& face, const std::string& path, float fontSize);
		void CreateAtlas(FontFace& face, int w, int h);

		std::optional<CharInfo> BakeGlyphToFace(FontFace& face, unsigned int cp) const;
		const CharInfo* GetOrCreateGlyph(unsigned int cp, int& outFaceIdx);

		bool m_Initialized = false;
		float m_FontSize = 0.0f;
		float m_CharWidth = 0.0f;
		int   m_TabWidth = 4;

		std::vector<FontFace> m_Faces;
		std::unordered_map<unsigned int, CachedGlyph> m_GlyphCache;
	};

}
