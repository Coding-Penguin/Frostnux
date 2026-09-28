#pragma once
#include "TextBuffer.h"
#include <vector>
#include <string>
#include <string_view>

namespace Frostnux {

	enum class CompletionKind : uint8_t
	{
		Keyword, Type, Control, Identifier, Function, Macro
	};

	struct CompletionItem
	{
		std::u32string  label;
		std::u32string  insertText;
		CompletionKind  kind = CompletionKind::Identifier;
	};

	class CompletionEngine
	{
	public:
		void rebuildIndex(const TextBuffer& buf);

		[[nodiscard]] std::vector<CompletionItem>
			query(std::u32string_view prefix, size_t limit = 32) const;

		[[nodiscard]] static std::u32string
			extractPrefix(const TextBuffer& buf, Position cursor);

		[[nodiscard]] static bool isIdentifierChar(char32_t c) noexcept;

	private:
		std::vector<std::u32string> m_Words;
	};

}
