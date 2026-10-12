#pragma once
#include "Frostnux/ui/uiTools/TextRenderer.h"
#include <unordered_map>
#include <string>

namespace Frostnux {

	enum class FontStyle
	{
		Regular,
		Bold,
		Italic,
		BoldItalic
	};

	class FontManager
	{
	public:
		static FontManager& Get();

		bool LoadFont(const std::string& name, const std::string& filepath, float size, FontStyle style = FontStyle::Regular);
		TextRenderer* GetFont(const std::string& name, FontStyle style = FontStyle::Regular);
		bool AddFallback(const std::string& name, const std::string& filepath, FontStyle style = FontStyle::Regular);
		void UnloadAll();
	private:
		FontManager() = default;
		~FontManager() = default;

		struct FontKey
		{
			std::string name;
			FontStyle style;

			bool operator==(const FontKey& other) const { return name == other.name && style == other.style; }
		};

		struct FontKeyHash
		{
			std::size_t operator()(const FontKey& k) const
			{
				return std::hash<std::string>()(k.name) ^ (std::hash<int>()((int)k.style) << 1);
			}
		};
		std::unordered_map<FontKey, std::unique_ptr<TextRenderer>, FontKeyHash> m_Fonts;
	};

}
