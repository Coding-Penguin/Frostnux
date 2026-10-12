#pragma once
#include <string>
#include <vector>

namespace Frostnux {

	struct ProcessResult
	{
		int			exitCode = -1;
		std::string	output;
	};

	class Process
	{
	public:

		static ProcessResult Run(const std::string& command, const std::vector<std::string>& args, const std::string& workingDir = {});

		static bool Exists(const std::string& command);
	};

}
