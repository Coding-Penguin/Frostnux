#include <fxpch.h>
#include "FontManager.h"

namespace Frostnux {

	FontManager& FontManager::Get()
	{
		static FontManager instance;
		return instance;
	}

	bool FontManager::LoadFont(const std::string& name, const std::string& filepath, float size, FontStyle style)
	{
		FontKey key{ name, style };
		auto it = m_Fonts.find(key);
		if (it != m_Fonts.end())
		{
			it->second->Unload();
			m_Fonts.erase(it);
		}
		auto font = std::make_unique<TextRenderer>();
		if (!font->LoadFont(filepath, size))
		{
			FX_CORE_ERROR("Failed to load font: {} at size {}", filepath, size);
			return false;
		}
		m_Fonts[key] = std::move(font);
		return true;
	}

	TextRenderer* FontManager::GetFont(const std::string& name, FontStyle style)
	{
		FontKey key{ name, style };
		auto it = m_Fonts.find(key);
		if (it != m_Fonts.end()) return it->second.get();
		return nullptr;
	}

	bool FontManager::AddFallback(const std::string& name, const std::string& filepath, FontStyle style)
	{
		FontKey key{ name, style };
		auto it = m_Fonts.find(key);
		if (it == m_Fonts.end())
		{
			FX_CORE_ERROR("AddFallback: font '{}' not found", name);
			return false;
		}
		return it->second->AddFallback(filepath);
	}

	void FontManager::UnloadAll()
	{
		m_Fonts.clear();
	}

}