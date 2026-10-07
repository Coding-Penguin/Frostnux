#include "fxpch.h"
#include "CodeEditor.h"
#include "TextBuffer.h"
#include "SymbolIndex.h"
#include <glad/glad.h>
#include <fstream>

namespace Frostnux {

	namespace {

		char32_t toLowerUnicode(char32_t c)
		{
			if (c >= U'A' && c <= U'Z') return c + 32;
			if (c >= 0x00C0 && c <= 0x00DE && c != 0x00D7) return c + 0x20;
			if (c >= 0x0100 && c <= 0x017F && (c % 2 == 0)) return c + 1;
			if (c >= 0x0391 && c <= 0x03A9 && c != 0x03A2) return c + 0x20;
			if (c >= 0x0410 && c <= 0x042F) return c + 0x20;
			if (c >= 0x0400 && c <= 0x040F) return c + 0x50;
			if (c >= 0x0460 && c <= 0x04FF && (c % 2 == 0)) return c + 1;
			return c;
		}

		bool isWordBoundary(std::u32string_view line, int col, int len)
		{
			if (col > 0 && CompletionEngine::isIdentifierChar(line[static_cast<std::basic_string_view<char32_t, std::char_traits<char32_t>>::size_type>(col) - 1]))
				return false;
			const int end = col + len;
			if (end < (int)line.size() && CompletionEngine::isIdentifierChar(line[end]))
				return false;
			return true;
		}

		int globMatchLen(std::u32string_view text, int start, int end, std::u32string_view pat, int pi, bool caseSensitive)
		{
			int ti = start;
			while (pi < (int)pat.size())
			{
				char32_t pc = pat[pi];

				if (pc == U'\\' && pi + 1 < (int)pat.size())
				{
					if (ti >= end) return -1;
					char32_t tc = text[ti], lit = pat[static_cast<std::basic_string_view<char32_t, std::char_traits<char32_t>>::size_type>(pi) + 1];
					if (!caseSensitive) { tc = toLowerUnicode(tc); lit = toLowerUnicode(lit); }
					if (tc != lit) return -1;
					++ti; pi += 2;
				}
				else if (pc == U'?')
				{
					if (ti >= end) return -1;
					++ti; ++pi;
				}
				else if (pc == U'*')
				{
					while (pi < (int)pat.size() && pat[pi] == U'*') ++pi;
					if (pi == (int)pat.size()) return end - start;

					for (int k = ti; k <= end; ++k)
					{
						int sub = globMatchLen(text, k, end, pat, pi, caseSensitive);
						if (sub >= 0) return (k - start) + sub;
					}
					return -1;
				}
				else
				{
					if (ti >= end) return -1;
					char32_t tc = text[ti];
					if (!caseSensitive) { tc = toLowerUnicode(tc); pc = toLowerUnicode(pc); }
					if (tc != pc) return -1;
					++ti; ++pi;
				}
			}
			return ti - start;
		}

		std::u32string applyIndent(std::u32string_view line, int delta)
		{
			if (delta > 0)
			{
				if (line.empty()) return {};
				std::u32string result;
				result.reserve(line.size() + 4);
				result += U"    ";
				result.append(line);
				return result;
			}
			else
			{
				int remove = 0;
				const int n = static_cast<int>(line.size());

				if (n >= 4 && line[0] == U' ' && line[1] == U' ' && line[2] == U' ' && line[3] == U' ')
				{
					remove = 4;
				}
				else if (n > 0 && line[0] == U'\t')
				{
					remove = 1;
				}
				else
				{
					while (remove < n && line[remove] == U' ') ++remove;
				}

				return std::u32string(line.substr(remove));
			}
		}

	}

	Tab& CodeEditor::addTab(const std::string& path)
	{
		auto t = std::make_unique<Tab>();
		if (!path.empty())
		{
			t->path = path;
			const size_t slash = path.find_last_of("/\\");
			const std::string base = (slash == std::string::npos) ? path : path.substr(slash + 1);
			t->title = utf8_to_u32(base);
		}
		Tab& ref = *t;
		m_Tabs.push_back(std::move(t));
		m_Active = static_cast<int>(m_Tabs.size()) - 1;
		ref.invalidateHighlight();
		return ref;
	}

	Tab& CodeEditor::newFile(const std::string& title)
	{
		auto t = std::make_unique<Tab>();
		t->title = utf8_to_u32(title);

		Tab& ref = *t;
		m_Tabs.push_back(std::move(t));
		m_Active = static_cast<int>(m_Tabs.size()) - 1;
		ref.invalidateHighlight();
		return ref;
	}

	Tab* CodeEditor::openFile(const std::string& path)
	{
		for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i)
		{
			if (m_Tabs[i]->path == path)
			{
				m_Active = i;
				return m_Tabs[i].get();
			}
		}

		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file) return nullptr;

		const std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::string content(static_cast<size_t>(size), '\0');
		if (!file.read(content.data(), size))
			return nullptr;

		if (content.size() >= 3 && static_cast<unsigned char>(content[0]) == 0xEF && static_cast<unsigned char>(content[1]) == 0xBB && static_cast<unsigned char>(content[2]) == 0xBF)
		{
			content.erase(0, 3);
		}

		Tab& tab = addTab(path);
		tab.buffer.setText(utf8_to_u32(content));
		tab.invalidateHighlight();
		return &tab;
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
		const int	rawKey = e.GetKeyCode();
		const int	rawMods = e.GetMods();
		const bool	rawCtrl = (rawMods & FX_KEY_CONTROL) != 0;
		const bool	rawShift = (rawMods & FX_KEY_SHIFT) != 0;

		if (rawCtrl && rawKey == FX_KEY_F) { openSearch(false); return true; }
		if (rawCtrl && rawKey == FX_KEY_H) { openSearch(true);  return true; }

		if (m_Search.active)
		{
			Tab* tp = activeTab();

			if (rawKey == FX_KEY_ESCAPE) { closeSearch(); return true; }

			if (rawKey == FX_KEY_ENTER && tp)
			{
				if (m_Search.replaceMode && m_Search.focusedField == 1)
					replaceCurrentMatch(*tp);
				else if (rawShift)
					gotoPrevMatch(*tp);
				else
					gotoNextMatch(*tp);
				return true;
			}

			if (rawKey == FX_KEY_F3 && tp)
			{
				if (rawShift) gotoPrevMatch(*tp);
				else          gotoNextMatch(*tp);
				return true;
			}

			if (rawKey == FX_KEY_TAB)
			{
				if (m_Search.replaceMode)
					m_Search.focusedField = 1 - m_Search.focusedField;
				return true;
			}

			if (rawKey == FX_KEY_BACKSPACE) { searchInputBackspace(); return true; }
			if (rawKey == FX_KEY_DELETE) { searchInputDelete();    return true; }
			if (rawKey == FX_KEY_LEFT) { searchInputMoveLeft();  return true; }
			if (rawKey == FX_KEY_RIGHT) { searchInputMoveRight(); return true; }
			if (rawKey == FX_KEY_HOME)
			{
				if (m_Search.focusedField == 0)	m_Search.queryCursor = 0;
				else							m_Search.replaceCursor = 0;
				return true;
			}
			if (rawKey == FX_KEY_END)
			{
				if (m_Search.focusedField == 0)	m_Search.queryCursor = (int)m_Search.query.size();
				else							m_Search.replaceCursor = (int)m_Search.replaceText.size();
				return true;
			}
		}

		if (m_Comp.active)
		{
			const int key = e.GetKeyCode();
			switch (key)
			{
			case FX_KEY_UP:		moveCompletion(-1);	return true;
			case FX_KEY_DOWN:	moveCompletion(+1);	return true;
			case FX_KEY_ENTER:
			case FX_KEY_TAB:	acceptCompletion();	return true;
			case FX_KEY_ESCAPE:	cancelCompletion();	return true;
			default: break;
			}

			if (key == FX_KEY_SPACE && (e.GetMods() & FX_KEY_CONTROL))
			{
				triggerCompletion(true);
				return true;
			}
		}

		Tab* tp = activeTab();
		if (!tp) return false;
		Tab& t = *tp;

		const int	key = e.GetKeyCode();
		const int	mods = e.GetMods();
		const bool	ctrl = (mods & FX_KEY_CONTROL) != 0;
		const bool	shift = (mods & FX_KEY_SHIFT) != 0;
		const bool	alt = (mods & FX_KEY_ALT) != 0;

		if (alt && !ctrl)
		{
			if (key == FX_KEY_LEFT) { navBack();    return true; }
			if (key == FX_KEY_RIGHT) { navForward(); return true; }
		}

		if (!ctrl && !alt)
		{
			if (key == FX_KEY_F12) { goToDefinition();                return true; }
		}

		if (ctrl && !alt)
		{
			if (key == FX_KEY_RIGHT_BRACKET) { jumpToMatchingBrace(t, true);  return true; }
			if (key == FX_KEY_LEFT_BRACKET) { jumpToMatchingBrace(t, false); return true; }
		}

		if (ctrl)
		{
			switch (key)
			{
			case FX_KEY_Z:		if (shift) doRedo(t); else doUndo(t);	return true;
			case FX_KEY_Y:		doRedo(t);								return true;
			case FX_KEY_A:
			{
				Position s { 0, 0 };
				Position en { t.buffer.lineCount() - 1, static_cast<int>(t.buffer.line(t.buffer.lineCount() - 1).size()) };
				t.selection.set(s, en);
				return true;
			}
			case FX_KEY_HOME:	moveHome(t, shift, true);	return true;
			case FX_KEY_END:	moveEnd(t, shift, true);	return true;
			case FX_KEY_LEFT:	moveLeft(t, shift, true);	return true;
			case FX_KEY_RIGHT:	moveRight(t, shift, true);	return true;
			case FX_KEY_TAB:	switchTab(shift ? -1 : 1);	return true;
			case FX_KEY_W:		closeTab(m_Active);			return true;
			case FX_KEY_S:		saveTab(t, shift);			return true;
			case FX_KEY_C:		doCopy(t);					return true;
			case FX_KEY_V:		doPaste(t);					return true;
			case FX_KEY_X:		doCut(t);					return true;
			default: break;
			}
		}

		switch (key)
		{
		case FX_KEY_LEFT:		moveLeft(t, shift, false);					return true;
		case FX_KEY_RIGHT:		moveRight(t, shift, false);					return true;
		case FX_KEY_UP:			moveUp(t, shift);							return true;
		case FX_KEY_DOWN:		moveDown(t, shift);							return true;
		case FX_KEY_HOME:		moveHome(t, shift, false);					return true;
		case FX_KEY_END:		moveEnd(t, shift, false);					return true;
		case FX_KEY_PAGE_UP:	movePageUp(t, shift);						return true;
		case FX_KEY_PAGE_DOWN:	movePageDown(t, shift);						return true;
		case FX_KEY_BACKSPACE:	backspace(t);								return true;
		case FX_KEY_DELETE:		deleteForward(t);							return true;
		case FX_KEY_ENTER:		newline(t);									return true;
		case FX_KEY_TAB:		tabKey(t, shift);							return true;
		case FX_KEY_ESCAPE:		t.selection.clear(t.selection.active());	return true;
		default: return false;
		}
	}

	bool CodeEditor::onChar(CharEvent& e)
	{
		if (m_Search.active)
		{
			const unsigned int cp = e.GetCharCode();
			if (cp == 0 || cp < 32) return false;
			searchInputChar(static_cast<char32_t>(cp));
			return true;
		}

		Tab* t = activeTab();
		if (!t) return false;
		const unsigned int cp = e.GetCharCode();
		if (cp == 0 || cp < 32) return false;

		const char32_t c = static_cast<char32_t>(cp);

		static const std::u32string kOpens = U"([{\"'";
		static const std::u32string kCloses = U")]}\"'";

		char32_t closeCh = 0;
		for (size_t k = 0; k < kOpens.size(); ++k)
		{
			if (kOpens[k] == c) { closeCh = kCloses[k]; break; }
		}

		if (closeCh != 0)
		{
			if (!t->selection.empty())
			{
				const Position s = t->selection.start();
				const Position epos = t->selection.end();
				const std::u32string sel = t->buffer.getText(s, epos);

				std::u32string wrapped;
				wrapped.reserve(sel.size() + 2);
				wrapped += c;
				wrapped += sel;
				wrapped += closeCh;

				applyEdit(*t, s, epos, wrapped);

				const Position after = t->selection.active();
				if (after.col > 0)
				{
					Position target { after.line, after.col - 1 };
					t->selection.clear(target);
					t->desiredCol = target.col;
					ensureCursorVisible(*t);
				}
				cancelCompletion();
				return true;
			}

			const Position pos = t->selection.active();
			auto line = t->buffer.line(pos.line);

			if (c == U'"' || c == U'\'')
			{
				if (pos.col > 0)
				{
					const char32_t prev = line[static_cast<std::basic_string_view<char32_t, std::char_traits<char32_t>>::size_type>(pos.col) - 1];
					const bool wordPrev = (prev >= U'a' && prev <= U'z') || (prev >= U'A' && prev <= U'Z') || (prev >= U'0' && prev <= U'9') || prev == U'_';
					if (wordPrev)
					{
						insertText(*t, std::u32string(1, c));
						cancelCompletion();
						return true;
					}
				}
			}

			if (pos.col < static_cast<int>(line.size()) && line[pos.col] == closeCh)
			{
				Position next{ pos.line, pos.col + 1 };
				t->selection.clear(next);
				t->desiredCol = next.col;
				cancelCompletion();
				return true;
			}

			std::u32string pair;
			pair += c;
			pair += closeCh;
			insertText(*t, pair);

			const Position cur = t->selection.active();
			Position mid { cur.line, cur.col - 1 };
			t->selection.clear(mid);
			t->desiredCol = mid.col;
			cancelCompletion();
			return true;
		}

		insertText(*t, std::u32string(1, c));

		const Position pos = t->selection.active();
		const auto line = t->buffer.line(pos.line);

		bool shouldTrigger = false;
		if (CompletionEngine::isIdentifierChar(c))
			shouldTrigger = true;
		else if (c == U'.')
			shouldTrigger = true;
		else if (c == U'>' && pos.col >= 2 && line[static_cast<size_t>(pos.col) - 2] == U'-')
			shouldTrigger = true;
		else if (c == U':' && pos.col >= 2 && line[static_cast<size_t>(pos.col) - 2] == U':')
			shouldTrigger = true;

		if (shouldTrigger)	triggerCompletion(false);
		else				cancelCompletion();

		return true;
	}

	bool CodeEditor::onMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		if (e.GetMouseButton() != 0) return false;

		const float x = e.GetMouseX();
		const float y = e.GetMouseY();
		if (!Rect{ m_vpX, m_vpY, m_vpW, m_vpH }.contains(x, y)) return false;

		if (m_Search.active && handleSearchClick(x, y)) return true;

		if (y < m_vpY + m_TabBarH)
		{
			constexpr float kPadLeft = 12.0f;
			constexpr float kPadRight = 28.0f;
			constexpr float kCloseSize = 16.0f;

			float tx = m_vpX + 8.0f;
			for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i)
			{
				const float titleW = m_Renderer->measureText(m_Tabs[i]->title, m_CharScale);
				const float w = titleW + kPadLeft + kPadRight + m_CloseButtonXOffset;

				const float closeX = tx + w - kCloseSize - 6.0f;
				const float closeY = m_vpY + (m_TabBarH - kCloseSize) * 0.5f;
				if (x >= closeX - 4.0f && x <= closeX + kCloseSize + 4.0f && y >= closeY && y <= closeY + kCloseSize)
				{
					closeTab(i);
					return true;
				}

				if (x >= tx && x <= tx + w)
				{
					m_Active = i;
					return true;
				}
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

		if (m_IsCtrlDown && m_IsCtrlDown())
		{
			auto line = t.buffer.line(p.line);
			if (p.col < static_cast<int>(line.size()) && CompletionEngine::isIdentifierChar(line[p.col]))
			{
				int s = p.col;
				int e2 = p.col;
				while (s > 0 && CompletionEngine::isIdentifierChar(line[static_cast<std::basic_string_view<char32_t, std::char_traits<char32_t>>::size_type>(s) - 1])) --s;
				while (e2 < static_cast<int>(line.size()) && CompletionEngine::isIdentifierChar(line[e2])) ++e2;

				std::u32string name(line.substr(s, e2 - s));
				if (t.symbols.findSymbolAnywhere(name))
				{
					t.selection.clear(p);
					t.desiredCol = p.col;
					goToDefinition();
					return true;
				}
			}
		}

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
		m_MouseX = e.GetX();
		m_MouseY = e.GetY();

		Tab* tp = activeTab();
		if (!tp) return false;
		Tab& t = *tp;

		if (t.vbar.onMouseMove(m_MouseX, m_MouseY)) { applyScrollFromBars(t); return true; }
		if (t.hbar.onMouseMove(m_MouseX, m_MouseY)) { applyScrollFromBars(t); return true; }

		if (m_Dragging)
		{
			const Position p = pixelToPosition(t, m_MouseX, m_MouseY);
			t.selection.setActive(p);
			t.desiredCol = p.col;
			ensureCursorVisible(t);
			return true;
		}
		return false;
	}

	bool CodeEditor::onMouseScrolled(MouseScrolledEvent& e)
	{
		Tab* tp = activeTab();
		if (!tp) return false;
		Tab& t = *tp;

		if (!Rect{ m_vpX, m_vpY, m_vpW, m_vpH }.contains(m_MouseX, m_MouseY)) return false;

		t.scrollY -= static_cast<double>(e.GetYOffset()) * 3.0 * m_LineHeight;
		t.scrollX -= static_cast<double>(e.GetXOffset()) * 3.0 * 40.0;
		t.scrollY = std::max(0.0, t.scrollY);
		t.scrollX = std::max(0.0, t.scrollX);
		syncBars(t);
		return true;
	}

	void CodeEditor::applyEdit(Tab& t, Position from, Position to, std::u32string_view text) const
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

		const bool structural = text.find(U'\n') != std::u32string_view::npos || ed.removed.find(U'\n') != std::u32string::npos;

		if (structural)
			t.highlighter.markDirtyToEnd(std::min(from.line, after.line));
		else
			t.highlighter.markDirty(from.line, after.line);

		t.markSymbolsDirty(m_CurrentTime);
	}

	void CodeEditor::insertText(Tab& t, std::u32string_view text)
	{
		if (text.empty()) return;
		if (!t.selection.empty())
		{
			applyEdit(t, t.selection.start(), t.selection.end(), text);
		}
		else
		{
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

		if (p.col > 0)
		{
			auto line = t.buffer.line(p.line);
			if (p.col < static_cast<int>(line.size()))
			{
				const char32_t before = line[static_cast<std::basic_string_view<char32_t, std::char_traits<char32_t>>::size_type>(p.col) - 1];
				const char32_t after = line[p.col];

				const bool pair = (before == U'(' && after == U')') || (before == U'[' && after == U']') || (before == U'{' && after == U'}') || (before == U'"' && after == U'"') || (before == U'\'' && after == U'\'');

				if (pair)
				{
					Position start{ p.line, p.col - 1 };
					Position end{ p.line, p.col + 1 };
					applyEdit(t, start, end, U"");
					return;
				}
			}
		}

		Position start;
		if (p.col == 0)
			start = Position { p.line - 1, static_cast<int>(t.buffer.line(p.line - 1).size()) };
		else
			start = Position { p.line, p.col - 1 };
		applyEdit(t, start, p, U"");
	}

	void CodeEditor::deleteForward(Tab& t)
	{
		if (!t.selection.empty()) { deleteSelection(t); return; }
		const Position p = t.selection.active();
		const auto l = t.buffer.line(p.line);

		Position end;
		if (p.col < static_cast<int>(l.size()))		end = Position{ p.line, p.col + 1 };
		else if (p.line + 1 < t.buffer.lineCount())	end = Position{ p.line + 1, 0 };
		else return;
		applyEdit(t, p, end, U"");
	}

	void CodeEditor::newline(Tab& t)
	{
		if (!t.selection.empty())
		{
			deleteSelection(t);
		}

		const Position cur = t.selection.active();
		const auto line = t.buffer.line(cur.line);

		std::u32string indent;
		for (char32_t c : line)
		{
			if (c == U' ' || c == U'\t') indent += c;
			else break;
		}

		int back = cur.col - 1;
		while (back >= 0 && (line[back] == U' ' || line[back] == U'\t')) --back;
		if (back >= 0 && line[back] == U'{')
		{
			indent += U"    ";
		}

		int fwd = cur.col;
		while (fwd < static_cast<int>(line.size()) && (line[fwd] == U' ' || line[fwd] == U'\t')) ++fwd;
		if (fwd < static_cast<int>(line.size()) && line[fwd] == U'}')
		{
			if (indent.size() >= 4) indent.resize(indent.size() - 4);
		}

		std::u32string text = U"\n";
		text += indent;
		insertText(t, text);
	}

	void CodeEditor::tabKey(Tab& t, bool shift)
	{
		const int delta = shift ? -1 : 1;

		if (t.selection.empty())
		{
			if (!shift)
			{
				insertText(t, U"    ");
				return;
			}

			indentLines(t, t.selection.active().line, t.selection.active().line, -1);
			return;
		}

		const Position s = t.selection.start();
		const Position e = t.selection.end();
		indentLines(t, s.line, e.line, delta);
	}

	void CodeEditor::indentLines(Tab& t, int lineFrom, int lineTo, int delta)
	{
		if (lineFrom < 0 || lineTo < lineFrom) return;
		if (lineTo >= t.buffer.lineCount()) return;

		const bool hadSelection = !t.selection.empty();
		const int  origSelStartLine = t.selection.start().line;
		const int  origSelEndLine = t.selection.end().line;

		std::u32string newText;
		for (int i = lineFrom; i <= lineTo; ++i)
		{
			if (i > lineFrom) newText += U'\n';
			newText += applyIndent(t.buffer.line(i), delta);
		}

		Position from { lineFrom, 0 };
		Position to { lineTo, static_cast<int>(t.buffer.line(lineTo).size()) };

		applyEdit(t, from, to, newText);

		if (hadSelection)
		{
			Position newStart { origSelStartLine, 0 };
			Position newEnd { origSelEndLine, static_cast<int>(t.buffer.line(origSelEndLine).size()) };
			t.selection.set(newStart, newEnd);
		}
		else
		{
			const Position cur = t.selection.active();
			const int newLen = static_cast<int>(t.buffer.line(cur.line).size());
			t.selection.clear(Position{ cur.line, std::min(cur.col, newLen) });
			t.desiredCol = t.selection.active().col;
		}
	}

	void CodeEditor::moveCursor(Tab& t, Position p, bool selecting)
	{
		p = t.buffer.clamp(p);
		if (selecting)	t.selection.setActive(p);
		else			t.selection.clear(p);
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
				while (c > 0 && !isWordChar(l[static_cast<std::basic_string_view<char32_t, std::char_traits<char32_t>>::size_type>(c) - 1])) --c;
				while (c > 0 && isWordChar(l[static_cast<std::basic_string_view<char32_t, std::char_traits<char32_t>>::size_type>(c) - 1])) --c;
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
		t.desiredCol = std::min(t.desiredCol, static_cast<int>(t.buffer.line(t.selection.active().line).size()));
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
		const bool structural = ed.removed.find(U'\n') != std::u32string::npos || ed.inserted.find(U'\n') != std::u32string::npos;
		
		if (structural)	t.highlighter.markDirtyToEnd(from.line);
		else			t.highlighter.markDirty(from.line, endLine);
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

		const bool structural = ed.removed.find(U'\n') != std::u32string::npos || ed.inserted.find(U'\n') != std::u32string::npos;
		if (structural)	t.highlighter.markDirtyToEnd(from.line);
		else			t.highlighter.markDirty(from.line, after.line);
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
		m_CurrentTime = now;
		m_CursorVisible = (std::fmod(now, 1.0) < 0.5);

		for (auto& t : m_Tabs)
		{
			t->highlighter.update(t->buffer);
			syncBars(*t);
		}

		constexpr double kDebounceSeconds = 0.1; // 100 ms
		for (auto& t : m_Tabs)
		{
			if (t->symbolsDirty && (now - t->symbolsDirtyAt) > kDebounceSeconds)
			{
				t->symbols.rebuild(t->buffer);
				t->symbolsDirty = false;
				t->symbolsDirtyAt = 0.0;
			}
		}

		if (Tab* t = activeTab())
			updateReferenceHighlights(*t);
	}

	void CodeEditor::render(float x, float y, float w, float h)
	{
		if (!m_Renderer) return;

		Tab* t = activeTab();
		if (!t) return;

		layout(x, y, w, h);

		m_Renderer->drawRect(x, y, w, h, m_Theme.bg);
		drawTabBar();

		const float editorY = y + m_TabBarH;
		const float editorH = h - m_TabBarH;
		const float textX = x + m_GutterWidth;
		const float textY = editorY;
		const float textW = w - m_GutterWidth - m_ScrollBarSize;
		const float textH = editorH - m_ScrollBarSize;

		drawCurrentLine(*t, textX, textY, textW);
		drawReferenceHighlights(*t, textX, textY, textW, textH);
		drawSearchHighlights(*t, textX, textY, textW, textH);
		drawSelection(*t, textX, textY, textW, textH);
		drawTextLines(*t, textX, textY, textW, textH);
		drawGutter(*t, textY, textH);
		drawCursor(*t, textX, textY, textH);
		drawScrollBars(*t);
		drawCompletionPopup(*t, textX, textY, textH);
		drawSearchPanel(*t);
	}

	void CodeEditor::drawTabBar()
	{
		m_Renderer->drawRect(m_vpX, m_vpY, m_vpW, m_TabBarH, m_Theme.tabBg);

		constexpr float kPadLeft = 12.0f;
		constexpr float kPadRight = 28.0f;
		constexpr float kCloseSize = 16.0f;

		const bool mouseInTabBar = m_MouseY >= m_vpY && m_MouseY <= m_vpY + m_TabBarH && m_MouseX >= m_vpX && m_MouseX <= m_vpX + m_vpW;

		int hoveredTab = -1;
		int hoveredClose = -1;

		float tx = m_vpX + 8.0f;
		for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i)
		{
			Tab& tab = *m_Tabs[i];
			const float titleW = m_Renderer->measureText(tab.title, m_CharScale);
			const float w = titleW + kPadLeft + kPadRight + m_CloseButtonXOffset;

			if (mouseInTabBar && m_MouseX >= tx && m_MouseX <= tx + w)
			{
				hoveredTab = i;

				const float closeX = tx + w - kCloseSize - 6.0f;
				const float closeY = m_vpY + (m_TabBarH - kCloseSize) * 0.5f;
				if (m_MouseX >= closeX - 4.0f && m_MouseX <= closeX + kCloseSize + 4.0f && m_MouseY >= closeY && m_MouseY <= closeY + kCloseSize)
				{
					hoveredClose = i;
				}
			}

			tx += w;
		}

		tx = m_vpX + 8.0f;
		for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i)
		{
			Tab& tab = *m_Tabs[i];
			const float titleW = m_Renderer->measureText(tab.title, m_CharScale);
			const float w = titleW + kPadLeft + kPadRight + m_CloseButtonXOffset;
			Color bg;
			if (i == m_Active)
				bg = m_Theme.tabActive;
			else if (i == hoveredTab)
				bg = { m_Theme.tabActive.r * 0.7f, m_Theme.tabActive.g * 0.7f, m_Theme.tabActive.b * 0.7f, m_Theme.tabActive.a };
			else
				bg = m_Theme.tabBg;

			m_Renderer->drawRect(tx, m_vpY, w, m_TabBarH, bg);

			std::u32string title = tab.title;
			if (tab.dirty) title += U" *";
			m_Renderer->drawText(title, tx + kPadLeft, m_vpY + 7.0f, m_CharScale, m_Theme.text);
			const float closeX = tx + w - kCloseSize - 6.0f;
			const float closeY = m_vpY + (m_TabBarH - kCloseSize) * 0.5f;

			if (i == hoveredClose)
			{
				Color btnBg = { 0.30f, 0.30f, 0.32f, 0.9f };
				m_Renderer->drawRect(closeX - 2.0f, closeY - 2.0f, kCloseSize + 4.0f, kCloseSize + 4.0f, btnBg);
			}

			const float pad = 4.0f;
			const float x0 = closeX + pad;
			const float y0 = closeY + pad;
			const float x1 = closeX + kCloseSize - pad;
			const float y1 = closeY + kCloseSize - pad;

			Color crossColor = (i == hoveredClose) ? Color { 1.0f, 1.0f, 1.0f, 1.0f } : m_Theme.gutterText;

			m_Renderer->drawLine(x0, y0, x1, y1, 1.5f, crossColor);
			m_Renderer->drawLine(x1, y0, x0, y1, 1.5f, crossColor);

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

	void CodeEditor::drawSelection(Tab& t, float textX, float textY, float textW, float textH)
	{
		(void)textW;
		if (t.selection.empty()) return;

		const Position s = t.selection.start();
		const Position e = t.selection.end();

		for (int line = s.line; line <= e.line; ++line)
		{
			const float y = textY + static_cast<float>(line) * m_LineHeight - static_cast<float>(t.scrollY);
			if (y + m_LineHeight < textY) continue;
			if (y > textY + textH)       break;

			const int lineLen = static_cast<int>(t.buffer.line(line).size());
			const int colStart = (line == s.line) ? s.col : 0;
			const bool addTrailingSpace = (line != e.line);

			int colEnd;
			if (line == e.line)	colEnd = e.col;
			else				colEnd = lineLen;

			const float x0 = textX + colToX(t, line, colStart) - static_cast<float>(t.scrollX);

			float x1;
			if (addTrailingSpace)
				x1 = textX + colToX(t, line, lineLen) + m_Renderer->measureText(U" ", m_CharScale) - static_cast<float>(t.scrollX);
			else
				x1 = textX + colToX(t, line, colEnd) - static_cast<float>(t.scrollX);

			if (x1 > x0)
				m_Renderer->drawRect(x0, y, x1 - x0, m_LineHeight, m_Theme.selection);
		}
	}

	void CodeEditor::drawTextLines(Tab& t, float textX, float textY, float textW, float textH)
	{
		(void)textW;

		const int first = std::max(0, static_cast<int>(t.scrollY / m_LineHeight));
		const int visLines = static_cast<int>(textH / m_LineHeight) + 2;
		const int last = std::min(t.buffer.lineCount() - 1, first + visLines);

		for (int line = first; line <= last; ++line)
		{
			const float y = textY + static_cast<float>(line) * m_LineHeight - static_cast<float>(t.scrollY);
			float x = textX - static_cast<float>(t.scrollX);

			const auto& tokens = t.highlighter.tokens(line);
			const auto  text = t.buffer.line(line);

			int cursor = 0;
			for (const auto& tok : tokens)
			{
				if (tok.start > cursor)
				{
					const auto chunk = text.substr(cursor, tok.start - static_cast<std::basic_string_view<char32_t, std::char_traits<char32_t>>::size_type>(cursor));
					m_Renderer->drawText(chunk, x, y, m_CharScale, m_Theme.text);
					x += m_Renderer->measureText(chunk, m_CharScale);
				}
				const auto chunk = text.substr(tok.start, static_cast<std::basic_string_view<char32_t, std::char_traits<char32_t>>::size_type>(tok.end) - tok.start);
				m_Renderer->drawText(chunk, x, y, m_CharScale, m_Theme.of(tok.type), m_Theme.fontOf(tok.type));
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
	
	void CodeEditor::triggerCompletion(bool force)
	{
		Tab* t = activeTab();
		if (!t) return;

		if (!t->selection.empty()) { cancelCompletion(); return; }

		const Position cur = t->selection.active();
		CompletionResult r = m_Completion.complete(t->buffer, t->symbols, cur, force);

		if (!r.valid)
		{
			cancelCompletion();
			return;
		}

		if (r.items.size() == 1)
		{
			const int n = r.replaceEnd - r.replaceStart;
			if ((int)r.items[0].label.size() == n && r.items[0].label == r.items[0].insertText)
			{
				cancelCompletion();
				return;
			}
		}

		m_Comp.active = true;
		m_Comp.items = std::move(r.items);
		m_Comp.selected = 0;
		m_Comp.replaceStart = r.replaceStart;
		m_Comp.replaceEnd = r.replaceEnd;

		const float lineY = m_vpY + m_TabBarH + static_cast<float>(cur.line) * m_LineHeight - static_cast<float>(t->scrollY);
		const float caretX = m_vpX + m_GutterWidth + colToX(*t, cur.line, r.replaceStart) - static_cast<float>(t->scrollX);

		m_Comp.popupX = caretX;
		m_Comp.popupY = lineY + m_LineHeight;
	}

	void CodeEditor::cancelCompletion()
	{
		m_Comp.active = false;
		m_Comp.items.clear();
		m_Comp.selected = 0;
	}

	void CodeEditor::moveCompletion(int dir)
	{
		if (!m_Comp.active || m_Comp.items.empty()) return;
		const int n = static_cast<int>(m_Comp.items.size());
		m_Comp.selected = ((m_Comp.selected + dir) % n + n) % n;
	}

	void CodeEditor::acceptCompletion()
	{
		Tab* t = activeTab();
		if (!t || !m_Comp.active || m_Comp.items.empty())
		{
			cancelCompletion();
			return;
		}

		const auto& item = m_Comp.items[m_Comp.selected];
		const Position cur = t->selection.active();

		Position from { cur.line, m_Comp.replaceStart };
		Position to { cur.line, m_Comp.replaceEnd };
		applyEdit(*t, from, to, item.insertText);

		cancelCompletion();
	}

	void CodeEditor::drawCompletionPopup(Tab& t, float textX, float textY, float textH)
	{
		(void)t; (void)textX; (void)textY; (void)textH;
		if (!m_Comp.active || m_Comp.items.empty()) return;
		if (!m_Renderer) return;

		constexpr float rowH = 20.0f;
		constexpr float padX = 8.0f;
		constexpr float popupW = 260.0f;
		constexpr int   maxRows = 12;

		const int n = static_cast<int>(m_Comp.items.size());
		const int rows = std::min(n, maxRows);

		int first = 0;
		if (m_Comp.selected >= maxRows)
			first = m_Comp.selected - maxRows + 1;
		if (first > n - maxRows) first = std::max(0, n - maxRows);

		const float popupH = rows * rowH + 4.0f;

		float px = m_Comp.popupX;
		float py = m_Comp.popupY;
		if (py + popupH > m_vpY + m_vpH)
			py = m_Comp.popupY - m_LineHeight - popupH;

		if (px + popupW > m_vpX + m_vpW)
			px = m_vpX + m_vpW - popupW - 2.0f;

		Color bg { 0.15f, 0.15f, 0.17f, 0.97f };
		Color border { 0.30f, 0.30f, 0.34f, 1.0f };
		Color selBg { 0.22f, 0.36f, 0.55f, 1.0f };
		Color detailColor { 0.55f, 0.55f, 0.55f, 1.0f };

		m_Renderer->drawRect(px - 1, py - 1, popupW + 2, popupH + 2, border);
		m_Renderer->drawRect(px, py, popupW, popupH, bg);

		for (int i = 0; i < rows; ++i)
		{
			const int idx = first + i;
			if (idx >= n) break;

			const float ry = py + 2.0f + i * rowH;

			if (idx == m_Comp.selected)
				m_Renderer->drawRect(px + 1, ry, popupW - 2, rowH, selBg);

			const auto& item = m_Comp.items[idx];

			Color nameColor;
			switch (item.kind)
			{
			case CompletionKind::Keyword:
			case CompletionKind::Control:	nameColor = { 0.78f, 0.44f, 0.85f, 1.0f }; break;
			case CompletionKind::Type:		nameColor = { 0.30f, 0.79f, 0.78f, 1.0f }; break;
			case CompletionKind::Function:	nameColor = { 0.86f, 0.86f, 0.66f, 1.0f }; break;
			case CompletionKind::Class:
			case CompletionKind::Struct:
			case CompletionKind::Enum:		nameColor = { 0.30f, 0.79f, 0.78f, 1.0f }; break;
			case CompletionKind::Namespace:	nameColor = { 0.85f, 0.85f, 0.85f, 1.0f }; break;
			case CompletionKind::EnumValue:	nameColor = { 0.71f, 0.80f, 0.66f, 1.0f }; break;
			default:						nameColor = { 0.90f, 0.90f, 0.90f, 1.0f }; break;
			}

			m_Renderer->drawText(item.label, px + padX, ry + 3.0f, 1.0f, nameColor, FontStyle::Regular);

			if (!item.detail.empty())
			{
				const float labelW = m_Renderer->measureText(item.label, 1.0f);
				const float detailX = px + padX + labelW + 12.0f;

				const float detailW = m_Renderer->measureText(item.detail, 1.0f);
				if (detailX + detailW < px + popupW - 4.0f)
				{
					m_Renderer->drawText(item.detail, detailX, ry + 3.0f, 1.0f, detailColor, FontStyle::Regular);
				}
			}
		}

		if (n > maxRows)
		{
			const float trackW = 4.0f;
			const float trackX = px + popupW - trackW - 2.0f;
			const float trackY = py + 2.0f;
			const float trackH = rows * rowH;

			m_Renderer->drawRect(trackX, trackY, trackW, trackH, { 0.20f, 0.20f, 0.22f, 0.5f });

			const float thumbH = std::max(20.0f, trackH * static_cast<float>(rows) / n);
			const float t = (n > rows) ? static_cast<float>(first) / (n - rows) : 0.0f;
			const float thumbY = trackY + t * (trackH - thumbH);

			m_Renderer->drawRect(trackX, thumbY, trackW, thumbH, { 0.45f, 0.45f, 0.50f, 0.9f });
		}
	}

	void CodeEditor::doCopy(Tab& t)
	{
		if (t.selection.empty()) return;
		if (!m_SetClipboard) return;

		const std::u32string txt = t.buffer.getText(t.selection.start(), t.selection.end());
		if (txt.empty()) return;

		m_SetClipboard(u32_to_utf8(txt));
	}

	void CodeEditor::doCut(Tab& t)
	{
		if (t.selection.empty()) return;
		doCopy(t);
		deleteSelection(t);
	}

	void CodeEditor::doPaste(Tab& t)
	{
		if (!m_GetClipboard) return;

		const std::string utf8 = m_GetClipboard();
		if (utf8.empty()) return;

		std::u32string raw = utf8_to_u32(utf8);

		std::u32string text;
		text.reserve(raw.size());
		for (size_t i = 0; i < raw.size(); ++i)
		{
			if (raw[i] == U'\r')
			{
				if (i + 1 < raw.size() && raw[i + 1] == U'\n')
					continue;
				text += U'\n';
			}
			else
			{
				text += raw[i];
			}
		}

		std::u32string cleaned;
		cleaned.reserve(text.size());
		for (char32_t c : text)
		{
			if (c == U'\n' || c == U'\t') { cleaned += c; continue; }
			if (c < 32) continue;
			cleaned += c;
		}

		if (cleaned.empty()) return;
		insertText(t, cleaned);
	}

	NavLocation CodeEditor::captureNavLocation()
	{
		NavLocation loc;
		Tab* t = activeTab();
		if (!t) return loc;
		loc.tab = t;
		loc.pos = t->selection.active();
		loc.scrollX = t->scrollX;
		loc.scrollY = t->scrollY;
		return loc;
	}

	void CodeEditor::goToDefinition()
	{
		Tab* t = activeTab();
		if (!t) return;

		const Position cursor = t->selection.active();
		auto line = t->buffer.line(cursor.line);
		if (line.empty()) return;

		int s = std::clamp(cursor.col, 0, static_cast<int>(line.size()));
		int e = s;
		while (s > 0 && CompletionEngine::isIdentifierChar(line[static_cast<size_t>(s) - 1])) --s;
		while (e < static_cast<int>(line.size()) && CompletionEngine::isIdentifierChar(line[e])) ++e;
		if (s == e) return;

		const std::u32string name(line.substr(s, e - s));

		if (const Symbol* sym = t->symbols.findSymbolAnywhere(name))
		{
			jumpTo(t, sym->line, sym->col);
			return;
		}

		if (m_ProjectIndex && !t->path.empty() && m_ProjectIndex->IsIndexed(t->path))
		{
			auto matches = m_ProjectIndex->FindSymbol(name, t->path, 8);
			if (!matches.empty())
			{
				const SymbolMatch& m = matches[0];

				if (Tab* target = FindTabByPath(m.filePath))
				{
					jumpTo(target, m.symbol->line, m.symbol->col);
					return;
				}

				NavLocation from;
				from.tab = t;
				from.pos = cursor;
				from.scrollX = t->scrollX;
				from.scrollY = t->scrollY;
				m_Nav.push(from);

				Tab* target = openFile(m.filePath);
				if (!target) return;

				Position p{ m.symbol->line, m.symbol->col };
				p = target->buffer.clamp(p);
				target->selection.clear(p);
				target->desiredCol = p.col;
				ensureCursorVisible(*target);

				m_Nav.push(captureNavLocation());
				return;
			}
		}

		for (auto& other : m_Tabs)
		{
			if (other.get() == t) continue;
			if (const Symbol* sym = other->symbols.findSymbolAnywhere(name))
			{
				jumpTo(other.get(), sym->line, sym->col);
				return;
			}
		}
	}

	Tab* CodeEditor::FindTabByPath(const std::string& path)
	{
		if (path.empty()) return nullptr;
		for (auto& t : m_Tabs)
		{
			if (t->path == path) return t.get();
		}
		return nullptr;
	}

	void CodeEditor::navBack()
	{
		NavLocation target;
		if (!m_Nav.back(captureNavLocation(), target)) return;
		if (!target.tab) return;

		for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i)
		{
			if (m_Tabs[i].get() == target.tab)
			{
				m_Active = i;
				break;
			}
		}

		target.tab->selection.clear(target.pos);
		target.tab->desiredCol = target.pos.col;
		target.tab->scrollX = target.scrollX;
		target.tab->scrollY = target.scrollY;
		ensureCursorVisible(*target.tab);
	}

	void CodeEditor::navForward()
	{
		NavLocation target;
		if (!m_Nav.forward(captureNavLocation(), target)) return;
		if (!target.tab) return;

		for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i)
		{
			if (m_Tabs[i].get() == target.tab)
			{
				m_Active = i;
				break;
			}
		}

		target.tab->selection.clear(target.pos);
		target.tab->desiredCol = target.pos.col;
		target.tab->scrollX = target.scrollX;
		target.tab->scrollY = target.scrollY;
		ensureCursorVisible(*target.tab);
	}

	void CodeEditor::jumpToMatchingBrace(Tab& t, bool forward)
	{
		const Position cur = t.selection.active();
		auto line = t.buffer.line(cur.line);
		if (cur.col >= static_cast<int>(line.size())) return;

		const char32_t c = line[cur.col];

		char32_t open = 0, close = 0;
		if (forward)
		{
			if (c == U'{') { open = U'{'; close = U'}'; }
			else if (c == U'(') { open = U'('; close = U')'; }
			else if (c == U'[') { open = U'['; close = U']'; }
			else return;
		}
		else
		{
			if (c == U'}') { open = U'{'; close = U'}'; }
			else if (c == U')') { open = U'('; close = U')'; }
			else if (c == U']') { open = U'['; close = U']'; }
			else return;
		}

		const int lineCount = t.buffer.lineCount();
		int depth = 0;

		if (forward)
		{
			for (int l = cur.line; l < lineCount; ++l)
			{
				auto ln = t.buffer.line(l);
				const int startCol = (l == cur.line) ? cur.col : 0;
				for (int i = startCol; i < static_cast<int>(ln.size()); ++i)
				{
					if (ln[i] == open)  ++depth;
					else if (ln[i] == close)
					{
						--depth;
						if (depth == 0)
						{
							t.selection.clear(Position{ l, i });
							t.desiredCol = i;
							ensureCursorVisible(t);
							return;
						}
					}
				}
			}
		}
		else
		{
			for (int l = cur.line; l >= 0; --l)
			{
				auto ln = t.buffer.line(l);
				const int startCol = (l == cur.line) ? std::min(cur.col, static_cast<int>(ln.size()) - 1) : static_cast<int>(ln.size()) - 1;
				for (int i = startCol; i >= 0; --i)
				{
					if (ln[i] == close) ++depth;
					else if (ln[i] == open)
					{
						--depth;
						if (depth == 0)
						{
							t.selection.clear(Position{ l, i });
							t.desiredCol = i;
							ensureCursorVisible(t);
							return;
						}
					}
				}
			}
		}
	}

	void CodeEditor::updateReferenceHighlights(Tab& t)
	{
		const Position cur = t.selection.active();

		if (cur == m_LastCursorPos && m_RefHighlight.active) return;
		m_LastCursorPos = cur;

		m_RefHighlight.occurrences.clear();
		m_RefHighlight.active = false;

		auto line = t.buffer.line(cur.line);
		if (line.empty()) return;

		int s = std::clamp(cur.col, 0, static_cast<int>(line.size()));
		int e = s;
		while (s > 0 && CompletionEngine::isIdentifierChar(line[static_cast<std::basic_string_view<char32_t, std::char_traits<char32_t>>::size_type>(s) - 1])) --s;
		while (e < static_cast<int>(line.size()) && CompletionEngine::isIdentifierChar(line[e])) ++e;
		if (s == e || e - s < 2) return;

		std::u32string name(line.substr(s, e - s));

		const int lineCount = t.buffer.lineCount();
		for (int i = 0; i < lineCount; ++i)
		{
			auto l = t.buffer.line(i);
			const int len = static_cast<int>(l.size());
			int j = 0;
			while (j < len)
			{
				if (!CompletionEngine::isIdentifierChar(l[j])) { ++j; continue; }
				const int a = j;
				while (j < len && CompletionEngine::isIdentifierChar(l[j])) ++j;
				if (static_cast<int>(name.size()) == j - a && std::u32string_view(l).substr(a, static_cast<std::basic_string_view<char32_t, std::char_traits<char32_t>>::size_type>(j) - a) == name)
				{
					m_RefHighlight.occurrences.push_back({ i, { a, j } });
				}
			}
		}

		if (m_RefHighlight.occurrences.size() > 1)
			m_RefHighlight.active = true;
		else
			m_RefHighlight.occurrences.clear();
	}

	void CodeEditor::drawReferenceHighlights(Tab& t, float textX, float textY, float textW, float textH)
	{
		(void)textW;
		if (!m_RefHighlight.active) return;

		const Color hl { 0.28f, 0.28f, 0.32f, 0.85f };

		for (const auto& [line, range] : m_RefHighlight.occurrences)
		{
			const float y = textY + static_cast<float>(line) * m_LineHeight - static_cast<float>(t.scrollY);
			if (y + m_LineHeight < textY) continue;
			if (y > textY + textH) break;

			const float x0 = textX + colToX(t, line, range.first) - static_cast<float>(t.scrollX);
			const float x1 = textX + colToX(t, line, range.second) - static_cast<float>(t.scrollX);

			if (x1 > x0)
				m_Renderer->drawRect(x0, y, x1 - x0, m_LineHeight, hl);
		}
	}

	bool CodeEditor::saveTab(Tab& t, bool saveAs)
	{
		std::string path = t.path;

		if (saveAs || path.empty())
		{
			if (!m_SaveAsDialog) return false;
			path = m_SaveAsDialog();
			if (path.empty()) return false;

			t.path = path;
			const size_t slash = path.find_last_of("/\\");
			const std::string base = (slash == std::string::npos)
				? path : path.substr(slash + 1);
			t.title = utf8_to_u32(base);
		}

		std::string out;
		out.reserve(1024);
		const int n = t.buffer.lineCount();
		for (int i = 0; i < n; ++i)
		{
			out += u32_to_utf8(t.buffer.line(i));
			if (i + 1 < n) out += '\n';
		}

		std::ofstream file(path, std::ios::binary | std::ios::trunc);
		if (!file) return false;
		file.write(out.data(), static_cast<std::streamsize>(out.size()));
		if (!file) return false;

		t.dirty = false;
		return true;
	}

	void CodeEditor::jumpTo(Tab* target, int line, int col)
	{
		if (!target) return;

		m_Nav.push(captureNavLocation());

		for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i)
		{
			if (m_Tabs[i].get() == target)
			{
				m_Active = i;
				break;
			}
		}

		Position p { line, col };
		p = target->buffer.clamp(p);
		target->selection.clear(p);
		target->desiredCol = p.col;
		ensureCursorVisible(*target);

		m_Nav.push(captureNavLocation());
	}

	void CodeEditor::openSearch(bool replaceMode)
	{
		m_Search.active = true;
		m_Search.replaceMode = replaceMode;
		m_Search.focusedField = 0;

		Tab* t = activeTab();
		if (t && !t->selection.empty())
		{
			const Position s = t->selection.start();
			const Position e = t->selection.end();
			if (s.line == e.line && e.col > s.col)
			{
				m_Search.query = std::u32string(t->buffer.line(s.line).substr(s.col, e.col - s.col));
				m_Search.queryCursor = (int)m_Search.query.size();
			}
		}

		if (t) performSearch(*t);
	}

	void CodeEditor::closeSearch()
	{
		m_Search.active = false;
		m_Search.matches.clear();
		m_Search.currentMatch = -1;
	}

	void CodeEditor::performSearch(Tab& t)
	{
		m_Search.matches.clear();
		m_Search.currentMatch = -1;

		if (m_Search.query.empty()) return;

		const bool cs = m_Search.caseSensitive;
		const bool ww = m_Search.wholeWord;

		if (m_Search.usePattern)
		{
			for (int i = 0; i < t.buffer.lineCount(); ++i)
			{
				auto line = t.buffer.line(i);
				const int lineLen = (int)line.size();
				for (int j = 0; j < lineLen; ++j)
				{
					int len = globMatchLen(line, j, lineLen, m_Search.query, 0, cs);
					if (len <= 0) continue;
					if (ww && !isWordBoundary(line, j, len)) continue;
					m_Search.matches.push_back({ i, j, len });
				}
			}
		}
		else
		{
			const int qlen = (int)m_Search.query.size();
			for (int i = 0; i < t.buffer.lineCount(); ++i)
			{
				auto line = t.buffer.line(i);
				const int lineLen = (int)line.size();
				if (lineLen < qlen) continue;

				for (int j = 0; j + qlen <= lineLen; ++j)
				{
					bool ok = true;
					for (int k = 0; k < qlen; ++k)
					{
						char32_t tc = line[static_cast<std::basic_string_view<char32_t, std::char_traits<char32_t>>::size_type>(j) + k];
						char32_t qc = m_Search.query[k];
						if (!cs) { tc = toLowerUnicode(tc); qc = toLowerUnicode(qc); }
						if (tc != qc) { ok = false; break; }
					}
					if (!ok) continue;
					if (ww && !isWordBoundary(line, j, qlen)) continue;
					m_Search.matches.push_back({ i, j, qlen });
				}
			}
		}

		if (!m_Search.matches.empty()) m_Search.currentMatch = 0;
	}

	void CodeEditor::gotoMatch(Tab& t, int idx)
	{
		if (idx < 0 || idx >= (int)m_Search.matches.size()) return;
		m_Search.currentMatch = idx;

		const auto& m = m_Search.matches[idx];
		Position from{ m.line, m.col };
		Position to{ m.line, m.col + m.len };
		t.selection.set(from, to);
		t.desiredCol = m.col;
		ensureCursorVisible(t);
	}

	void CodeEditor::gotoNextMatch(Tab& t)
	{
		if (m_Search.matches.empty()) return;
		const int n = (int)m_Search.matches.size();
		gotoMatch(t, (m_Search.currentMatch + 1) % n);
	}

	void CodeEditor::gotoPrevMatch(Tab& t)
	{
		if (m_Search.matches.empty()) return;
		const int n = (int)m_Search.matches.size();
		gotoMatch(t, (m_Search.currentMatch - 1 + n) % n);
	}

	void CodeEditor::replaceCurrentMatch(Tab& t)
	{
		if (m_Search.currentMatch < 0) return;
		if (m_Search.currentMatch >= (int)m_Search.matches.size()) return;

		const auto& m = m_Search.matches[m_Search.currentMatch];
		Position from { m.line, m.col };
		Position to { m.line, m.col + m.len };
		applyEdit(t, from, to, m_Search.replaceText);

		performSearch(t);

		if (m_Search.currentMatch < (int)m_Search.matches.size() - 1)
		{
			m_Search.currentMatch++;
			gotoMatch(t, m_Search.currentMatch);
		}
	}

	void CodeEditor::replaceAllMatches(Tab& t)
	{
		if (m_Search.matches.empty()) return;

		for (int i = (int)m_Search.matches.size() - 1; i >= 0; --i)
		{
			const auto& m = m_Search.matches[i];
			Position from { m.line, m.col };
			Position to { m.line, m.col + m.len };
			applyEdit(t, from, to, m_Search.replaceText);
		}

		performSearch(t);
	}

	void CodeEditor::searchInputChar(char32_t c)
	{
		if (m_Search.focusedField == 0)
		{
			m_Search.query.insert(m_Search.query.begin() + m_Search.queryCursor, c);
			m_Search.queryCursor++;
		}
		else
		{
			m_Search.replaceText.insert(m_Search.replaceText.begin() + m_Search.replaceCursor, c);
			m_Search.replaceCursor++;
		}

		if (Tab* t = activeTab()) performSearch(*t);
	}

	void CodeEditor::searchInputBackspace()
	{
		if (m_Search.focusedField == 0)
		{
			if (m_Search.queryCursor <= 0) return;
			m_Search.query.erase(m_Search.query.begin() + m_Search.queryCursor - 1);
			m_Search.queryCursor--;
		}
		else
		{
			if (m_Search.replaceCursor <= 0) return;
			m_Search.replaceText.erase(m_Search.replaceText.begin() + m_Search.replaceCursor - 1);
			m_Search.replaceCursor--;
		}
		if (Tab* t = activeTab()) performSearch(*t);
	}

	void CodeEditor::searchInputDelete()
	{
		if (m_Search.focusedField == 0)
		{
			if (m_Search.queryCursor >= (int)m_Search.query.size()) return;
			m_Search.query.erase(m_Search.query.begin() + m_Search.queryCursor);
		}
		else
		{
			if (m_Search.replaceCursor >= (int)m_Search.replaceText.size()) return;
			m_Search.replaceText.erase(m_Search.replaceText.begin() + m_Search.replaceCursor);
		}
		if (Tab* t = activeTab()) performSearch(*t);
	}

	void CodeEditor::searchInputMoveLeft()
	{
		if (m_Search.focusedField == 0)
		{
			if (m_Search.queryCursor > 0) m_Search.queryCursor--;
		}
		else
		{
			if (m_Search.replaceCursor > 0) m_Search.replaceCursor--;
		}
	}

	void CodeEditor::searchInputMoveRight()
	{
		if (m_Search.focusedField == 0)
		{
			if (m_Search.queryCursor < (int)m_Search.query.size()) m_Search.queryCursor++;
		}
		else
		{
			if (m_Search.replaceCursor < (int)m_Search.replaceText.size()) m_Search.replaceCursor++;
		}
	}

	bool CodeEditor::handleSearchClick(float x, float y)
	{
		if (!m_Search.active) return false;

		const bool inPanel = x >= m_Search.panelX && x <= m_Search.panelX + m_Search.panelW && y >= m_Search.panelY && y <= m_Search.panelY + m_Search.panelH;

		if (!inPanel)
		{
			closeSearch();
			return false;
		}

		Tab* t = activeTab();
		if (!t) return true;

		if (x >= m_Search.queryBoxX && x <= m_Search.queryBoxX + m_Search.queryBoxW && y >= m_Search.queryBoxY && y <= m_Search.queryBoxY + m_Search.queryBoxH)
		{
			m_Search.focusedField = 0;
			return true;
		}

		if (m_Search.replaceMode && x >= m_Search.replaceBoxX && x <= m_Search.replaceBoxX + m_Search.replaceBoxW && y >= m_Search.replaceBoxY && y <= m_Search.replaceBoxY + m_Search.replaceBoxH)
		{
			m_Search.focusedField = 1;
			return true;
		}

		if (x >= m_Search.closeBtnX && x <= m_Search.closeBtnX + m_Search.closeBtnSize && y >= m_Search.closeBtnY && y <= m_Search.closeBtnY + m_Search.closeBtnSize)
		{
			closeSearch();
			return true;
		}

		if (y >= m_Search.btnY && y <= m_Search.btnY + m_Search.btnSize)
		{
			if (x >= m_Search.nextBtnX && x <= m_Search.nextBtnX + m_Search.btnSize)
			{
				gotoNextMatch(*t); return true;
			}
			if (x >= m_Search.prevBtnX && x <= m_Search.prevBtnX + m_Search.btnSize)
			{
				gotoPrevMatch(*t); return true;
			}
			if (x >= m_Search.caseBtnX && x <= m_Search.caseBtnX + m_Search.btnSize)
			{
				m_Search.caseSensitive = !m_Search.caseSensitive;
				performSearch(*t);
				return true;
			}
		}

		if (m_Search.replaceMode && y >= m_Search.replaceBoxY && y <= m_Search.replaceBoxY + m_Search.replaceBoxH)
		{
			const float repW = 60.0f;
			if (x >= m_Search.replaceAllBtnX && x <= m_Search.replaceAllBtnX + repW)
			{
				replaceAllMatches(*t); return true;
			}
			if (x >= m_Search.replaceBtnX && x <= m_Search.replaceBtnX + repW)
			{
				replaceCurrentMatch(*t); return true;
			}
		}

		return true;
	}

	void CodeEditor::drawSearchHighlights(Tab& t, float textX, float textY, float textW, float textH)
	{
		(void)textW;
		if (!m_Search.active || m_Search.matches.empty()) return;

		const Color matchColor { 0.55f, 0.45f, 0.15f, 0.55f };
		const Color currentColor { 0.85f, 0.55f, 0.10f, 0.85f };

		for (int i = 0; i < (int)m_Search.matches.size(); ++i)
		{
			const auto& m = m_Search.matches[i];
			if (m.line < 0 || m.line >= t.buffer.lineCount()) continue;

			const float y = textY + (float)m.line * m_LineHeight - (float)t.scrollY;
			if (y + m_LineHeight < textY) continue;
			if (y > textY + textH) break;

			const float x0 = textX + colToX(t, m.line, m.col) - (float)t.scrollX;
			const float x1 = textX + colToX(t, m.line, m.col + m.len) - (float)t.scrollX;

			const Color c = (i == m_Search.currentMatch) ? currentColor : matchColor;
			if (x1 > x0)
				m_Renderer->drawRect(x0, y, x1 - x0, m_LineHeight, c);
		}
	}

	void CodeEditor::drawSearchBtn(float x, float y, float w, float h, std::u32string_view label, bool active)
	{
		const Color bg = active ? Color { 0.30f, 0.50f, 0.80f, 1.0f } : Color { 0.25f, 0.25f, 0.28f, 1.0f };
		const Color fg { 0.90f, 0.90f, 0.90f, 1.0f };

		m_Renderer->drawRect(x, y, w, h, bg);

		const float tw = m_Renderer->measureText(label, 1.0f);
		m_Renderer->drawText(label, x + (w - tw) * 0.5f, y + (h - m_LineHeight) * 0.5f + 1.0f, 1.0f, fg, FontStyle::Regular);
	}

	void CodeEditor::drawSearchPanel(Tab& t)
	{
		(void)t;
		if (!m_Search.active) return;
		if (!m_Renderer) return;

		std::string path = "Resources/Languages/" + LanguageManager::GetLanguageCode() + ".json";
		std::ifstream file(path);
		nlohmann::json j;
		file >> j;
	
		constexpr float panelW = 640.0f;
		const float panelH = m_Search.replaceMode ? 74.0f : 46.0f;
		const float panelX = m_vpX + (m_vpW - panelW) * 0.5f;
		const float panelY = m_vpY + m_vpH - panelH - 24.0f;
	
		m_Search.panelX = panelX;
		m_Search.panelY = panelY;
		m_Search.panelW = panelW;
		m_Search.panelH = panelH;
	
		m_Renderer->drawRect(panelX - 1, panelY - 1, panelW + 2, panelH + 2, { 0.30f, 0.30f, 0.34f, 1.0f });
		m_Renderer->drawRect(panelX, panelY, panelW, panelH, { 0.18f, 0.18f, 0.20f, 0.98f });
	
		constexpr float padX = 10.0f;
		constexpr float rowH = 22.0f;
		constexpr float inputH = 22.0f;
		constexpr float labelW = 44.0f;
		constexpr float btnSize = 22.0f;
		constexpr float btnGap = 4.0f;
		const float inputY0 = panelY + 10.0f;
		const float inputY1 = inputY0 + rowH + 6.0f;
	
		const float inputX = panelX + padX + labelW;
		const float buttonsW = btnSize * 6 + btnGap * 5 + 8.0f;
		const float inputW = panelW - padX * 2 - labelW - buttonsW;
	
		m_Search.queryBoxX = inputX;
		m_Search.queryBoxY = inputY0;
		m_Search.queryBoxW = inputW;
		m_Search.queryBoxH = inputH;
	
		m_Search.replaceBoxX = inputX;
		m_Search.replaceBoxY = inputY1;
		m_Search.replaceBoxW = inputW;
		m_Search.replaceBoxH = inputH;
	
		m_Search.btnSize = btnSize;
		m_Search.btnY = inputY0 + (inputH - btnSize) * 0.5f;
	
		float bx = panelX + panelW - padX - btnSize;
		m_Search.closeBtnX = bx;
		m_Search.closeBtnY = m_Search.btnY;
		bx -= btnSize + btnGap; m_Search.nextBtnX = bx;
		bx -= btnSize + btnGap; m_Search.prevBtnX = bx;
		bx -= btnSize + btnGap; m_Search.patternBtnX = bx;
		bx -= btnSize + btnGap; m_Search.wordBtnX = bx;
		bx -= btnSize + btnGap; m_Search.caseBtnX = bx;
	
		const float textY0 = inputY0 + (inputH - m_LineHeight) * 0.5f;
		const float textY1 = inputY1 + (inputH - m_LineHeight) * 0.5f;
	
		m_Renderer->drawText(utf8_to_u32(j.value("Find", "Find")), panelX + padX, textY0, 1.0f, m_Theme.text, FontStyle::Regular);
		if (m_Search.replaceMode)
			m_Renderer->drawText(utf8_to_u32(j.value("Replace", "Replace")), panelX + padX, textY1, 1.0f, m_Theme.text, FontStyle::Regular);
	
		auto drawInputBox = [&](float ix, float iy, float iw, float ih,
			const std::u32string& text, int cursor, bool focused)
			{
				const Color boxBg{ 0.10f, 0.10f, 0.12f, 1.0f };
				const Color border = focused ? Color{ 0.35f, 0.60f, 0.95f, 1.0f } : Color{ 0.25f, 0.25f, 0.28f, 1.0f };
	
				m_Renderer->drawRect(ix, iy, iw, ih, border);
				m_Renderer->drawRect(ix + 1, iy + 1, iw - 2, ih - 2, boxBg);
	
				const float ty = iy + (ih - m_LineHeight) * 0.5f;
				m_Renderer->drawText(text, ix + 6.0f, ty, 1.0f, m_Theme.text, FontStyle::Regular);
	
				if (focused && m_CursorVisible)
				{
					const float cw = m_Renderer->measureText(std::u32string_view(text).substr(0, cursor), 1.0f);
					m_Renderer->drawRect(ix + 6.0f + cw, iy + 3.0f, 1.5f, ih - 6.0f, m_Theme.cursor);
				}
			};
	
		drawInputBox(m_Search.queryBoxX, m_Search.queryBoxY, m_Search.queryBoxW, m_Search.queryBoxH, m_Search.query, m_Search.queryCursor, m_Search.focusedField == 0);
	
		if (m_Search.replaceMode)
			drawInputBox(m_Search.replaceBoxX, m_Search.replaceBoxY, m_Search.replaceBoxW, m_Search.replaceBoxH, m_Search.replaceText, m_Search.replaceCursor, m_Search.focusedField == 1);
	
		drawSearchBtn(m_Search.closeBtnX, m_Search.closeBtnY, btnSize, btnSize, U"×", false);
		drawSearchBtn(m_Search.nextBtnX, m_Search.btnY, btnSize, btnSize, U"↓", false);
		drawSearchBtn(m_Search.prevBtnX, m_Search.btnY, btnSize, btnSize, U"↑", false);
		drawSearchBtn(m_Search.patternBtnX, m_Search.btnY, btnSize, btnSize, U".*", m_Search.usePattern);
		drawSearchBtn(m_Search.wordBtnX, m_Search.btnY, btnSize, btnSize, U"W", m_Search.wholeWord);
		drawSearchBtn(m_Search.caseBtnX, m_Search.btnY, btnSize, btnSize, U"Aa", m_Search.caseSensitive);
	
		if (m_Search.replaceMode)
		{
			constexpr float repW = 65.0f;
			constexpr float allW = 80.0f;
			const float repY = inputY1 + 1.0f;
			const float repH = inputH - 2.0f;
	
			float rbx = panelX + panelW - padX - allW;
			m_Search.replaceAllBtnX = rbx;
			drawSearchBtn(rbx, repY, allW, repH, utf8_to_u32(j.value("ReplaceAll", "Replace All")), false);
	
			rbx -= repW + 8.0f;
			m_Search.replaceBtnX = rbx;
			drawSearchBtn(rbx, repY, repW, repH, utf8_to_u32(j.value("Replace", "Replace")), false);
		}
	
		const int total = (int)m_Search.matches.size();
		const int cur = (total > 0) ? (m_Search.currentMatch + 1) : 0;
	
		std::u32string counter = utf8_to_u32(std::to_string(cur) + " / " + std::to_string(total));
	
		const float counterW = m_Renderer->measureText(counter, 1.0f);
		const float counterX = m_Search.queryBoxX + m_Search.queryBoxW - counterW - 6.0f;
	
		if (total == 0 && !m_Search.query.empty())
			m_Renderer->drawText(counter, counterX, textY0, 1.0f, Color { 0.90f, 0.40f, 0.40f, 1.0f }, FontStyle::Regular);
		else
			m_Renderer->drawText(counter, counterX, textY0, 1.0f, m_Theme.gutterText, FontStyle::Regular);
	}

}
