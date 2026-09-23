#include "fxpch.h"
#include "TextBuffer.h"

namespace Frostnux {

	char32_t TextBuffer::at(Position p) const
	{
		if (p.line < 0 || p.line >= lineCount()) return 0;
		auto l = line(p.line);
		if (p.col < 0 || p.col >= (int)l.size()) return 0;
		return l[p.col];
	}

	Position TextBuffer::clamp(Position p) const
	{
		p.line = std::clamp(p.line, 0, lineCount() - 1);
		int len = (int)lines_[p.line].size();
		p.col = std::clamp(p.col, 0, len);
		return p;
	}

	Position TextBuffer::advance(Position p, std::u32string_view text) const
	{
		p = clamp(p);
		for (char32_t c : text)
		{
			if (c == U'\n') [[unlikely]] { p.line++; p.col = 0; }
			else { p.col++; }
		}
		return clamp(p);
	}

	std::u32string TextBuffer::getText(Position from, Position to) const
	{
		from = clamp(from); to = clamp(to);
		if (to < from) std::swap(from, to);
		std::u32string out;
		if (from.line == to.line)
		{
			return std::u32string(line(from.line).substr(from.col, to.col - from.col));
		}
		out.reserve(64);
		out.append(line(from.line).substr(from.col));
		out += U'\n';
		for (int i = from.line + 1; i < to.line; ++i)
		{
			out.append(line(i));
			out += U'\n';
		}
		out.append(line(to.line).substr(0, to.col));
		return out;
	}

	void TextBuffer::insert(Position p, std::u32string_view text)
	{
		if (text.empty()) return;
		p = clamp(p);

		auto firstNl = text.find(U'\n');
		if (firstNl == std::u32string_view::npos) [[likely]]
		{
			lines_[p.line].insert(p.col, text);
			return;
		}

		Line head = lines_[p.line].substr(0, p.col);
		Line tail = lines_[p.line].substr(p.col);

		std::vector<Line> parts;
		size_t start = 0;
		while (true)
		{
			size_t nl = text.find(U'\n', start);
			if (nl == std::u32string_view::npos)
			{
				parts.emplace_back(text.substr(start));
				break;
			}
			parts.emplace_back(text.substr(start, nl - start));
			start = nl + 1;
		}

		lines_[p.line] = head + parts.front();

		for (size_t i = 1; i + 1 < parts.size(); ++i)
		{
			const int offset = p.line + static_cast<int>(i);
			lines_.insert(lines_.begin() + offset, std::move(parts[i]));
		}

		const int lastOffset = p.line + static_cast<int>(parts.size()) - 1;
		lines_.insert(lines_.begin() + lastOffset, parts.back() + tail);
	}

	void TextBuffer::erase(Position from, Position to)
	{
		from = clamp(from); to = clamp(to);
		if (to < from) std::swap(from, to);
		if (from == to) return;

		if (from.line == to.line) [[likely]]
		{
			lines_[from.line].erase(from.col, to.col - from.col);
			return;
		}

		Line head = lines_[from.line].substr(0, from.col);
		Line tail = lines_[to.line].substr(to.col);
		lines_[from.line] = head + tail;

		const int firstIdx = from.line + 1;
		const int lastIdx = to.line + 1;
		lines_.erase(lines_.begin() + firstIdx, lines_.begin() + lastIdx);
	}

	void TextBuffer::setText(std::u32string_view text)
	{
		lines_.clear();
		lines_.emplace_back();
		insert(Position{ 0, 0 }, text);
	}

	Position TextBuffer::backspaceAt(Position p, int& outDeleted)
	{
		p = clamp(p);
		outDeleted = 0;
		if (p.col > 0) [[likely]]
		{
			p.col--;
			outDeleted = 1;
			lines_[p.line].erase(p.col, 1);
			return p;
		}
		if (p.line == 0) return p;
		int prevLen = (int)lines_[p.line - 1].size();
		lines_[p.line - 1] += lines_[p.line];
		lines_.erase(lines_.begin() + p.line);
		p.line--;
		p.col = prevLen;
		outDeleted = 1;
		return p;
	}

	bool TextBuffer::deleteAt(Position p)
	{
		p = clamp(p);
		Line& l = lines_[p.line];
		if (p.col < (int)l.size())
		{
			l.erase(p.col, 1);
			return true;
		}
		if (p.line + 1 >= lineCount()) return false;
		l += lines_[p.line + 1];
		lines_.erase(lines_.begin() + p.line + 1);
		return true;
	}

}
