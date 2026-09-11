#pragma once

namespace Frostnux {

	enum class LanguageType
	{
		en_US,
		zh_CN,
		ru_RU
	};

	class LanguageManager
	{
	public:
		static LanguageType GetCurrentLanguage() { return s_CurrentLanguageType; }
		static void SetLanguage(LanguageType languageType) { s_CurrentLanguageType = languageType; }
		static std::string GetLanguageName()
		{
			switch (s_CurrentLanguageType)
			{
			case LanguageType::en_US: return "English (US)";
			case LanguageType::zh_CN: return "Chinese (Simplified)";
			case LanguageType::ru_RU: return "Russian";
			default: return "Unknown";
			}
		}
		static std::string GetLanguageCode()
		{
			switch (s_CurrentLanguageType)
			{
			case LanguageType::en_US: return "en-US";
			case LanguageType::zh_CN: return "zh-CN";
			case LanguageType::ru_RU: return "ru-RU";
			default: return "unknown";
			}
		}
	private:
		LanguageManager() = default;

		LanguageManager(const LanguageManager&) = delete;
		LanguageManager& operator=(const LanguageManager&) = delete;
		LanguageManager(LanguageManager&&) = delete;
		LanguageManager& operator=(LanguageManager&&) = delete;
		LanguageManager& operator=(const LanguageManager&&) = delete;

		static LanguageType s_CurrentLanguageType;
	};

}
