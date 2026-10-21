#include "fxpch.h"
#include "Project.h"
#include "Frostnux/Core/Log.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Frostnux {

	namespace {
		std::string KindToString(ProjectKind k)
		{
			switch (k)
			{
			case ProjectKind::StaticLib:	return "StaticLib";
			case ProjectKind::DynamicLib:	return "DynamicLib";
			default:						return "ConsoleApp";
			}
		}
		ProjectKind StringToKind(const std::string& s)
		{
			if (s == "StaticLib")  return ProjectKind::StaticLib;
			if (s == "DynamicLib") return ProjectKind::DynamicLib;
			return ProjectKind::ConsoleApp;
		}
	}

	bool Project::Load(const std::filesystem::path& filepath)
	{
		std::ifstream f(filepath);
		if (!f) { FX_CORE_ERROR("Project::Load: cannot open {}", filepath.string()); return false; }

		json j;
		try { f >> j; }
		catch (const std::exception& e)
		{
			FX_CORE_ERROR("Project::Load: JSON error: {}", e.what());
			return false;
		}

		path = filepath;
		path = path.lexically_normal();
		name = j.value("name", "MyProject");
		kind = StringToKind(j.value("kind", "ConsoleApp"));
		language = j.value("language", "cpp");
		standard = j.value("standard", "c++20");
		outputDir = j.value("outputDir", "bin");
		outputName = j.value("outputName", "");
		sources = j.value("sources", std::vector<std::string>{});
		headers = j.value("headers", std::vector<std::string>{});
		includeDirs = j.value("includeDirs", std::vector<std::string>{});
		defines = j.value("defines", std::vector<std::string>{});
		libraries = j.value("libraries", std::vector<std::string>{});

		configurations.clear();
		if (j.contains("configurations") && j["configurations"].is_object())
		{
			for (auto& [key, val] : j["configurations"].items())
			{
				ProjectConfig c;
				c.name = key;
				c.debugInfo = val.value("debugInfo", false);
				c.optimization = val.value("optimization", "/Od");
				c.flags = val.value("flags", std::vector<std::string>{});
				configurations.push_back(std::move(c));
			}
		}
		if (configurations.empty())
		{
			ProjectConfig d; d.name = "Debug";   d.debugInfo = true;
			ProjectConfig r; r.name = "Release"; r.optimization = "/O2";
			configurations = { std::move(d), std::move(r) };
		}
		activeConfig = j.value("activeConfig", "Debug");

		if (outputName.empty()) outputName = name;

		FX_CORE_INFO("Project loaded: {} ({})", name, path.string());
		return true;
	}

	bool Project::Save(const std::filesystem::path& filepath)
	{
		auto& target = filepath.empty() ? path : filepath;
		if (target.empty()) { FX_CORE_ERROR("Project::Save: no path"); return false; }
		path = target;
		if (outputName.empty()) outputName = name;

		json j;
		j["version"] = s_Version;
		j["name"] = name;
		j["kind"] = KindToString(kind);
		j["language"] = language;
		j["standard"] = standard;
		j["sources"] = sources;
		j["headers"] = headers;
		j["includeDirs"] = includeDirs;
		j["defines"] = defines;
		j["libraries"] = libraries;
		j["outputDir"] = outputDir;
		j["outputName"] = outputName;
		j["activeConfig"] = activeConfig;

		json cfgs = json::object();
		for (const auto& c : configurations)
			cfgs[c.name] = 
			{
				{"debugInfo", c.debugInfo},
				{"optimization", c.optimization},
				{"flags", c.flags}
			};
		j["configurations"] = cfgs;

		std::error_code ec;
		std::filesystem::create_directories(target.parent_path(), ec);

		std::ofstream f(target);
		if (!f) { FX_CORE_ERROR("Project::Save: cannot write {}", target.string()); return false; }
		f << j.dump(4);
		return true;
	}

	namespace {

		[[nodiscard]] bool IsIgnoredDir(const std::string& name)
		{
			static const std::unordered_set<std::string> ignored = 
			{
				"bin", "obj", "build", "out", "output",
				".git", ".svn", ".hg", ".vs", ".vscode", ".idea",
				"node_modules", "__pycache__", "Debug", "Release",
				"x64", "x86", "ARM", "ARM64"
			};
			std::string lower;
			lower.reserve(name.size());
			for (char c : name)
				lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			return ignored.count(lower) > 0;
		}

		[[nodiscard]] bool IsSourceExt(const std::string& ext)
		{
			return ext == ".cpp" || ext == ".c" || ext == ".cc" || ext == ".cxx" || ext == ".c++";
		}

		[[nodiscard]] bool IsHeaderExt(const std::string& ext)
		{
			return ext == ".h" || ext == ".hpp" || ext == ".hxx" || ext == ".hh" || ext == ".h++" || ext == ".inl";
		}

		void CollectFiles(const std::filesystem::path& root, std::vector<std::filesystem::path>& sources, std::vector<std::filesystem::path>& headers)
		{
			std::error_code ec;
			if (!std::filesystem::is_directory(root, ec)) return;

			for (const auto& entry : std::filesystem::directory_iterator(root, ec))
			{
				if (ec) break;
				const auto& p = entry.path();
				const std::string name = p.filename().string();

				if (!name.empty() && name[0] == '.') continue;

				if (entry.is_directory(ec))
				{
					if (IsIgnoredDir(name)) continue;
					CollectFiles(p, sources, headers);
					continue;
				}

				if (!entry.is_regular_file(ec)) continue;

				std::string ext;
				auto dot = name.find_last_of('.');
				if (dot != std::string::npos)
				{
					ext = name.substr(dot);
					for (auto& c : ext)
						c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
				}

				if (IsSourceExt(ext))		sources.push_back(p);
				else if (IsHeaderExt(ext))	headers.push_back(p);
			}
		}

	}

	std::vector<std::filesystem::path> Project::ResolveSources() const
	{
		std::vector<std::filesystem::path> sources;
		std::vector<std::filesystem::path> headers;
		CollectFiles(Directory(), sources, headers);

		std::sort(sources.begin(), sources.end());
		return sources;
	}

	std::vector<std::filesystem::path> Project::ResolveHeaders() const
	{
		std::vector<std::filesystem::path> sources;
		std::vector<std::filesystem::path> headers;
		CollectFiles(Directory(), sources, headers);

		std::sort(headers.begin(), headers.end());
		return headers;
	}

	const ProjectConfig* Project::ActiveConfig() const
	{
		for (const auto& c : configurations)
			if (c.name == activeConfig) return &c;
		return configurations.empty() ? nullptr : &configurations.front();
	}

}
