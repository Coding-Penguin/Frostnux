#pragma once

namespace Frostnux
{

	enum class Theme
	{
		// Light
		Paper = 0,
		Frost,
		// Dark
		Dark,
		Iron
	};

	class ThemeManager
	{
	public:
		static Theme GetCurrentTheme() { return s_CurrentTheme; }
		static void SetTheme(Theme theme) { s_CurrentTheme = theme; }
		static bool IsDarkTheme() { return (s_CurrentTheme == Theme::Dark || s_CurrentTheme == Theme::Iron); }
		static std::tuple<float, float, float> GetBGColor()
		{
			switch (s_CurrentTheme)
			{
			case Theme::Dark:
				return { 0.01f, 0.01f, 0.02f };
			case Theme::Paper:
				return { 1.0f, 0.97f, 0.93f };
			case Theme::Frost:
				return { 0.87f, 0.87f, 0.9f };
			case Theme::Iron:
				return { 0.01f, 0.03f, 0.07f };
			}
		}
		static std::string GetThemeName()
		{
			switch (s_CurrentTheme)
			{
			case Theme::Dark:
				return "Dark";
			case Theme::Paper:
				return "Paper";
			case Theme::Frost:
				return "Frost";
			case Theme::Iron:
				return "Iron";
			}
		}
		static int GetThemeCount() { return 4; }
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
