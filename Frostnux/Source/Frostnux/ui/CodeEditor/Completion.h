#pragma once
#include "TextBuffer.h"
#include "SymbolIndex.h"
#include <vector>

namespace Frostnux {

	enum class CompletionKind : uint8_t
	{
		Keyword, Control, Type,
		Identifier, Function, Member,
		Namespace, Class, Struct, Enum, EnumValue
	};

	struct CompletionItem
	{
		std::u32string label;
		std::u32string insertText;
		std::u32string detail;
		CompletionKind kind = CompletionKind::Identifier;
	};

	struct CompletionResult
	{
		std::vector<CompletionItem> items;
		int  replaceStart = 0;
		int  replaceEnd = 0;
		bool valid = false;
	};

	class CompletionEngine
	{
	public:
		[[nodiscard]] CompletionResult complete( const TextBuffer& buf, const SymbolIndex& symbols, Position cursor, bool force = false) const;

		[[nodiscard]] static bool isIdentifierChar(char32_t c) noexcept;
	private:
		enum class Context { Global, Member, Scope };

		[[nodiscard]] static std::u32string extractPrefix(std::u32string_view line, int col);

		[[nodiscard]] static std::u32string extractQualifier(std::u32string_view line, int endCol);

		[[nodiscard]] static Context detectContext(std::u32string_view line, int prefixStart, int& qualifierEnd);

		[[nodiscard]] static const std::vector<std::u32string>& keywords();

		static void sortItems(std::vector<CompletionItem>& items, std::u32string_view prefix);
	};

}
