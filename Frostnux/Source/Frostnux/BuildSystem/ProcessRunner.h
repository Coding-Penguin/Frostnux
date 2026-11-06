#pragma once
#include <string>
#include <functional>

namespace Frostnux {

	class ProcessRunner
	{
	public:
		static void Run(const std::string& command, const std::string& workingDir, std::function<void(const std::string&)> onOutput, std::function<void(int)> onExit);
	};

}