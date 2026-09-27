#pragma once

namespace Frostnux {

	enum class LicenseType
	{
		Community,
		Professional,
		Enterprise
	};

	class LicenseManager
	{
	public:
		static LicenseType GetCurrentLicense() { return s_CurrentLicense; }
		static void SetLicense(LicenseType license) { s_CurrentLicense = license; }

		static std::string GetLicenseName()
		{
			switch (s_CurrentLicense)
			{
			case LicenseType::Community:
				return "Community";
			case LicenseType::Professional:
				return "Professional";
			case LicenseType::Enterprise:
				return "Enterprise";
			}
		}
	private:
		LicenseManager() = default;

		LicenseManager(const LicenseManager&) = delete;
		LicenseManager& operator=(const LicenseManager&) = delete;
		LicenseManager(LicenseManager&&) = delete;
		LicenseManager& operator=(LicenseManager&&) = delete;
		LicenseManager& operator=(const LicenseManager&&) = delete;

		static LicenseType s_CurrentLicense;
	};

}

