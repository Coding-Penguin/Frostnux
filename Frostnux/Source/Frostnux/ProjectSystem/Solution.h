#pragma once
#include "Project.h"
#include <string>
#include <vector>
#include <filesystem>
#include <memory>

namespace Frostnux {

	class Solution
	{
	public:
		std::string name = "Solution";
		std::filesystem::path path;		// .frostnux file path
		std::vector<std::unique_ptr<Project>> projects;

		std::string startupProjectName;

		bool Load(const std::filesystem::path& filepath);
		bool Save(const std::filesystem::path& filepath = {});

		Project* AddProject(const std::string& projName, ProjectKind kind);
		void     RemoveProject(int idx);
		Project* FindProject(const std::string& n);

		Project* GetStartupProject();

		[[nodiscard]] std::filesystem::path Directory() const { return path.parent_path(); }
		[[nodiscard]] bool IsValid() const { return !path.empty(); }
	};

}
