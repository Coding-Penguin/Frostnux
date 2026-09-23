#include "fxpch.h"
#include "Highlighter.h"

namespace Frostnux {

	[[nodiscard]] constexpr bool isControlKw(std::u32string_view w) noexcept
	{
		return w == U"if" || w == U"else" || w == U"for" || w == U"while"
			|| w == U"do" || w == U"switch" || w == U"case" || w == U"default"
			|| w == U"break" || w == U"continue" || w == U"return" || w == U"goto"
			|| w == U"try" || w == U"catch" || w == U"throw"
			|| w == U"co_await" || w == U"co_return" || w == U"co_yield";
	}

	[[nodiscard]] constexpr bool isTypeKw(std::u32string_view w) noexcept
	{
		return w == U"int" || w == U"long" || w == U"short" || w == U"char"
			|| w == U"bool" || w == U"float" || w == U"double" || w == U"void"
			|| w == U"wchar_t" || w == U"char8_t" || w == U"char16_t" || w == U"char32_t"
			|| w == U"signed" || w == U"unsigned" || w == U"size_t" || w == U"ptrdiff_t"
			|| w == U"nullptr_t" || w == U"string" || w == U"vector" || w == U"map"
			|| w == U"unordered_map" || w == U"set" || w == U"unordered_set"
			|| w == U"pair" || w == U"tuple" || w == U"array"
			|| w == U"shared_ptr" || w == U"unique_ptr" || w == U"weak_ptr"
			|| w == U"optional" || w == U"variant" || w == U"string_view"
			|| w == U"span" || w == U"function" || w == U"atomic"
			|| w == U"thread" || w == U"mutex";
	}

	[[nodiscard]] constexpr bool isKeywordKw(std::u32string_view w) noexcept
	{
		return w == U"class" || w == U"struct" || w == U"union" || w == U"enum"
			|| w == U"namespace" || w == U"template" || w == U"typename" || w == U"using"
			|| w == U"public" || w == U"private" || w == U"protected" || w == U"virtual"
			|| w == U"override" || w == U"final" || w == U"static" || w == U"extern"
			|| w == U"inline" || w == U"constexpr" || w == U"consteval" || w == U"constinit"
			|| w == U"const" || w == U"volatile" || w == U"mutable" || w == U"friend"
			|| w == U"explicit" || w == U"operator" || w == U"new" || w == U"delete"
			|| w == U"this" || w == U"sizeof" || w == U"alignof" || w == U"decltype"
			|| w == U"noexcept" || w == U"static_assert" || w == U"concept"
			|| w == U"requires" || w == U"auto";
	}

	[[nodiscard]] constexpr bool isMacroKw(std::u32string_view w) noexcept
	{
		return w == U"NULL" || w == U"true" || w == U"false"
			|| w == U"TRUE" || w == U"FALSE" || w == U"EOF";
	}

	[[nodiscard]] constexpr bool isDigit(char32_t c) noexcept { return c >= U'0' && c <= U'9'; }
	[[nodiscard]] constexpr bool isAlpha(char32_t c) noexcept
	{
		return (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z');
	}
	[[nodiscard]] constexpr bool isIdentStart(char32_t c) noexcept
	{
		return c == U'_' || isAlpha(c) || c > 127;
	}
	[[nodiscard]] constexpr bool isIdentChar(char32_t c) noexcept
	{
		return isIdentStart(c) || isDigit(c);
	}
	[[nodiscard]] constexpr bool isOpChar(char32_t c) noexcept
	{
		constexpr std::u32string_view ops = U"+-*/%=<>!&|^~?:;,.()[]{}";
		for (char32_t o : ops) if (o == c) return true;
		return false;
	}

	void Highlighter::update(const TextBuffer& buf)
	{
		int n = buf.lineCount();
		if ((int)tokens_.size() < n) tokens_.resize(n);
		if ((int)states_.size() < n) states_.resize(n);

		if (dirtyFrom_ == INT32_MAX) return;
		if (dirtyFrom_ >= n) [[unlikely]] { dirtyFrom_ = INT32_MAX; return; }

		int i = dirtyFrom_;
		LineState st;
		if (i > 0) st = states_[i - 1];

		while (i < n)
		{
			LineState newEnd;
			tokenizeLine(buf.line(i), st, tokens_[i], newEnd);

			bool same = (states_[i] == newEnd);
			states_[i] = newEnd;
			st = newEnd;

			if (same) [[unlikely]] break;
			++i;
		}
		dirtyFrom_ = INT32_MAX;
	}

	void Highlighter::tokenizeLine(std::u32string_view s, LineState st, std::vector<Token>& out, LineState& end)
	{
		out.clear();
		const int n = (int)s.size();
		int i = 0;

		auto push = [&](int a, int b, TokenType t)
			{
			if (b > a) out.push_back({ a, b, t });
			};

		if (st.mode == LineState::Mode::BlockComment) [[unlikely]]
		{
			int p = 0;
			while (p + 1 < n && !(s[p] == U'*' && s[p + 1] == U'/')) ++p;
			if (p + 1 < n)
			{
				push(0, p + 2, TokenType::Comment);
				i = p + 2;
				st.mode = LineState::Mode::Normal;
			}
			else
			{
				push(0, n, TokenType::Comment);
				end = st; return;
			}
		}

		else if (st.mode == LineState::Mode::RawString) [[unlikely]]
		{
			std::string closer = ")" + st.rawDelim + "\"";
			std::u32string cu(closer.begin(), closer.end());
			auto pos = s.find(std::u32string_view(cu));
			if (pos != std::u32string_view::npos)
			{
				push(0, (int)(pos + cu.size()), TokenType::String);
				i = (int)(pos + cu.size());
				st.mode = LineState::Mode::Normal;
				st.rawDelim.clear();
			}
			else
			{
				push(0, n, TokenType::String);
				end = st; return;
			}
		}

		while (i < n)
		{
			char32_t c = s[i];

			if (c == U'#') [[unlikely]]
			{
				int k = 0;
				while (k < i && (s[k] == U' ' || s[k] == U'\t')) ++k;
				if (k == i) { push(i, n, TokenType::Preprocessor); break; }
			}

			if (c == U'/' && i + 1 < n && s[i + 1] == U'/') [[unlikely]]
			{
				push(i, n, TokenType::Comment); break;
			}

			if (c == U'/' && i + 1 < n && s[i + 1] == U'*') [[unlikely]]
			{
				int p = i + 2;
				while (p + 1 < n && !(s[p] == U'*' && s[p + 1] == U'/')) ++p;
				if (p + 1 < n) { push(i, p + 2, TokenType::Comment); i = p + 2; }
				else
				{
					push(i, n, TokenType::Comment);
					st.mode = LineState::Mode::BlockComment;
					end = st; return;
				}
				continue;
			}

			if (c == U'R' && i + 1 < n && s[i + 1] == U'"') [[unlikely]]
			{
				int p = i + 2; std::string delim;
				while (p < n && s[p] != U'(' && delim.size() < 16) delim += (char)s[p++];
				if (p < n && s[p] == U'(')
				{
					++p;
					std::string closer = ")" + delim + "\"";
					std::u32string cu(closer.begin(), closer.end());
					auto pos = s.find(std::u32string_view(cu), p);
					if (pos != std::u32string_view::npos)
					{
						push(i, (int)(pos + cu.size()), TokenType::String);
						i = (int)(pos + cu.size());
					}
					else
					{
						push(i, n, TokenType::String);
						st.mode = LineState::Mode::RawString;
						st.rawDelim = delim;
						end = st; return;
					}
					continue;
				}
			}

			if (c == U'"' || c == U'\'')
			{
				char32_t q = c; int p = i + 1;
				while (p < n)
				{
					if (s[p] == U'\\' && p + 1 < n) { p += 2; continue; }
					if (s[p] == q) { ++p; break; }
					++p;
				}
				push(i, p, q == U'"' ? TokenType::String : TokenType::Char);
				i = p; continue;
			}

			if (isDigit(c) || (c == U'.' && i + 1 < n && isDigit(s[i + 1])))
			{
				int p = i;
				while (p < n && (isIdentChar(s[p]) || s[p] == U'.' || s[p] == U'\'')) ++p;
				push(i, p, TokenType::Number); i = p; continue;
			}

			if (isIdentStart(c)) [[likely]]
			{
				int p = i;
				while (p < n && isIdentChar(s[p])) ++p;
				std::u32string_view w = s.substr(i, p - i);

				TokenType t = TokenType::Identifier;
				if (isControlKw(w)) t = TokenType::Control;
				else if (isKeywordKw(w)) t = TokenType::Keyword;
				else if (isTypeKw(w))    t = TokenType::Type;
				else if (isMacroKw(w))   t = TokenType::Macro;
				else
				{
					int q = p;
					while (q < n && (s[q] == U' ' || s[q] == U'\t')) ++q;
					if (q < n && s[q] == U'(') t = TokenType::Function;
				}
				push(i, p, t); i = p; continue;
			}

			if (isOpChar(c))
			{
				int p = i;
				while (p < n && isOpChar(s[p])) ++p;
				push(i, p, TokenType::Operator); i = p; continue;
			}

			++i;
		}

		end = LineState{};
	}

}
