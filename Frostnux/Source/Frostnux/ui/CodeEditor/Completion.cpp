#include "fxpch.h"
#include "Completion.h"
#include <unordered_set>
#include <algorithm>

namespace Frostnux {

	// ---------- KeyWord Map ----------
	namespace
	{

		const std::vector<std::u32string>& Keywords()
		{
			static const std::vector<std::u32string> k =
			{
				U"alignas", U"alignof", U"auto", U"bool", U"break", U"case",
				U"catch", U"char", U"char16_t", U"char32_t", U"char8_t",
				U"class", U"concept", U"const", U"consteval", U"constexpr",
				U"constinit", U"continue", U"co_await", U"co_return", U"co_yield",
				U"decltype", U"default", U"delete", U"do", U"double",
				U"else", U"enum", U"explicit", U"extern", U"false", U"float",
				U"for", U"friend", U"goto", U"if", U"inline", U"int", U"long",
				U"mutable", U"namespace", U"new", U"noexcept", U"nullptr",
				U"operator", U"override", U"private", U"protected", U"public",
				U"register", U"requires", U"return", U"short", U"signed",
				U"sizeof", U"static", U"static_assert", U"struct", U"switch",
				U"template", U"this", U"throw", U"true", U"try", U"typedef",
				U"typename", U"union", U"unsigned", U"using", U"virtual",
				U"void", U"volatile", U"wchar_t", U"while",
				U"string", U"vector", U"map", U"set", U"unordered_map",
				U"unordered_set", U"pair", U"tuple", U"array",
				U"shared_ptr", U"unique_ptr", U"weak_ptr",
				U"size_t", U"ptrdiff_t", U"uint32_t", U"int32_t",
				U"uint64_t", U"int64_t", U"string_view", U"span",
			};
			return k;
		}

		CompletionKind classifyKeyword(std::u32string_view w)
		{
			if (w == U"if" || w == U"else" || w == U"for" || w == U"while"
				|| w == U"do" || w == U"switch" || w == U"case" || w == U"default"
				|| w == U"break" || w == U"continue" || w == U"return" || w == U"goto"
				|| w == U"try" || w == U"catch" || w == U"throw")
				return CompletionKind::Control;

			if (w == U"int" || w == U"long" || w == U"short" || w == U"char"
				|| w == U"bool" || w == U"float" || w == U"double" || w == U"void"
				|| w == U"size_t" || w == U"string" || w == U"vector")
				return CompletionKind::Type;

			return CompletionKind::Keyword;
		}

	}

	bool CompletionEngine::isIdentifierChar(char32_t c) noexcept
	{
		return (c >= U'a' && c <= U'z')
			|| (c >= U'A' && c <= U'Z')
			|| (c >= U'0' && c <= U'9')
			|| c == U'_'
			|| c > 127;
	}

	std::u32string CompletionEngine::extractPrefix(const TextBuffer& buf, Position cursor)
	{
		if (cursor.line < 0 || cursor.line >= buf.lineCount()) return {};
		const auto line = buf.line(cursor.line);
		if (line.empty()) return {};

		int end = std::clamp(cursor.col, 0, static_cast<int>(line.size()));
		int start = end;
		while (start > 0 && isIdentifierChar(line[start - 1]))
			--start;

		if (start == end) return {};
		return std::u32string(line.substr(start, end - start));
	}

	void CompletionEngine::rebuildIndex(const TextBuffer& buf)
	{
		std::unordered_set<std::u32string> seen;
		const int n = buf.lineCount();

		for (int i = 0; i < n; ++i)
		{
			auto line = buf.line(i);
			const int len = static_cast<int>(line.size());
			int j = 0;
			while (j < len)
			{
				if (!isIdentifierChar(line[j])) { ++j; continue; }
				int s = j;
				while (j < len && isIdentifierChar(line[j])) ++j;
				if (j - s >= 2 && j - s <= 64)
				{
					std::u32string w(line.substr(s, j - s));
					char32_t c0 = w[0];
					if (!(c0 >= U'0' && c0 <= U'9'))
						seen.insert(std::move(w));
				}
			}
		}

		m_Words.assign(seen.begin(), seen.end());
		std::sort(m_Words.begin(), m_Words.end());
	}

	std::vector<CompletionItem> CompletionEngine::query(std::u32string_view prefix, size_t limit) const
	{
		std::vector<CompletionItem> out;
		if (prefix.empty()) return out;

		out.reserve(limit);

		for (const auto& kw : Keywords())
		{
			if (out.size() >= limit) break;
			if (kw.size() >= prefix.size()
				&& std::u32string_view(kw).substr(0, prefix.size()) == prefix)
			{
				out.push_back({ kw, kw, classifyKeyword(kw) });
			}
		}

		for (const auto& w : m_Words)
		{
			if (out.size() >= limit) break;
			if (w.size() < prefix.size()) continue;
			if (std::u32string_view(w).substr(0, prefix.size()) != prefix) continue;

			bool dup = false;
			for (const auto& it : out)
				if (it.label == w) { dup = true; break; }
			if (dup) continue;

			out.push_back({ w, w, CompletionKind::Identifier });
		}

		std::stable_sort(out.begin(), out.end(),
			[](const CompletionItem& a, const CompletionItem& b)
			{
				return a.label.size() < b.label.size();
			});

		if (out.size() > limit) out.resize(limit);
		return out;
	}

}
