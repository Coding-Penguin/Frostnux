#include "fxpch.h"
#include "SymbolIndex.h"

namespace Frostnux {

	namespace {

		[[nodiscard]] bool isIdentStart(char32_t c)
		{
			return (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z')
				|| c == U'_' || c > 127;
		}
		[[nodiscard]] bool isIdentChar(char32_t c)
		{
			return isIdentStart(c) || (c >= U'0' && c <= U'9');
		}

		std::vector<SymbolToken> tokenizeLine(std::u32string_view line)
		{
			std::vector<SymbolToken> out;
			const int n = (int)line.size();
			int i = 0;
			while (i < n)
			{
				char32_t c = line[i];
				if (c == U' ' || c == U'\t' || c == U'\r') { ++i; continue; }

				if (c == U'/' && i + 1 < n && line[i + 1] == U'/') break;

				if (c == U'/' && i + 1 < n && line[i + 1] == U'*')
				{
					i += 2;
					while (i + 1 < n && !(line[i] == U'*' && line[i + 1] == U'/')) ++i;
					i = (i + 1 < n) ? i + 2 : n;
					continue;
				}

				if (c == U'"' || c == U'\'')
				{
					char32_t q = c;
					int s = i++;
					while (i < n)
					{
						if (line[i] == U'\\' && i + 1 < n) { i += 2; continue; }
						if (line[i] == q) { ++i; break; }
						++i;
					}
					out.push_back({ SymbolToken::Kind::String, {} });
					continue;
				}

				if (isIdentStart(c))
				{
					int s = i;
					while (i < n && isIdentChar(line[i])) ++i;
					out.push_back({ SymbolToken::Kind::Ident, std::u32string(line.substr(s, i - s)) });
					continue;
				}

				if (c >= U'0' && c <= U'9')
				{
					while (i < n && (isIdentChar(line[i]) || line[i] == U'.')) ++i;
					out.push_back({ SymbolToken::Kind::Number, {} });
					continue;
				}

				out.push_back({ SymbolToken::Kind::Punct, std::u32string(1, c) });
				++i;
			}
			return out;
		}

		[[nodiscard]] bool isTypeModifier(const std::u32string& w)
		{
			static const std::unordered_set<std::u32string> s =
			{
				U"const", U"volatile", U"static", U"extern", U"inline",
				U"virtual", U"friend", U"explicit", U"constexpr", U"consteval",
				U"constinit", U"mutable", U"register", U"thread_local",
				U"typename", U"using", U"typedef", U"template"
			};
			return s.count(w) > 0;
		}

		[[nodiscard]] bool isControlKeyword(const std::u32string& w)
		{
			static const std::unordered_set<std::u32string> s =
			{
				U"if", U"else", U"for", U"while", U"do", U"switch",
				U"case", U"default", U"break", U"continue", U"return",
				U"goto", U"try", U"catch", U"throw", U"new", U"delete",
				U"sizeof", U"alignof", U"decltype", U"static_assert",
				U"this", U"operator"
			};
			return s.count(w) > 0;
		}

	}

	struct SymbolIndex::ParseContext
	{
		std::vector<int>	scopeStack = { 0 };
		std::vector<bool>	braceIsScope;
		bool				pendingScope = false;
	};

	void SymbolIndex::rebuild(const TextBuffer& buf)
	{
		scopes_.clear();
		symbols_.clear();
		scopeSymbols_.clear();

		scopes_.emplace_back();
		scopes_[0].parent = -1;
		scopes_[0].kind = SymbolKind::Namespace;

		ParseContext ctx;

		const int n = buf.lineCount();
		for (int i = 0; i < n; ++i)
		{
			auto toks = tokenizeLine(buf.line(i));
			if (!toks.empty())
				processTokens(toks, i, ctx);
		}
	}

	void SymbolIndex::processTokens(const std::vector<SymbolToken>& toks, int line, ParseContext& ctx)
	{
		const int n = (int)toks.size();
		int i = 0;

		while (i < n)
		{
			const auto& t = toks[i];

			if (t.kind == SymbolToken::Kind::Punct)
			{
				if (t.text == U"{")
				{
					ctx.braceIsScope.push_back(ctx.pendingScope);
					ctx.pendingScope = false;
					++i;
					continue;
				}
				if (t.text == U"}")
				{
					if (!ctx.braceIsScope.empty())
					{
						if (ctx.braceIsScope.back() && ctx.scopeStack.size() > 1)
							ctx.scopeStack.pop_back();
						ctx.braceIsScope.pop_back();
					}
					++i;
					continue;
				}
				++i;
				continue;
			}

			if (t.kind != SymbolToken::Kind::Ident) { ++i; continue; }
			const auto& w = t.text;

			if (w == U"namespace")
			{
				if (i + 1 < n && toks[i + 1].kind == SymbolToken::Kind::Ident)
				{
					int braceIdx = -1;
					for (int j = i + 2; j < n; ++j)
					{
						if (toks[j].kind != SymbolToken::Kind::Punct) continue;
						if (toks[j].text == U"{") { braceIdx = j; break; }
						if (toks[j].text == U"=") { braceIdx = -1; break; }
					}
					if (braceIdx >= 0)
					{
						pushPendingScope(ctx, SymbolKind::Namespace, toks[i + 1].text);
						i = braceIdx;
						continue;
					}
					i += 2;
					continue;
				}
				++i;
				continue;
			}

			if (w == U"class" || w == U"struct" || w == U"union")
			{
				SymbolKind k = (w == U"class") ? SymbolKind::Class
					: (w == U"struct") ? SymbolKind::Struct
					: SymbolKind::Union;
				if (i + 1 < n && toks[i + 1].kind == SymbolToken::Kind::Ident)
				{
					int braceIdx = -1;
					for (int j = i + 2; j < n; ++j)
					{
						if (toks[j].kind != SymbolToken::Kind::Punct) continue;
						if (toks[j].text == U"{") { braceIdx = j; break; }
						if (toks[j].text == U";") { braceIdx = -2; break; }
					}
					if (braceIdx >= 0)
					{
						pushPendingScope(ctx, k, toks[i + 1].text);
						i = braceIdx;
						continue;
					}
					if (braceIdx == -2)
					{
						registerSymbol(ctx, toks[i + 1].text, k, {}, line);
					}
					i += 2;
					continue;
				}
				++i;
				continue;
			}

			if (w == U"enum")
			{
				int nameIdx = i + 1;
				if (nameIdx < n && (toks[nameIdx].text == U"class" || toks[nameIdx].text == U"struct")) 
					++nameIdx;
				if (nameIdx < n && toks[nameIdx].kind == SymbolToken::Kind::Ident)
				{
					int braceIdx = -1;
					for (int j = nameIdx + 1; j < n; ++j)
					{
						if (toks[j].kind == SymbolToken::Kind::Punct && toks[j].text == U"{")
						{
							braceIdx = j; break;
						}
					}
					if (braceIdx >= 0)
					{
						pushPendingScope(ctx, SymbolKind::Enum, toks[nameIdx].text);

						for (int j = braceIdx + 1; j < n; ++j)
						{
							if (toks[j].kind == SymbolToken::Kind::Ident
								&& !isTypeModifier(toks[j].text))
								registerSymbol(ctx, toks[j].text,
									SymbolKind::EnumValue, {}, line);
						}
						i = braceIdx + 1;
						continue;
					}
				}
			}

			++i;
		}

		if (!ctx.scopeStack.empty())
		{
			const int sid = ctx.scopeStack.back();
			const auto k = scopes_[sid].kind;
			if (k == SymbolKind::Namespace || k == SymbolKind::Class
				|| k == SymbolKind::Struct || k == SymbolKind::Union)
			{
				tryParseDeclaration(toks, line, ctx);
			}
		}
	}

	void SymbolIndex::tryParseDeclaration(const std::vector<SymbolToken>& toks, int line, ParseContext& ctx)
	{
		const int n = (int)toks.size();
		if (n < 2) return;

		int parenIdx = -1;
		for (int i = 0; i < n; ++i)
		{
			if (toks[i].kind == SymbolToken::Kind::Punct && toks[i].text == U"(")
			{
				parenIdx = i; break;
			}
			if (toks[i].kind == SymbolToken::Kind::Punct && toks[i].text == U";")
				break;
		}

		if (parenIdx >= 2)
		{
			int nameIdx = -1;
			for (int i = parenIdx - 1; i >= 0; --i)
			{
				if (toks[i].kind != SymbolToken::Kind::Ident) continue;
				if (isTypeModifier(toks[i].text) || isControlKeyword(toks[i].text))
					continue;
				nameIdx = i;
				break;
			}
			if (nameIdx >= 1)
			{
				int retIdx = -1;
				for (int i = nameIdx - 1; i >= 0; --i)
				{
					if (toks[i].kind != SymbolToken::Kind::Ident) continue;
					if (isTypeModifier(toks[i].text)) continue;
					retIdx = i;
					break;
				}
				if (retIdx >= 0)
				{
					registerSymbol(ctx, toks[nameIdx].text,
						SymbolKind::Function, toks[retIdx].text, line);
				}
			}
			return;
		}

		if (toks.back().kind != SymbolToken::Kind::Punct || toks.back().text != U";")
			return;

		int nameIdx = -1;
		for (int i = n - 2; i >= 0; --i)
		{
			if (toks[i].kind == SymbolToken::Kind::Punct)
			{
				if (toks[i].text == U"=") continue;
				if (toks[i].text == U",") break;
				if (toks[i].text == U")" || toks[i].text == U"(") return;
				if (toks[i].text == U">") return;
				continue;
			}
			if (toks[i].kind != SymbolToken::Kind::Ident) continue;
			if (isTypeModifier(toks[i].text) || isControlKeyword(toks[i].text))
				continue;
			nameIdx = i;
			break;
		}
		if (nameIdx < 1) return;

		int typeIdx = -1;
		for (int i = nameIdx - 1; i >= 0; --i)
		{
			if (toks[i].kind != SymbolToken::Kind::Ident) continue;
			if (isTypeModifier(toks[i].text)) continue;
			typeIdx = i;
			break;
		}
		if (typeIdx < 0) return;

		registerSymbol(ctx, toks[nameIdx].text,
			SymbolKind::Variable, toks[typeIdx].text, line);
	}

	void SymbolIndex::pushPendingScope(ParseContext& ctx, SymbolKind kind, const std::u32string& name)
	{
		const int parent = ctx.scopeStack.back();

		auto it = scopes_[parent].children.find(name);
		int id;
		if (it != scopes_[parent].children.end())
		{
			id = it->second;
		}
		else
		{
			id = (int)scopes_.size();
			Scope s;
			s.name = name;
			s.kind = kind;
			s.parent = parent;
			s.qualified = scopes_[parent].qualified.empty()
				? name
				: scopes_[parent].qualified + U"::" + name;
			scopes_.push_back(std::move(s));
			scopes_[parent].children[name] = id;
		}

		ctx.scopeStack.push_back(id);
		ctx.pendingScope = true;
	}

	void SymbolIndex::registerSymbol(ParseContext& ctx, std::u32string name,
		SymbolKind kind, std::u32string typeName, int line)
	{
		if (name.empty()) return;
		const int sid = ctx.scopeStack.back();

		auto& idxs = scopeSymbols_[sid];
		for (int idx : idxs)
		{
			if (symbols_[idx].name == name && symbols_[idx].kind == kind)
				return;
		}

		Symbol s;
		s.name = std::move(name);
		s.typeName = std::move(typeName);
		s.kind = kind;
		s.scopeId = sid;
		s.line = line;

		symbols_.push_back(std::move(s));
		idxs.push_back((int)symbols_.size() - 1);
	}

	int SymbolIndex::findScope(std::u32string_view qualified) const
	{
		std::u32string q(qualified);
		while (!q.empty() && q.back() == U':') q.pop_back();
		if (q.empty()) return 0;

		std::vector<std::u32string> parts;
		std::u32string cur;
		for (char32_t c : q)
		{
			if (c == U':')
			{
				if (!cur.empty()) { parts.push_back(std::move(cur)); cur.clear(); }
			}
			else cur.push_back(c);
		}
		if (!cur.empty()) parts.push_back(std::move(cur));

		int s = 0;
		for (const auto& p : parts)
		{
			auto it = scopes_[s].children.find(p);
			if (it == scopes_[s].children.end()) return -1;
			s = it->second;
		}
		return s;
	}

	const Symbol* SymbolIndex::findSymbol(int scopeId, std::u32string_view name) const
	{
		while (scopeId >= 0)
		{
			auto it = scopeSymbols_.find(scopeId);
			if (it != scopeSymbols_.end())
			{
				for (int idx : it->second)
				{
					if (symbols_[idx].name == name)
						return &symbols_[idx];
				}
			}
			scopeId = scopes_[scopeId].parent;
		}
		return nullptr;
	}

	std::vector<const Symbol*> SymbolIndex::symbolsInScope(int scopeId) const
	{
		std::vector<const Symbol*> out;
		auto it = scopeSymbols_.find(scopeId);
		if (it == scopeSymbols_.end()) return out;
		for (int idx : it->second) out.push_back(&symbols_[idx]);
		return out;
	}

	std::vector<const Symbol*> SymbolIndex::allSymbols() const
	{
		std::vector<const Symbol*> out;
		out.reserve(symbols_.size());
		for (const auto& s : symbols_) out.push_back(&s);
		return out;
	}

	const Symbol* SymbolIndex::findSymbolAnywhere(std::u32string_view name) const
	{
		for (const auto& s : symbols_)
			if (s.name == name) return &s;
		return nullptr;
	}

}
