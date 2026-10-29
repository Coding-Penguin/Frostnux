#include <fxpch.h>
#include "SettingsManager.h"
#include "Frostnux/Core/Log.h"
#include "Frostnux/Core/Application.h"
#include "Frostnux/ui/uiTools/SolutionExplorer.h"
#include "Frostnux/ui/CodeEditor/Highlighter.h"

namespace Frostnux {

	SettingsManager& SettingsManager::Get()
	{
		static SettingsManager instance;
		return instance;
	}

	void SettingsManager::Load()
	{
		std::ifstream file(m_Path);
		if (!file.is_open())
		{
			FX_CORE_WARN("Settings file not found, using defaults.");
			CreateFolder("config");
			std::ofstream file(m_Path);
			return;
		}

		try
		{
			nlohmann::json j;
			file >> j;
			m_Settings.languageIndex = j.value("languageIndex", 1);
			m_Settings.themeIndex = j.value("themeIndex", 0);
			m_Settings.fontSize = j.value("fontSize", 24);
			s_FontSize = m_Settings.fontSize;
			m_Settings.recentFiles = j.value("recentFiles", std::vector<std::string>{});
			m_Settings.openFiles = j.value("openFiles", std::vector<std::string>{});
			m_Settings.tokenIndex = j.value("tokenIndex", std::vector<int>{});
			m_Settings.WindowWidth = j.value("WindowWidth", 1920);
			m_Settings.WindowHeight = j.value("WindowHeight", 1080);
			m_Settings.IsMaximize = j.value("IsMaximize", false);
			m_Settings.solutionExplorerScrollY = j.value("solutionExplorerScrollY", 0.0f);


			if (m_Settings.themeIndex < 0 || m_Settings.themeIndex >= ThemeManager::GetThemeCount())
			{
				FX_CORE_WARN("Invalid theme index in settings: {}. Defaulting to Dark theme.", m_Settings.themeIndex);
				ThemeManager::SetTheme(Theme::Dark);
				SettingsManager::Get().Save();
			}
			else
			{
				ThemeManager::SetTheme((Theme)m_Settings.themeIndex);
			}
			if (m_Settings.languageIndex < 0 || m_Settings.languageIndex >= LanguageManager::GetLanguageCount())
			{
				FX_CORE_WARN("Invalid language index in settings: {}. Defaulting to English (US).", m_Settings.languageIndex);
				LanguageManager::SetLanguage(LanguageType::en_US);
				SettingsManager::Get().Save();
			}
			else
			{
				LanguageManager::SetLanguage((LanguageType)m_Settings.languageIndex);
			}
		}
		catch (const std::exception& e)
		{
			FX_CORE_ERROR("Failed to parse settings JSON: {}", e.what());
		}

		SolutionExplorer::SetScrollY(m_Settings.solutionExplorerScrollY);
	}

	void SettingsManager::Save()
	{
		m_Settings.fontSize = s_FontSize;
		m_Settings.languageIndex = (int)LanguageManager::GetCurrentLanguage();
		m_Settings.themeIndex = (int)ThemeManager::GetCurrentTheme();
		if (Application::HasInstance())
		{
			auto& window = Application::Get().GetWindow();
			m_Settings.IsMaximize = window.IsMaximized();
			m_Settings.WindowWidth = window.GetWidth();
			m_Settings.WindowHeight = window.GetHeight();
		}
		m_Settings.solutionExplorerScrollY = SolutionExplorer::GetScrollY();
		m_Settings.tokenIndex = EditorTheme::GetTokenCount();

		nlohmann::json j;
		j["themeIndex"] = m_Settings.themeIndex;
		j["languageIndex"] = m_Settings.languageIndex;
		j["fontSize"] = m_Settings.fontSize;
		j["recentFiles"] = m_Settings.recentFiles;
		j["openFiles"] = m_Settings.openFiles;
		j["tokenIndex"] = m_Settings.tokenIndex;
		j["WindowWidth"] = m_Settings.WindowWidth;
		j["WindowHeight"] = m_Settings.WindowHeight;
		j["solutionExplorerScrollY"] = m_Settings.solutionExplorerScrollY;
		j["IsMaximize"] = m_Settings.IsMaximize;

		std::ofstream file(m_Path);
		if (!file)
		{
			FX_CORE_ERROR("Failed to save settings to {}", m_Path);
			return;
		}
		file << j.dump(4);
		FX_CORE_INFO("Settings saved.");
	}

	void SettingsManager::AddRecentFile(const std::string& filepath)
	{
		auto& recent = m_Settings.recentFiles;

		recent.erase(std::remove(recent.begin(), recent.end(), filepath), recent.end());

		recent.insert(recent.begin(), filepath);

		Save();
	}

	void SettingsManager::SetOpenFiles(const std::vector<std::string>& files)
	{
		m_Settings.openFiles = files;
		Save();
	}

}