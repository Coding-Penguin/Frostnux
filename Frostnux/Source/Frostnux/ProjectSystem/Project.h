#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace Frostnux {

	enum class ProjectKind
	{
		ConsoleApp,
		StaticLib,
		DynamicLib
	};

	struct ProjectConfig
	{
		std::string	name = "Debug";
		bool		debugInfo = true;
		std::string optimization = "/Od";
		std::vector<std::string> flags;
	};

	class Project
	{
	public:
		std::string			name = "MyProject";
		ProjectKind			kind = ProjectKind::ConsoleApp;
		std::string			language = "cpp";
		std::string			standard = "c++20";
		std::filesystem::path path;      // .fxproject file path

		std::vector<std::string> sources;
		std::vector<std::string> headers;
		std::vector<std::string> includeDirs;
		std::vector<std::string> defines;
		std::vector<std::string> libraries;
		std::string outputDir = "Binaries";
		std::string outputName;

		std::vector<ProjectConfig> configurations;
		std::string activeConfig = "Debug";

		bool Load(const std::filesystem::path& filepath);
		bool Save(const std::filesystem::path& filepath = {});

		[[nodiscard]] std::filesystem::path Directory() const { return path.parent_path(); }
		[[nodiscard]] const ProjectConfig* ActiveConfig() const;

		[[nodiscard]] std::vector<std::filesystem::path> ResolveSources() const;
		[[nodiscard]] std::vector<std::filesystem::path> ResolveHeaders() const;
	};

}
