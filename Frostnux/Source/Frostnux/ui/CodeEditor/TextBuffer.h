#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <compare>

namespace Frostnux {

	struct Position
	{
		int line = 0;
		int col = 0;
		auto operator<=>(const Position&) const = default;
	};

	struct Color
	{
		float r = 1, g = 1, b = 1, a = 1;
	};

	struct Rect
	{
		float x = 0, y = 0, w = 0, h = 0;
		[[nodiscard]] bool contains(float px, float py) const noexcept
		{
			return px >= x && px <= x + w && py >= y && py <= y + h;
		}
	};

	[[nodiscard]] inline std::u32string utf8_to_u32(std::string_view s)
	{
		std::u32string out;
		out.reserve(s.size());
		size_t i = 0;
		while (i < s.size())
		{
			uint8_t c = (uint8_t)s[i];
			uint32_t cp = 0;
			int extra = 0;
			if ((c & 0x80) == 0x00) { cp = c;        extra = 0; }
			else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; }
			else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; }
			else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; extra = 3; }
			else { ++i; continue; }
			++i;

			for (int k = 0; k < extra && i < s.size(); ++k, ++i)
			{
				uint8_t cc = (uint8_t)s[i];
				if ((cc & 0xC0) != 0x80) break;
				cp = (cp << 6) | (cc & 0x3F);
			}
			out.push_back((char32_t)cp);
		}
		return out;
	}

	[[nodiscard]] inline std::string u32_to_utf8(std::u32string_view s)
	{
		std::string out;
		out.reserve(s.size() * 4);
		for (char32_t cp : s)
		{
			uint32_t c = (uint32_t)cp;
			if (c < 0x80) out += (char)c;
			else if (c < 0x800)
			{
				out += (char)(0xC0 | (c >> 6));
				out += (char)(0x80 | (c & 0x3F));
			}
			else if (c < 0x10000)
			{
				out += (char)(0xE0 | (c >> 12));
				out += (char)(0x80 | ((c >> 6) & 0x3F));
				out += (char)(0x80 | (c & 0x3F));
			}
			else
			{
				out += (char)(0xF0 | (c >> 18));
				out += (char)(0x80 | ((c >> 12) & 0x3F));
				out += (char)(0x80 | ((c >> 6) & 0x3F));
				out += (char)(0x80 | (c & 0x3F));
			}
		}
		return out;
	}
	
	[[nodiscard]] inline bool isWordChar(char32_t c) noexcept
	{
		return (c >= U'a' && c <= U'z')
			|| (c >= U'A' && c <= U'Z')
			|| (c >= U'0' && c <= U'9')
			|| c == U'_'
			|| c > 127;
	}

	using Line = std::u32string;

	class TextBuffer
	{
	public:
		TextBuffer() { lines_.emplace_back(); }

		[[nodiscard]] int					lineCount() const noexcept { return (int)lines_.size(); }
		[[nodiscard]] std::u32string_view	line(int i) const noexcept { return lines_[i]; }

		[[nodiscard]] char32_t		 at(Position p) const;
		[[nodiscard]] Position		 clamp(Position p) const;
		[[nodiscard]] Position		 advance(Position p, std::u32string_view text) const;
		[[nodiscard]] std::u32string getText(Position from, Position to) const;

		void insert(Position p, std::u32string_view text);
		void erase(Position from, Position to);
		void setText(std::u32string_view text);

		Position backspaceAt(Position p, int& outDeleted);
		bool	 deleteAt(Position p);

	private:
		std::vector<Line> lines_;
	};

}
