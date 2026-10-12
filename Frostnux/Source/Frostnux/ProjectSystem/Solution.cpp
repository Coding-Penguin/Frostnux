#include "fxpch.h"
#include "Solution.h"
#include "Frostnux/Core/Log.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Frostnux {

	bool Solution::Load(const std::filesystem::path& filepath)
	{
		std::ifstream f(filepath);
		if (!f) { FX_CORE_ERROR("Solution::Load: cannot open {}", filepath.string()); return false; }

		json j;
		try { f >> j; }
		catch (const std::exception& e)
		{
			FX_CORE_ERROR("Solution::Load: JSON error: {}", e.what());
			return false;
		}

		path = filepath;
		name = j.value("name", "MySolution");
		projects.clear();

		const auto base = filepath.parent_path();
		for (const auto& rel : j.value("projects", std::vector<std::string>{}))
		{
			auto projPath = base / rel;
			auto p = std::make_unique<Project>();
			if (p->Load(projPath))
				projects.push_back(std::move(p));
			else
				FX_CORE_WARN("Solution::Load: failed to load {}", projPath.string());
		}

		FX_CORE_INFO("Solution loaded: {} ({} projects)", name, projects.size());
		return true;
	}

	bool Solution::Save(const std::filesystem::path& filepath)
	{
		auto& target = filepath.empty() ? path : filepath;
		if (target.empty()) { FX_CORE_ERROR("Solution::Save: no path"); return false; }
		path = target;
		if (name.empty()) name = path.stem().string();

		const auto base = path.parent_path();

		for (auto& p : projects)
		{
			if (p->path.empty())
			{
				auto projDir = base / p->name;
				std::error_code ec;
				std::filesystem::create_directories(projDir, ec);
				p->path = projDir / (p->name + ".fxproject");
			}
			p->Save();
		}

		json j;
		j["version"] = "1.0";
		j["name"] = name;

		std::vector<std::string> rels;
		for (auto& p : projects)
			rels.push_back(std::filesystem::relative(p->path, base).generic_string());
		j["projects"] = rels;

		std::error_code ec;
		std::filesystem::create_directories(base, ec);

		std::ofstream f(path);
		if (!f) { FX_CORE_ERROR("Solution::Save: cannot write {}", path.string()); return false; }
		f << j.dump(4);
		return true;
	}

	Project* Solution::AddProject(const std::string& projName, ProjectKind kind)
	{
		auto p = std::make_unique<Project>();
		p->name = projName;
		p->kind = kind;
		p->outputName = projName;

		ProjectConfig d; d.name = "Debug";		d.debugInfo = true;
		ProjectConfig r; r.name = "Release";	r.optimization = "/O2";
		p->configurations = { std::move(d), std::move(r) };

		Project* raw = p.get();
		projects.push_back(std::move(p));
		return raw;
	}

	void Solution::RemoveProject(int idx)
	{
		if (idx < 0 || idx >= static_cast<int>(projects.size())) return;
		projects.erase(projects.begin() + idx);
	}

	Project* Solution::FindProject(const std::string& n)
	{
		for (auto& p : projects)
			if (p->name == n) return p.get();
		return nullptr;
	}

}
