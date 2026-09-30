#include "fxpch.h"
#include "Completion.h"
#include <algorithm>
#include <unordered_set>

namespace Frostnux {

	bool CompletionEngine::isIdentifierChar(char32_t c) noexcept
	{
		return (c >= U'a' && c <= U'z')
			|| (c >= U'A' && c <= U'Z')
			|| (c >= U'0' && c <= U'9')
			|| c == U'_'
			|| c > 127;
	}

	const std::vector<std::u32string>& CompletionEngine::keywords()
	{
		static const std::vector<std::u32string> k = {
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
		};
		return k;
	}

	namespace {

		[[nodiscard]] bool startsWith(std::u32string_view s, std::u32string_view p)
		{
			if (p.empty()) return true;
			if (s.size() < p.size()) return false;
			return s.substr(0, p.size()) == p;
		}

		[[nodiscard]] bool alreadyContains(const std::vector<CompletionItem>& items,
			std::u32string_view name)
		{
			for (const auto& it : items)
				if (it.label == name) return true;
			return false;
		}

		[[nodiscard]] CompletionKind classifyKeyword(std::u32string_view w)
		{
			static const std::unordered_set<std::u32string> ctrl =
			{
				U"if", U"else", U"for", U"while", U"do", U"switch", U"case",
				U"default", U"break", U"continue", U"return", U"goto",
				U"try", U"catch", U"throw", U"co_await", U"co_return", U"co_yield"
			};
			static const std::unordered_set<std::u32string> type =
			{
				U"int", U"long", U"short", U"char", U"bool", U"float", U"double",
				U"void", U"wchar_t", U"char8_t", U"char16_t", U"char32_t",
				U"signed", U"unsigned", U"auto"
			};
			std::u32string key(w);
			if (ctrl.count(key)) return CompletionKind::Control;
			if (type.count(key)) return CompletionKind::Type;
			return CompletionKind::Keyword;
		}

		[[nodiscard]] CompletionKind toCompletionKind(SymbolKind k)
		{
			switch (k)
			{
			case SymbolKind::Namespace: return CompletionKind::Namespace;
			case SymbolKind::Class:     return CompletionKind::Class;
			case SymbolKind::Struct:    return CompletionKind::Struct;
			case SymbolKind::Union:     return CompletionKind::Struct;
			case SymbolKind::Enum:      return CompletionKind::Enum;
			case SymbolKind::EnumValue: return CompletionKind::EnumValue;
			case SymbolKind::Function:  return CompletionKind::Function;
			case SymbolKind::Variable:  return CompletionKind::Identifier;
			case SymbolKind::Member:    return CompletionKind::Member;
			default:                    return CompletionKind::Identifier;
			}
		}

		[[nodiscard]] int kindWeight(CompletionKind k)
		{
			switch (k)
			{
			case CompletionKind::Keyword:   return 5;
			case CompletionKind::Control:   return 4;
			case CompletionKind::Type:      return 3;
			case CompletionKind::Function:  return 2;
			case CompletionKind::Class:
			case CompletionKind::Struct:
			case CompletionKind::Enum:      return 1;
			case CompletionKind::Namespace: return 1;
			default:                        return 0;
			}
		}

	}

	std::u32string CompletionEngine::extractPrefix(std::u32string_view line, int col)
	{
		if (col <= 0 || col > (int)line.size()) return {};
		int s = col;
		while (s > 0 && isIdentifierChar(line[s - 1])) --s;
		if (s == col) return {};
		return std::u32string(line.substr(s, col - s));
	}

	CompletionEngine::Context CompletionEngine::detectContext(std::u32string_view line, int prefixStart, int& qualifierEnd)
	{
		int p = prefixStart - 1;
		while (p >= 0 && (line[p] == U' ' || line[p] == U'\t')) --p;
		if (p < 0) { qualifierEnd = -1; return Context::Global; }

		if (line[p] == U':' && p > 0 && line[p - 1] == U':')
		{
			qualifierEnd = p - 1;
			return Context::Scope;
		}
		if (line[p] == U'>' && p > 0 && line[p - 1] == U'-')
		{
			qualifierEnd = p - 1;
			return Context::Member;
		}
		if (line[p] == U'.')
		{
			qualifierEnd = p;
			return Context::Member;
		}

		qualifierEnd = -1;
		return Context::Global;
	}

	std::u32string CompletionEngine::extractQualifier(std::u32string_view line, int endCol)
	{
		if (endCol <= 0) return {};
		int p = endCol - 1;
		while (p >= 0 && (line[p] == U' ' || line[p] == U'\t')) --p;
		if (p < 0) return {};

		const int end = p + 1;
		int start = p;

		while (start >= 0)
		{
			if (isIdentifierChar(line[start])) { --start; continue; }
			if (start >= 1 && line[start] == U':' && line[start - 1] == U':')
			{
				start -= 2;
				while (start >= 0 && (line[start] == U' ' || line[start] == U'\t'))
					--start;
				continue;
			}
			break;
		}

		if (start + 1 >= end) return {};
		return std::u32string(line.substr(start + 1, end - start - 1));
	}

	void CompletionEngine::sortItems(std::vector<CompletionItem>& items, std::u32string_view prefix)
	{
		std::stable_sort(items.begin(), items.end(), [prefix](const CompletionItem& a, const CompletionItem& b)
			{
				const int wa = kindWeight(a.kind);
				const int wb = kindWeight(b.kind);
				if (wa != wb) return wa > wb;

				if (a.label.size() != b.label.size())
					return a.label.size() < b.label.size();

				return a.label < b.label;
			});
	}

	CompletionResult CompletionEngine::complete(const TextBuffer& buf, const SymbolIndex& symbols, Position cursor, bool force) const
	{
		CompletionResult result;

		if (cursor.line < 0 || cursor.line >= buf.lineCount()) return result;
		const auto line = buf.line(cursor.line);
		if (cursor.col < 0 || cursor.col >(int)line.size()) return result;

		std::u32string prefix = extractPrefix(line, cursor.col);

		int qualifierEnd = -1;
		const int prefixStart = cursor.col - (int)prefix.size();
		Context ctx = detectContext(line, prefixStart, qualifierEnd);

		if (!force && ctx == Context::Global && prefix.size() < 2)
			return result;

		if (ctx == Context::Global)
		{
			for (const auto& kw : keywords())
			{
				if (!startsWith(kw, prefix)) continue;
				result.items.push_back({ kw, kw, {}, classifyKeyword(kw) });
			}

			auto all = symbols.allSymbols();
			for (const auto* s : all)
			{
				if (!startsWith(s->name, prefix)) continue;
				if (alreadyContains(result.items, s->name)) continue;
				result.items.push_back({
					s->name, s->name, s->typeName, toCompletionKind(s->kind)
					});
			}
		}

		else
		{
			std::u32string qualifier = extractQualifier(line, qualifierEnd);
			if (qualifier.empty()) return result;

			int scopeId = -1;

			if (ctx == Context::Scope)
			{
				scopeId = symbols.findScope(qualifier);
			}
			else
			{
				const Symbol* objSym = symbols.findSymbolAnywhere(qualifier);
				if (!objSym || objSym->typeName.empty()) return result;
				scopeId = symbols.findScope(objSym->typeName);
			}

			if (scopeId < 0) return result;

			for (const auto* s : symbols.symbolsInScope(scopeId))
			{
				if (!startsWith(s->name, prefix)) continue;
				if (alreadyContains(result.items, s->name)) continue;
				result.items.push_back(
				{
					s->name, s->name, s->typeName, toCompletionKind(s->kind)
				});
			}
		}

		sortItems(result.items, prefix);

		constexpr size_t kMaxItems = 64;
		if (result.items.size() > kMaxItems) result.items.resize(kMaxItems);

		result.replaceStart = prefixStart;
		result.replaceEnd = cursor.col;
		result.valid = !result.items.empty();
		return result;
	}

}
