#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace Frostnux {

	struct AppSettings
	{
		std::vector<std::string> recentFiles;
		std::vector<std::string> openFiles;
		std::vector<int> tokenIndex;

		int languageIndex = 1;
		int themeIndex = 0;
		int fontSize = 24;
		float solutionExplorerScrollY = 0.0f;
		unsigned int WindowWidth = 1920, WindowHeight = 1080;
		bool IsMaximize = false;
	};

	class SettingsManager
	{
	public:
		static SettingsManager& Get();

		void Load();
		void Save();

		AppSettings& GetSettings() { return m_Settings; }

		void AddRecentFile(const std::string& filepath);
		void SetOpenFiles(const std::vector<std::string>& files);
	private:
		SettingsManager() = default;
		~SettingsManager() = default;
		AppSettings m_Settings;

		std::string m_Path = "config/config.json";
		std::string m_RootPath = "config";
	};

}
