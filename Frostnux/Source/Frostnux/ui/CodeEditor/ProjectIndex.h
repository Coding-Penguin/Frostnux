#pragma once
#include "SymbolIndex.h"
#include "TextBuffer.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace Frostnux {

	struct SymbolMatch
	{
		const Symbol*	symbol = nullptr;
		std::string		filePath;
		int				priority = 0;
	};

	class ProjectIndex
	{
	public:
		void SetRoot(const std::string& root) { m_Root = root; }
		[[nodiscard]] const std::string& GetRoot() const { return m_Root; }

		void Scan();
		void IndexFile(const std::string& path);
		void RemoveFile(const std::string& path);

		[[nodiscard]] std::vector<SymbolMatch> FindSymbol(std::u32string_view name, const std::string& fromFile, int maxResults = 16) const;

		[[nodiscard]] const std::vector<std::string>& GetIncludes(const std::string& path) const;

		[[nodiscard]] bool IsIndexed(const std::string& path) const;
		[[nodiscard]] size_t FileCount() const { return m_Indices.size(); }
	private:
		void ScanDir(const std::string& dir);
		void ParseIncludes(const std::string& path, const TextBuffer& buffer, std::vector<std::string>& out);

		std::string m_Root;
		std::unordered_map<std::string, SymbolIndex> m_Indices;
		std::unordered_map<std::string, std::vector<std::string>> m_Includes;
	};

}
