#pragma once
#include "TextBuffer.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace Frostnux {

	enum class SymbolKind : uint8_t
	{
		Namespace, Class, Struct, Union, Enum, EnumValue,
		Function, Variable, Member, Typedef, Unknown
	};

	struct Symbol
	{
		std::u32string	name;
		std::u32string	typeName;
		SymbolKind		kind = SymbolKind::Unknown;
		int				scopeId = -1;
		int				line = 0;
		int				col = 0;
	};

	struct Scope
	{
		std::u32string	name;
		std::u32string	qualified;
		SymbolKind		kind = SymbolKind::Namespace;
		int				parent = -1;
		int				col = 0;
		std::unordered_map<std::u32string, int> children;
	};

	struct SymbolToken
	{
		enum class Kind : uint8_t { Ident, Punct, Number, String } kind;
		std::u32string text;
		int col = 0;
	};

	class SymbolIndex
	{
	public:
		void rebuild(const TextBuffer& buf);

		[[nodiscard]] int rootScope() const { return 0; }

		[[nodiscard]] int findScope(std::u32string_view qualified) const;

		[[nodiscard]] const Symbol* findSymbol(int scopeId, std::u32string_view name) const;

		[[nodiscard]] std::vector<const Symbol*> symbolsInScope(int scopeId) const;

		[[nodiscard]] std::vector<const Symbol*> allSymbols() const;

		[[nodiscard]] bool empty() const { return symbols_.empty(); }

		[[nodiscard]] const Symbol* findSymbolAnywhere(std::u32string_view name) const;
	private:
		struct ParseContext;

		void processTokens(const std::vector<SymbolToken>& toks, int line, ParseContext& ctx);
		void tryParseDeclaration(const std::vector<SymbolToken>& toks, int line, ParseContext& ctx);

		void pushPendingScope(ParseContext& ctx, SymbolKind kind, const std::u32string& name, int col);
		void registerSymbol(ParseContext& ctx, std::u32string name, SymbolKind kind, std::u32string typeName, int line, int col);

		std::vector<Scope>  scopes_;
		std::vector<Symbol> symbols_;
		std::unordered_map<int, std::vector<int>> scopeSymbols_;
	};

}
