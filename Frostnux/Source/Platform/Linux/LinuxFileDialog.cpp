#include "fxpch.h"

#if defined(FX_PLATFORM_LINUX) && !defined(FX_PLATFORM_ANDROID)

#include "FileDialog.h"
#include <cstdio>
#include <array>
#include <sstream>

namespace Frostnux {

	namespace {

		std::string FirstPattern(const std::string& filter)
		{
			auto first = filter.find('|');
			if (first == std::string::npos) return "*.*";
			auto end = filter.find('|', first + 1);
			std::string patterns = filter.substr(first + 1,
				end == std::string::npos ? std::string::npos : end - first - 1);

			std::string out;
			std::stringstream ss(patterns);
			std::string p;
			bool first2 = true;
			while (std::getline(ss, p, ';'))
			{
				if (!first2) out += ' ';
				out += p;
				first2 = false;
			}
			return out.empty() ? "*.*" : out;
		}

		std::string RunCommand(const std::string& cmd)
		{
			std::array<char, 4096> buf{};
			std::string result;

			FILE* pipe = popen(cmd.c_str(), "r");
			if (!pipe) return {};

			while (fgets(buf.data(), (int)buf.size(), pipe))
				result += buf.data();

			const int status = pclose(pipe);
			if (status != 0) return {};

			while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
				result.pop_back();

			return result;
		}
	}

	std::string SaveFileDialog(const std::string& defaultName, const std::string& filter)
	{
		const std::string patterns = FirstPattern(filter);

		std::string cmd = "zenity --file-selection --save --confirm-overwrite";
		cmd += " --file-filter='" + patterns + "'";
		if (!defaultName.empty())
			cmd += " --filename='" + defaultName + "'";
		cmd += " 2>/dev/null";

		return RunCommand(cmd);
	}

	std::string OpenFileDialog(const std::string& filter)
	{
		const std::string patterns = FirstPattern(filter);

		std::string cmd = "zenity --file-selection";
		cmd += " --file-filter='" + patterns + "'";
		cmd += " 2>/dev/null";

		return RunCommand(cmd);
	}

}

#endif
