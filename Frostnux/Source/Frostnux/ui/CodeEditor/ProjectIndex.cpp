#include "fxpch.h"
#include "ProjectIndex.h"
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace Frostnux {

	namespace fs = std::filesystem;

	namespace {

		bool IsCodeFile(const fs::path& p)
		{
			static const std::unordered_set<std::string> exts =
			{
				".cpp", ".c", ".cc", ".cxx",
				".h", ".hpp", ".hxx", ".hh",
			};
			return exts.count(p.extension().string()) > 0;
		}

		std::string NormalizePath(const std::string& path)
		{
			std::error_code ec;
			auto p = fs::weakly_canonical(path, ec);
			if (ec) return path;
			return p.generic_string();
		}

	}

	void ProjectIndex::Scan()
	{
		if (m_Root.empty()) return;
		m_Indices.clear();
		m_Includes.clear();
		ScanDir(m_Root);
	}

	void ProjectIndex::ScanDir(const std::string& dir)
	{
		std::error_code ec;
		fs::directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec);
		if (ec) return;

		for (const auto& entry : it)
		{
			const auto& p = entry.path();
			const std::string name = p.filename().string();

			if (name == ".git" || name == "build" || name == "bin" || name == "bin-int" || name == "Binaries" || name == "Intermediate" || name == "Intermediates" || name == "Binaries-Intermediates" || name == "node_modules" || name == ".vs" || name == "vendor" || name == ".vscode" || (!name.empty() && name[0] == '.'))
				continue;

			std::error_code ec2;
			if (entry.is_directory(ec2))
			{
				ScanDir(p.generic_string());
			}
			else if (entry.is_regular_file(ec2))
			{
				if (IsCodeFile(p))
					IndexFile(p.generic_string());
			}
		}
	}

	void ProjectIndex::IndexFile(const std::string& path)
	{
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file) return;

		const std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::string content(static_cast<size_t>(size), '\0');
		if (!file.read(content.data(), size)) return;

		if (content.size() >= 3 && static_cast<unsigned char>(content[0]) == 0xEF && static_cast<unsigned char>(content[1]) == 0xBB && static_cast<unsigned char>(content[2]) == 0xBF)
			content.erase(0, 3);

		TextBuffer buf;
		buf.setText(utf8_to_u32(content));

		const std::string key = NormalizePath(path);

		SymbolIndex idx;
		idx.rebuild(buf);
		m_Indices[key] = std::move(idx);

		std::vector<std::string> includes;
		ParseIncludes(key, buf, includes);
		m_Includes[key] = std::move(includes);
	}

	void ProjectIndex::RemoveFile(const std::string& path)
	{
		const std::string key = NormalizePath(path);
		m_Indices.erase(key);
		m_Includes.erase(key);
	}

	void ProjectIndex::ParseIncludes(const std::string& path, const TextBuffer& buffer, std::vector<std::string>& out)
	{
		const fs::path dir = fs::path(path).parent_path();
		const int n = buffer.lineCount();

		for (int i = 0; i < n; ++i)
		{
			auto line = buffer.line(i);
			if (line.empty()) continue;

			size_t k = 0;
			while (k < line.size() && (line[k] == U' ' || line[k] == U'\t')) ++k;
			if (k >= line.size() || line[k] != U'#') continue;
			++k;
			while (k < line.size() && (line[k] == U' ' || line[k] == U'\t')) ++k;

			constexpr std::u32string_view kw = U"include";
			if (k + kw.size() > line.size()) continue;
			if (line.substr(k, kw.size()) != kw) continue;
			k += kw.size();
			while (k < line.size() && (line[k] == U' ' || line[k] == U'\t')) ++k;
			if (k >= line.size()) continue;

			const char32_t open = line[k];
			char32_t close;
			bool isSystem;
			if (open == U'"') { close = U'"'; isSystem = false; }
			else if (open == U'<') { close = U'>'; isSystem = true; }
			else continue;

			++k;
			const size_t start = k;
			while (k < line.size() && line[k] != close) ++k;
			if (k >= line.size()) continue;

			const std::string incStr = u32_to_utf8(line.substr(start, k - start));
			if (isSystem) continue;

			std::error_code ec;
			fs::path resolved = dir / incStr;
			if (!fs::exists(resolved, ec) && !m_Root.empty())
			{
				fs::path alt = fs::path(m_Root) / incStr;
				if (fs::exists(alt, ec)) resolved = alt;
				else continue;
			}

			out.push_back(NormalizePath(resolved.generic_string()));
		}
	}

	std::vector<SymbolMatch> ProjectIndex::FindSymbol(std::u32string_view name, const std::string& fromFile, int maxResults) const
	{
		std::vector<SymbolMatch> out;
		const std::string fromKey = NormalizePath(fromFile);

		{
			auto it = m_Indices.find(fromKey);
			if (it != m_Indices.end())
			{
				if (const Symbol* s = it->second.findSymbolAnywhere(name))
					out.push_back({ s, fromKey, 0 });
			}
		}

		std::unordered_set<std::string> includeSet;
		{
			auto it = m_Includes.find(fromKey);
			if (it != m_Includes.end())
			{
				for (const auto& inc : it->second)
				{
					includeSet.insert(inc);
					auto ij = m_Indices.find(inc);
					if (ij == m_Indices.end()) continue;
					if (const Symbol* s = ij->second.findSymbolAnywhere(name))
						out.push_back({ s, inc, 1 });
				}
			}
		}

		for (const auto& [path, idx] : m_Indices)
		{
			if (path == fromKey) continue;
			if (includeSet.count(path)) continue;
			if (const Symbol* s = idx.findSymbolAnywhere(name))
			{
				out.push_back({ s, path, 2 });
				if (static_cast<int>(out.size()) >= maxResults * 2) break;
			}
		}

		std::stable_sort(out.begin(), out.end(),
			[](const SymbolMatch& a, const SymbolMatch& b)
			{
				return a.priority < b.priority;
			});

		if (static_cast<int>(out.size()) > maxResults) out.resize(maxResults);
		return out;
	}

	const std::vector<std::string>& ProjectIndex::GetIncludes(const std::string& path) const
	{
		static const std::vector<std::string> empty;
		const std::string key = NormalizePath(path);
		auto it = m_Includes.find(key);
		return it != m_Includes.end() ? it->second : empty;
	}

	bool ProjectIndex::IsIndexed(const std::string& path) const
	{
		return m_Indices.count(NormalizePath(path)) > 0;
	}

}
