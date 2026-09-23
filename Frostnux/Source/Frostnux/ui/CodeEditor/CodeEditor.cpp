#include "fxpch.h"
#include "CodeEditor.h"
#include "TextBuffer.h"

namespace Frostnux {

	Tab& CodeEditor::addTab(const std::string& path)
	{
		auto t = std::make_unique<Tab>();
		if (!path.empty())
		{
			t->path = path;
			const size_t slash = path.find_last_of("/\\");
			const std::string base =
				(slash == std::string::npos) ? path : path.substr(slash + 1);
			t->title = utf8_to_u32(base);
		}
		Tab& ref = *t;
		m_Tabs.push_back(std::move(t));
		m_Active = static_cast<int>(m_Tabs.size()) - 1;
		return ref;
	}

	void CodeEditor::closeTab(int idx)
	{
		if (idx < 0 || idx >= static_cast<int>(m_Tabs.size())) return;
		m_Tabs.erase(m_Tabs.begin() + idx);
		if (m_Tabs.empty()) m_Active = -1;
		else m_Active = std::min(m_Active, static_cast<int>(m_Tabs.size()) - 1);
	}

	void CodeEditor::switchTab(int dir)
	{
		if (m_Tabs.empty()) return;
		const int n = static_cast<int>(m_Tabs.size());
		m_Active = ((m_Active + dir) % n + n) % n;
	}

	Tab* CodeEditor::activeTab()
	{
		if (m_Active < 0 || m_Active >= static_cast<int>(m_Tabs.size())) return nullptr;
		return m_Tabs[m_Active].get();
	}

	bool CodeEditor::OnEvent(Event& e)
	{
		EventDispatcher d(e);

		d.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) { return onKeyPressed(ev); });
		d.Dispatch<CharEvent>([this](CharEvent& ev) { return onChar(ev); });
		d.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& ev) { return onMouseButtonPressed(ev); });
		d.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& ev) { return onMouseButtonReleased(ev); });
		d.Dispatch<MouseMovedEvent>([this](MouseMovedEvent& ev) { return onMouseMoved(ev); });
		d.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& ev) { return onMouseScrolled(ev); });

		return e.m_Handled;
	}

	bool CodeEditor::onKeyPressed(KeyPressedEvent& e)
	{
		Tab* tp = activeTab();
		if (!tp) return false;
		Tab& t = *tp;

		const int  key = e.GetKeyCode();
		const int  mods = e.GetMods();
		const bool ctrl = (mods & FX_KEY_CONTROL) != 0;
		const bool shift = (mods & FX_KEY_SHIFT) != 0;

		if (ctrl)
		{
			switch (key)
			{
			case FX_KEY_Z:  if (shift) doRedo(t); else doUndo(t); return true;
			case FX_KEY_Y:  doRedo(t);                            return true;
			case FX_KEY_A:
			{
				Position s{ 0, 0 };
				Position en{ t.buffer.lineCount() - 1, static_cast<int>(t.buffer.line(t.buffer.lineCount() - 1).size()) };
				t.selection.set(s, en);
				return true;
			}
			case FX_KEY_HOME:  moveHome(t, shift, true);  return true;
			case FX_KEY_END:   moveEnd(t, shift, true);  return true;
			case FX_KEY_LEFT:  moveLeft(t, shift, true);  return true;
			case FX_KEY_RIGHT: moveRight(t, shift, true);  return true;
			case FX_KEY_TAB:   switchTab(shift ? -1 : 1);  return true;
			case FX_KEY_W:     closeTab(m_Active);          return true;

			case FX_KEY_C:
			case FX_KEY_V:
			case FX_KEY_X:
				return true;

			default: break;
			}
		}

		switch (key)
		{
		case FX_KEY_LEFT:      moveLeft(t, shift, false); return true;
		case FX_KEY_RIGHT:     moveRight(t, shift, false); return true;
		case FX_KEY_UP:        moveUp(t, shift);        return true;
		case FX_KEY_DOWN:      moveDown(t, shift);        return true;
		case FX_KEY_HOME:      moveHome(t, shift, false); return true;
		case FX_KEY_END:       moveEnd(t, shift, false); return true;
		case FX_KEY_PAGE_UP:   movePageUp(t, shift);     return true;
		case FX_KEY_PAGE_DOWN: movePageDown(t, shift);     return true;

		case FX_KEY_BACKSPACE: backspace(t);               return true;
		case FX_KEY_DELETE:    deleteForward(t);           return true;
		case FX_KEY_ENTER:     newline(t);                 return true;
		case FX_KEY_TAB:       tabKey(t, shift);           return true;

		case FX_KEY_ESCAPE:
			t.selection.clear(t.selection.active());
			return true;

		default: return false;
		}
	}

	bool CodeEditor::onChar(CharEvent& e)
	{
		Tab* t = activeTab();
		if (!t) return false;
		const unsigned int cp = e.GetCharCode();
		if (cp == 0 || cp < 32) return false;
		insertText(*t, std::u32string(1, static_cast<char32_t>(cp)));
		return true;
	}

	bool CodeEditor::onMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		if (e.GetMouseButton() != 0) return false;

		const float x = e.GetMouseX();
		const float y = e.GetMouseY();
		if (!Rect{ m_vpX, m_vpY, m_vpW, m_vpH }.contains(x, y)) return false;

		if (y < m_vpY + m_TabBarH)
		{
			float tx = m_vpX + 8.0f;
			for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i)
			{
				const float w = m_Renderer->measureText(m_Tabs[i]->title, m_CharScale) + 32.0f;
				if (x >= tx && x <= tx + w) { m_Active = i; return true; }
				tx += w;
			}
			return true;
		}

		Tab* tp = activeTab();
		if (!tp) return false;
		Tab& t = *tp;

		if (t.vbar.hitTest(x, y)) { t.vbar.onMouseDown(x, y); applyScrollFromBars(t); return true; }
		if (t.hbar.hitTest(x, y)) { t.hbar.onMouseDown(x, y); applyScrollFromBars(t); return true; }

		const Position p = pixelToPosition(t, x, y);
		t.selection.set(p, p);
		t.desiredCol = p.col;
		m_Dragging = true;
		return true;
	}

	bool CodeEditor::onMouseButtonReleased(MouseButtonReleasedEvent& e)
	{
		if (e.GetMouseButton() != 0) return false;
		Tab* t = activeTab();
		if (t) { t->vbar.onMouseUp(); t->hbar.onMouseUp(); }
		const bool was = m_Dragging;
		m_Dragging = false;
		return was;
	}

	bool CodeEditor::onMouseMoved(MouseMovedEvent& e)
	{
		Tab* tp = activeTab();
		if (!tp) return false;
		Tab& t = *tp;

		const float x = e.GetX();
		const float y = e.GetY();

		if (t.vbar.onMouseMove(x, y)) { applyScrollFromBars(t); return true; }
		if (t.hbar.onMouseMove(x, y)) { applyScrollFromBars(t); return true; }

		if (m_Dragging)
		{
			const Position p = pixelToPosition(t, x, y);
			t.selection.setActive(p);
			t.desiredCol = p.col;
			ensureCursorVisible(t);
			return true;
		}
		return false;
	}

	bool CodeEditor::onMouseScrolled(MouseScrolledEvent& e) {
		Tab* tp = activeTab();
		if (!tp) return false;
		Tab& t = *tp;

		const float mx = e.GetMouseX();
		const float my = e.GetMouseY();
		if (!Rect{ m_vpX, m_vpY, m_vpW, m_vpH }.contains(mx, my)) return false;

		t.scrollY -= static_cast<double>(e.GetYOffset()) * 3.0 * m_LineHeight;
		t.scrollX -= static_cast<double>(e.GetXOffset()) * 3.0 * 40.0;
		t.scrollY = std::max(0.0, t.scrollY);
		t.scrollX = std::max(0.0, t.scrollX);
		syncBars(t);
		return true;
	}

	void CodeEditor::applyEdit(Tab& t, Position from, Position to,
		std::u32string_view text)
	{
		from = t.buffer.clamp(from);
		to = t.buffer.clamp(to);
		if (to < from) std::swap(from, to);

		Edit ed;
		ed.from = from;
		ed.to = to;
		ed.removed = t.buffer.getText(from, to);
		ed.inserted = std::u32string(text);

		t.buffer.erase(from, to);
		t.buffer.insert(from, text);

		const Position after = t.buffer.advance(from, text);
		t.selection.clear(after);
		t.desiredCol = after.col;
		t.dirty = true;
		t.longestDirty = true;

		t.undo.push(std::move(ed));
		t.highlighter.markDirty(from.line, after.line);
	}

	void CodeEditor::insertText(Tab& t, std::u32string_view text)
	{
		if (text.empty()) return;
		if (!t.selection.empty())
			applyEdit(t, t.selection.start(), t.selection.end(), text);
		else {
			const Position p = t.selection.active();
			applyEdit(t, p, p, text);
		}
	}

	void CodeEditor::deleteSelection(Tab& t)
	{
		if (t.selection.empty()) return;
		applyEdit(t, t.selection.start(), t.selection.end(), U"");
	}

	void CodeEditor::backspace(Tab& t)
	{
		if (!t.selection.empty()) { deleteSelection(t); return; }
		const Position p = t.selection.active();
		if (p.line == 0 && p.col == 0) return;

		Position start;
		if (p.col == 0)
			start = Position{ p.line - 1, static_cast<int>(t.buffer.line(p.line - 1).size()) };
		else
			start = Position{ p.line, p.col - 1 };
		applyEdit(t, start, p, U"");
	}

	void CodeEditor::deleteForward(Tab& t)
	{
		if (!t.selection.empty()) { deleteSelection(t); return; }
		const Position p = t.selection.active();
		const auto l = t.buffer.line(p.line);

		Position end;
		if (p.col < static_cast<int>(l.size()))      end = Position{ p.line, p.col + 1 };
		else if (p.line + 1 < t.buffer.lineCount())  end = Position{ p.line + 1, 0 };
		else return;
		applyEdit(t, p, end, U"");
	}

	void CodeEditor::newline(Tab& t)
	{
		const auto l = t.buffer.line(t.selection.active().line);
		std::u32string indent;
		for (char32_t c : l)
		{
			if (c == U' ' || c == U'\t') indent += c;
			else break;
		}
		std::u32string text = U"\n";
		text += indent;
		insertText(t, text);
	}

	void CodeEditor::tabKey(Tab& t, bool)
	{
		insertText(t, U"    ");
	}

	void CodeEditor::moveCursor(Tab& t, Position p, bool selecting)
	{
		p = t.buffer.clamp(p);
		if (selecting) t.selection.setActive(p);
		else           t.selection.clear(p);
		t.desiredCol = p.col;
		ensureCursorVisible(t);
	}

	void CodeEditor::moveLeft(Tab& t, bool selecting, bool byWord)
	{
		Position p = t.selection.active();
		if (!selecting && !t.selection.empty())
		{
			moveCursor(t, t.selection.start(), false);
			return;
		}
		if (p.col > 0)
		{
			if (byWord)
			{
				const auto l = t.buffer.line(p.line);
				int c = p.col;
				while (c > 0 && !isWordChar(l[c - 1])) --c;
				while (c > 0 && isWordChar(l[c - 1])) --c;
				p.col = c;
			}
			else --p.col;
		}
		else if (p.line > 0)
		{
			--p.line;
			p.col = static_cast<int>(t.buffer.line(p.line).size());
		}
		moveCursor(t, p, selecting);
	}

	void CodeEditor::moveRight(Tab& t, bool selecting, bool byWord)
	{
		Position p = t.selection.active();
		if (!selecting && !t.selection.empty())
		{
			moveCursor(t, t.selection.end(), false);
			return;
		}
		const auto l = t.buffer.line(p.line);
		if (p.col < static_cast<int>(l.size()))
		{
			if (byWord)
			{
				int c = p.col;
				const int n = static_cast<int>(l.size());
				while (c < n && isWordChar(l[c])) ++c;
				while (c < n && !isWordChar(l[c])) ++c;
				p.col = c;
			}
			else ++p.col;
		}
		else if (p.line + 1 < t.buffer.lineCount())
		{
			++p.line;
			p.col = 0;
		}
		moveCursor(t, p, selecting);
	}

	void CodeEditor::moveUp(Tab& t, bool selecting)
	{
		Position p = t.selection.active();
		if (p.line > 0)
		{
			--p.line;
			const int len = static_cast<int>(t.buffer.line(p.line).size());
			p.col = std::min(t.desiredCol, len);
		}
		moveCursor(t, p, selecting);
		t.desiredCol = std::min(t.desiredCol,
			static_cast<int>(t.buffer.line(t.selection.active().line).size()));
	}

	void CodeEditor::moveDown(Tab& t, bool selecting)
	{
		Position p = t.selection.active();
		if (p.line + 1 < t.buffer.lineCount())
		{
			++p.line;
			const int len = static_cast<int>(t.buffer.line(p.line).size());
			p.col = std::min(t.desiredCol, len);
		}
		moveCursor(t, p, selecting);
		t.desiredCol = std::min(t.desiredCol,
			static_cast<int>(t.buffer.line(t.selection.active().line).size()));
	}

	void CodeEditor::moveHome(Tab& t, bool selecting, bool docStart)
	{
		Position p = t.selection.active();
		if (docStart) p = Position{ 0, 0 };
		else          p.col = 0;
		moveCursor(t, p, selecting);
	}

	void CodeEditor::moveEnd(Tab& t, bool selecting, bool docEnd)
	{
		Position p = t.selection.active();
		if (docEnd)
		{
			p.line = t.buffer.lineCount() - 1;
			p.col = static_cast<int>(t.buffer.line(p.line).size());
		}
		else
		{
			p.col = static_cast<int>(t.buffer.line(p.line).size());
		}
		moveCursor(t, p, selecting);
	}

	void CodeEditor::movePageUp(Tab& t, bool selecting)
	{
		Position p = t.selection.active();
		const int lines = std::max(1, static_cast<int>((m_vpH - m_TabBarH) / m_LineHeight));
		p.line = std::max(0, p.line - lines);
		p.col = std::min(p.col, static_cast<int>(t.buffer.line(p.line).size()));
		moveCursor(t, p, selecting);
	}

	void CodeEditor::movePageDown(Tab& t, bool selecting)
	{
		Position p = t.selection.active();
		const int lines = std::max(1, static_cast<int>((m_vpH - m_TabBarH) / m_LineHeight));
		p.line = std::min(t.buffer.lineCount() - 1, p.line + lines);
		p.col = std::min(p.col, static_cast<int>(t.buffer.line(p.line).size()));
		moveCursor(t, p, selecting);
	}

	void CodeEditor::doUndo(Tab& t)
	{
		if (!t.undo.canUndo()) return;
		Edit ed = t.undo.popUndo();

		const Position from = ed.from;
		const Position to = t.buffer.advance(from, ed.inserted);

		t.buffer.erase(from, to);
		t.buffer.insert(from, ed.removed);

		t.selection.clear(from);
		t.desiredCol = from.col;
		t.dirty = true;
		t.longestDirty = true;

		const int endLine = from.line + static_cast<int>(ed.removed.size()) + 1;
		t.highlighter.markDirty(from.line, endLine);
	}

	void CodeEditor::doRedo(Tab& t)
	{
		if (!t.undo.canRedo()) return;
		Edit ed = t.undo.popRedo();

		const Position from = ed.from;
		const Position to = t.buffer.advance(from, ed.removed);

		t.buffer.erase(from, to);
		t.buffer.insert(from, ed.inserted);

		const Position after = t.buffer.advance(from, ed.inserted);
		t.selection.clear(after);
		t.desiredCol = after.col;
		t.dirty = true;
		t.longestDirty = true;

		t.highlighter.markDirty(from.line, after.line);
	}

	void CodeEditor::layout(float x, float y, float w, float h)
	{
		m_vpX = x; m_vpY = y; m_vpW = w; m_vpH = h;
		m_LineHeight = m_Renderer->lineHeight(m_CharScale);

		Tab* t = activeTab();
		if (!t) return;

		const float editorY = y + m_TabBarH;
		const float editorH = h - m_TabBarH;
		const float textW = w - m_GutterWidth - m_ScrollBarSize;
		const float textH = editorH - m_ScrollBarSize;

		t->vbar.layout(x + w - m_ScrollBarSize, editorY, m_ScrollBarSize, textH);
		t->hbar.layout(x + m_GutterWidth, y + m_TabBarH + textH, textW, m_ScrollBarSize);
	}

	void CodeEditor::syncBars(Tab& t)
	{
		const double contentH = static_cast<double>(t.buffer.lineCount()) * m_LineHeight;
		const double viewH = std::max(0.0, static_cast<double>(m_vpH - m_TabBarH - m_ScrollBarSize));
		t.vbar.setContent(contentH, viewH);
		t.vbar.setValue(t.scrollY);

		if (t.longestDirty)
		{
			double maxW = 0.0;
			for (int i = 0; i < t.buffer.lineCount(); ++i)
			{
				const double w = m_Renderer->measureText(t.buffer.line(i), m_CharScale);
				if (w > maxW) maxW = w;
			}
			t.longestWidth = maxW;
			t.longestDirty = false;
		}
		const double contentW = t.longestWidth;
		const double viewW = std::max(0.0, static_cast<double>(m_vpW - m_GutterWidth - m_ScrollBarSize));
		t.hbar.setContent(contentW, viewW);
		t.hbar.setValue(t.scrollX);
	}

	void CodeEditor::applyScrollFromBars(Tab& t)
	{
		t.scrollY = t.vbar.value();
		t.scrollX = t.hbar.value();
	}

	void CodeEditor::ensureCursorVisible(Tab& t)
	{
		const float viewH = m_vpH - m_TabBarH - m_ScrollBarSize;
		const float viewW = m_vpW - m_GutterWidth - m_ScrollBarSize;
		if (viewH <= 0.0f || viewW <= 0.0f) return;

		const Position c = t.selection.active();

		const double cursorY = static_cast<double>(c.line) * m_LineHeight;
		if (cursorY < t.scrollY) t.scrollY = cursorY;
		else if (cursorY + m_LineHeight > t.scrollY + viewH)
			t.scrollY = cursorY + m_LineHeight - viewH;

		const float cursorX = colToX(t, c.line, c.col);
		const float caretW = m_LineHeight * 0.6f;
		if (cursorX < t.scrollX) t.scrollX = cursorX;
		else if (cursorX + caretW > t.scrollX + viewW)
			t.scrollX = cursorX + caretW - viewW;

		t.scrollY = std::max(0.0, t.scrollY);
		t.scrollX = std::max(0.0, t.scrollX);
		syncBars(t);
	}

	float CodeEditor::colToX(const Tab& t, int line, int col) const
	{
		auto l = t.buffer.line(line);
		col = std::clamp(col, 0, static_cast<int>(l.size()));
		if (col == 0) return 0.0f;
		return m_Renderer->measureText(l.substr(0, static_cast<size_t>(col)), m_CharScale);
	}

	int CodeEditor::xToCol(const Tab& t, int line, float x) const
	{
		if (x <= 0.0f) return 0;
		auto l = t.buffer.line(line);
		float acc = 0.0f;
		for (int i = 0; i < static_cast<int>(l.size()); ++i)
		{
			const float w = m_Renderer->measureText(l.substr(i, 1), m_CharScale);
			if (acc + w * 0.5f > x) return i;
			acc += w;
		}
		return static_cast<int>(l.size());
	}

	Position CodeEditor::pixelToPosition(Tab& t, float x, float y) const
	{
		const float textTop = m_vpY + m_TabBarH;
		const float localY = y - textTop + static_cast<float>(t.scrollY);

		int line = static_cast<int>(localY / m_LineHeight);
		if (line < 0) line = 0;
		if (line >= t.buffer.lineCount()) line = t.buffer.lineCount() - 1;

		const float textX = m_vpX + m_GutterWidth;
		const float localX = x - textX + static_cast<float>(t.scrollX);

		const int col = xToCol(t, line, localX);
		return Position{ line, col };
	}

	void CodeEditor::update(double, double now)
	{
		m_CursorVisible = (std::fmod(now, 1.0) < 0.5);
		for (auto& t : m_Tabs)
		{
			t->highlighter.update(t->buffer);
			syncBars(*t);
		}
	}

	void CodeEditor::render(float x, float y, float w, float h)
	{
		if (!m_Renderer) return;
		layout(x, y, w, h);

		m_Renderer->drawRect(x, y, w, h, m_Theme.bg);
		drawTabBar();

		Tab* t = activeTab();
		if (!t) return;

		const float editorY = y + m_TabBarH;
		const float editorH = h - m_TabBarH;
		const float textX = x + m_GutterWidth;
		const float textY = editorY;
		const float textW = w - m_GutterWidth - m_ScrollBarSize;
		const float textH = editorH - m_ScrollBarSize;

		drawCurrentLine(*t, textX, textY, textW);
		drawSelection(*t, textX, textY, textW, textH);
		drawTextLines(*t, textX, textY, textW, textH);
		drawGutter(*t, textY, textH);
		drawCursor(*t, textX, textY, textH);
		drawScrollBars(*t);
	}

	void CodeEditor::drawTabBar()
	{
		m_Renderer->drawRect(m_vpX, m_vpY, m_vpW, m_TabBarH, m_Theme.tabBg);

		float tx = m_vpX + 8.0f;
		for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i)
		{
			Tab& tab = *m_Tabs[i];
			const float w = m_Renderer->measureText(tab.title, m_CharScale) + 32.0f;
			const Color bg = (i == m_Active) ? m_Theme.tabActive : m_Theme.tabBg;
			m_Renderer->drawRect(tx, m_vpY, w, m_TabBarH, bg);

			std::u32string title = tab.title;
			if (tab.dirty) title += U" *";
			m_Renderer->drawText(title, tx + 12.0f, m_vpY + 7.0f, m_CharScale, m_Theme.text);
			tx += w;
		}
	}

	void CodeEditor::drawCurrentLine(Tab& t, float textX, float textY, float textW)
	{
		const int curLine = t.selection.active().line;
		const float y = textY + static_cast<float>(curLine) * m_LineHeight - static_cast<float>(t.scrollY);
		if (y + m_LineHeight < textY) return;
		if (y > textY + m_vpH)        return;
		m_Renderer->drawRect(textX, y, textW, m_LineHeight, m_Theme.currentLine);
	}

	void CodeEditor::drawSelection(Tab& t, float textX, float textY,
		float textW, float textH) {
		(void)textW;
		if (t.selection.empty()) return;

		const Position s = t.selection.start();
		const Position e = t.selection.end();

		for (int line = s.line; line <= e.line; ++line)
		{
			const float y = textY + static_cast<float>(line) * m_LineHeight
				- static_cast<float>(t.scrollY);
			if (y + m_LineHeight < textY) continue;
			if (y > textY + textH)       break;

			const int lineLen = static_cast<int>(t.buffer.line(line).size());
			const int colStart = (line == s.line) ? s.col : 0;
			const bool addTrailingSpace = (line != e.line);

			int colEnd;
			if (line == e.line) colEnd = e.col;
			else                colEnd = lineLen;

			const float x0 = textX + colToX(t, line, colStart)
				- static_cast<float>(t.scrollX);

			float x1;
			if (addTrailingSpace)
				x1 = textX + colToX(t, line, lineLen)
				+ m_Renderer->measureText(U" ", m_CharScale)
				- static_cast<float>(t.scrollX);
			else
				x1 = textX + colToX(t, line, colEnd)
				- static_cast<float>(t.scrollX);

			if (x1 > x0)
				m_Renderer->drawRect(x0, y, x1 - x0, m_LineHeight, m_Theme.selection);
		}
	}

	void CodeEditor::drawTextLines(Tab& t, float textX, float textY,
		float textW, float textH)
	{
		(void)textW;

		const int first = std::max(0, static_cast<int>(t.scrollY / m_LineHeight));
		const int visLines = static_cast<int>(textH / m_LineHeight) + 2;
		const int last = std::min(t.buffer.lineCount() - 1, first + visLines);

		for (int line = first; line <= last; ++line)
		{
			const float y = textY + static_cast<float>(line) * m_LineHeight
				- static_cast<float>(t.scrollY);
			float x = textX - static_cast<float>(t.scrollX);

			const auto& tokens = t.highlighter.tokens(line);
			const auto  text = t.buffer.line(line);

			int cursor = 0;
			for (const auto& tok : tokens)
			{
				if (tok.start > cursor)
				{
					const auto chunk = text.substr(cursor, tok.start - cursor);
					m_Renderer->drawText(chunk, x, y, m_CharScale, m_Theme.text);
					x += m_Renderer->measureText(chunk, m_CharScale);
				}
				const auto chunk = text.substr(tok.start, tok.end - tok.start);
				m_Renderer->drawText(chunk, x, y, m_CharScale, m_Theme.of(tok.type));
				x += m_Renderer->measureText(chunk, m_CharScale);
				cursor = tok.end;
			}

			if (cursor < static_cast<int>(text.size()))
				m_Renderer->drawText(text.substr(cursor), x, y, m_CharScale, m_Theme.text);
		}
	}

	void CodeEditor::drawGutter(Tab& t, float textTop, float textH)
	{
		m_Renderer->drawRect(m_vpX, textTop, m_GutterWidth, textH, m_Theme.gutterBg);

		const int first = std::max(0, static_cast<int>(t.scrollY / m_LineHeight));
		const int visLines = static_cast<int>(textH / m_LineHeight) + 2;
		const int last = std::min(t.buffer.lineCount() - 1, first + visLines);

		for (int line = first; line <= last; ++line)
		{
			const float y = textTop + static_cast<float>(line) * m_LineHeight - static_cast<float>(t.scrollY);
			const std::u32string num = utf8_to_u32(std::to_string(line + 1));
			const float w = m_Renderer->measureText(num, m_CharScale);
			m_Renderer->drawText(num, m_vpX + m_GutterWidth - w - 8.0f, y, m_CharScale, m_Theme.gutterText);
		}
	}

	void CodeEditor::drawCursor(Tab& t, float textX, float textY, float textH)
	{
		if (!m_CursorVisible) return;
		const Position c = t.selection.active();
		const float y = textY + static_cast<float>(c.line) * m_LineHeight
			- static_cast<float>(t.scrollY);
		if (y + m_LineHeight < textY) return;
		if (y > textY + textH)       return;

		const float x = textX + colToX(t, c.line, c.col) - static_cast<float>(t.scrollX);
		m_Renderer->drawRect(x, y, 2.0f, m_LineHeight, m_Theme.cursor);
	}

	void CodeEditor::drawScrollBars(Tab& t)
	{
		m_Renderer->drawRect(
			static_cast<float>(t.vbar.trackX()),
			static_cast<float>(t.vbar.trackY()),
			static_cast<float>(t.vbar.trackW()),
			static_cast<float>(t.vbar.trackH()),
			m_Theme.scrollTrack);
		m_Renderer->drawRect(
			static_cast<float>(t.vbar.trackX()),
			static_cast<float>(t.vbar.trackY() + t.vbar.thumbPos()),
			static_cast<float>(t.vbar.trackW()),
			static_cast<float>(t.vbar.thumbLen()),
			m_Theme.scrollThumb);

		m_Renderer->drawRect(
			static_cast<float>(t.hbar.trackX()),
			static_cast<float>(t.hbar.trackY()),
			static_cast<float>(t.hbar.trackW()),
			static_cast<float>(t.hbar.trackH()),
			m_Theme.scrollTrack);
		m_Renderer->drawRect(
			static_cast<float>(t.hbar.trackX() + t.hbar.thumbPos()),
			static_cast<float>(t.hbar.trackY()),
			static_cast<float>(t.hbar.thumbLen()),
			static_cast<float>(t.hbar.trackH()),
			m_Theme.scrollThumb);
	}

}
