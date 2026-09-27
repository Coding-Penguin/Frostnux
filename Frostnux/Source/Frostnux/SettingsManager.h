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
		std::unordered_map<std::string, bool> fileExplorerExpandedState;

		int languageIndex = 1;
		int themeIndex = 3;
		int channelIndex = 1;
		int fontSize = 24;
		float fileExplorerScrollY = 0.0f;
		unsigned int WindowWidth = 1920, WindowHeight = 1080;
		bool IsMaximize = false;
	};

	class SettingsManager
	{
	public:
		static SettingsManager& Get();

		void Load();
		void Save() const;

		AppSettings& GetSettings() { return m_Settings; }

		void AddRecentFile(const std::string& filepath);
		void SetOpenFiles(const std::vector<std::string>& files);
		void SetTokenIndex(const std::vector<int> tokenIndex);
		void SetThemeIndex(int index);
		void SetLanguageIndex(int index);
		void SetChannel(int index);
		void SetFontSize(int size);
		void SetMaximize(bool isMaximized);

		void SetWindowSize(unsigned int width, unsigned int height);

		void SaveFileExplorerState(float scrollY, const std::unordered_map<std::string, bool>& expandedState);
		void LoadFileExplorerState(float& scrollY, std::unordered_map<std::string, bool>& expandedState);
	private:
		SettingsManager() = default;
		~SettingsManager() = default;
		AppSettings m_Settings;

		std::string m_Path = "config/config.json";
		std::string m_RootPath = "config";
	};

}
