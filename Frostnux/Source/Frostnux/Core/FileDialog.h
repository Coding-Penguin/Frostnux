#pragma once
#include <string>

namespace Frostnux {

	[[nodiscard]] std::string SaveFileDialog(const std::string& defaultName = "untitled.cpp", const std::string& filter = "C++ Files|*.cpp;*.h;*.hpp;*.c|All Files|*.*");

	[[nodiscard]] std::string OpenFileDialog(const std::string& filter = "All Files|*.*");

}
