#pragma once
#include "TextBuffer.h"
#include <vector>
#include <string>
#include <string_view>

namespace Frostnux {

	enum class TokenType : uint8_t
	{
		Default, Keyword, Control, Type, Identifier, Function,
		Number, String, Char, Comment, Preprocessor, Macro, Operator
	};

	struct Token
	{
		int start, end;
		TokenType type;
	};

	struct EditorTheme
	{
		Color bg{ 0.118f, 0.118f, 0.118f, 1 };
		Color text{ 0.86f, 0.86f, 0.86f, 1 };
		Color control{ 0.78f, 0.44f, 0.85f, 1 };
		Color keyword{ 0.34f, 0.61f, 1.00f, 1 };
		Color type{ 0.30f, 0.79f, 0.78f, 1 };
		Color function{ 0.86f, 0.86f, 0.66f, 1 };
		Color number{ 0.71f, 0.80f, 0.66f, 1 };
		Color string{ 0.81f, 0.43f, 0.36f, 1 };
		Color comment{ 0.34f, 0.62f, 0.34f, 1 };
		Color preproc{ 0.61f, 0.50f, 0.40f, 1 };
		Color macro{ 0.74f, 0.60f, 0.53f, 1 };
		Color cursor{ 0.90f, 0.90f, 0.90f, 1 };
		Color selection{ 0.26f, 0.40f, 0.62f, 1 };
		Color currentLine{ 1.0f, 1.0f, 1.0f, 0.05f };
		Color gutterText{ 0.40f, 0.40f, 0.42f, 1 };
		Color gutterBg{ 0.14f, 0.14f, 0.14f, 1 };
		Color tabBg{ 0.16f, 0.16f, 0.16f, 1 };
		Color tabActive{ 0.24f, 0.24f, 0.26f, 1 };
		Color scrollTrack{ 0.14f, 0.14f, 0.14f, 1 };
		Color scrollThumb{ 0.30f, 0.30f, 0.32f, 1 };

		[[nodiscard]] Color of(TokenType t) const noexcept
		{
			switch (t)
			{
			case TokenType::Control:      return control;
			case TokenType::Keyword:      return keyword;
			case TokenType::Type:         return type;
			case TokenType::Function:     return function;
			case TokenType::Number:       return number;
			case TokenType::String:
			case TokenType::Char:         return string;
			case TokenType::Comment:      return comment;
			case TokenType::Preprocessor: return preproc;
			case TokenType::Macro:        return macro;
			default:                      return text;
			}
		}
	};

	class Highlighter
	{
	public:
		void markDirty(int fromLine, int /*toLine*/) noexcept
		{
			if (fromLine < 0) fromLine = 0;
			dirtyFrom_ = std::min(dirtyFrom_, fromLine);
		}

		void update(const TextBuffer& buf);

		[[nodiscard]] const std::vector<Token>& tokens(int line) const noexcept
		{
			static const std::vector<Token> empty;
			if (line < 0 || line >= (int)tokens_.size()) return empty;
			return tokens_[line];
		}
	private:
		struct LineState
		{
			enum class Mode : uint8_t { Normal, BlockComment, RawString };
			Mode mode = Mode::Normal;
			std::string rawDelim;
			bool operator==(const LineState&) const = default;
		};

		void tokenizeLine(std::u32string_view s, LineState st,
			std::vector<Token>& out, LineState& end);

		std::vector<std::vector<Token>> tokens_;
		std::vector<LineState>          states_;
		int dirtyFrom_ = INT32_MAX;
	};

}
