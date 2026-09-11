#pragma once

namespace Frostnux
{

	enum class Theme // 6 items
	{
		// Light
		Light = 0,
		Cool_Breeze,
		Icy_Mint,
		// Dark
		Dark,
		Cool_Slate,
		Moonlight
	};

	class ThemeManager
	{
	public:
		static Theme GetCurrentTheme() { return s_CurrentTheme; }
		static void SetTheme(Theme theme) { s_CurrentTheme = theme; }
		static bool IsDarkTheme() { return (s_CurrentTheme == Theme::Dark || s_CurrentTheme == Theme::Cool_Slate || s_CurrentTheme == Theme::Moonlight); }
		static std::tuple<float, float, float> GetBGColor()
		{
			switch (s_CurrentTheme)
			{
			case Theme::Dark:
				return { 0.01f, 0.01f, 0.05f };
			case Theme::Light:
				return { 0.95f, 0.95f, 1.0f };
			case Theme::Cool_Breeze:
				return { 0.8f, 0.9f, 0.95f };
			case Theme::Cool_Slate:
				return { 0.1f, 0.13f, 0.17f };
			case Theme::Icy_Mint:
				return { 0.8f, 0.9f, 0.85f };
			case Theme::Moonlight:
				return { 0.05f, 0.1f, 0.17f };
			}
		}
		static std::string GetThemeName()
		{
			switch (s_CurrentTheme)
			{
			case Theme::Dark:
				return "Dark";
			case Theme::Light:
				return "Light";
			case Theme::Cool_Breeze:
				return "Cool Breeze";
			case Theme::Cool_Slate:
				return "Cool Slate";
			case Theme::Icy_Mint:
				return "Icy Mint";
			case Theme::Moonlight:
				return "Moonlight";
			}
		}
		static int GetThemeCount() { return 6; }
	private:
		ThemeManager() = default;

		ThemeManager(const ThemeManager&) = delete;
		ThemeManager& operator=(const ThemeManager&) = delete;
		ThemeManager(ThemeManager&&) = delete;
		ThemeManager& operator=(ThemeManager&&) = delete;
		ThemeManager& operator=(const ThemeManager&&) = delete;

		static Theme s_CurrentTheme;
	};

}
