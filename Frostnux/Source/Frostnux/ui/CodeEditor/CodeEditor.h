#pragma once
#include "Tab.h"

#include "Frostnux/Events/Event.h"
#include "Frostnux/Events/KeyEvent.h"
#include "Frostnux/Events/MouseEvent.h"

#include <vector>
#include <memory>
#include <string_view>

namespace Frostnux {

	class Renderer
	{
	public:
		virtual ~Renderer() = default;

		virtual void drawText(std::u32string_view text,
			float x, float y, float scale, Color color) = 0;

		virtual void drawRect(float x, float y, float w, float h, Color color) = 0;

		[[nodiscard]] virtual float measureText(std::u32string_view text,
			float scale) const = 0;

		[[nodiscard]] virtual float lineHeight(float scale) const = 0;
	};

	class CodeEditor
	{
	public:
		explicit CodeEditor(Renderer* r) : m_Renderer(r) {}
		~CodeEditor() = default;

		CodeEditor(const CodeEditor&) = delete;
		CodeEditor& operator=(const CodeEditor&) = delete;

		bool OnEvent(Event& e);

		Tab& addTab(const std::string& path = {});
		void closeTab(int idx);
		void switchTab(int dir);
		Tab* activeTab();
		[[nodiscard]] int activeIndex() const { return m_Active; }
		[[nodiscard]] int tabCount()    const { return static_cast<int>(m_Tabs.size()); }

		void update(double dt, double now);
		void render(float x, float y, float w, float h);

		[[nodiscard]] EditorTheme& theme() { return m_Theme; }
		[[nodiscard]] const EditorTheme& theme() const { return m_Theme; }
	private:
		bool onKeyPressed(KeyPressedEvent& e);
		bool onChar(CharEvent& e);
		bool onMouseButtonPressed(MouseButtonPressedEvent& e);
		bool onMouseButtonReleased(MouseButtonReleasedEvent& e);
		bool onMouseMoved(MouseMovedEvent& e);
		bool onMouseScrolled(MouseScrolledEvent& e);

		void layout(float x, float y, float w, float h);
		void syncBars(Tab& t);
		void applyScrollFromBars(Tab& t);
		void ensureCursorVisible(Tab& t);

		void applyEdit(Tab& t, Position from, Position to, std::u32string_view text);
		void insertText(Tab& t, std::u32string_view text);
		void deleteSelection(Tab& t);
		void backspace(Tab& t);
		void deleteForward(Tab& t);
		void newline(Tab& t);
		void tabKey(Tab& t, bool shift);
		void moveCursor(Tab& t, Position p, bool selecting);
		void moveLeft(Tab& t, bool selecting, bool byWord);
		void moveRight(Tab& t, bool selecting, bool byWord);
		void moveUp(Tab& t, bool selecting);
		void moveDown(Tab& t, bool selecting);
		void moveHome(Tab& t, bool selecting, bool docStart);
		void moveEnd(Tab& t, bool selecting, bool docEnd);
		void movePageUp(Tab& t, bool selecting);
		void movePageDown(Tab& t, bool selecting);
		void doUndo(Tab& t);
		void doRedo(Tab& t);

		[[nodiscard]] Position pixelToPosition(Tab& t, float x, float y) const;
		[[nodiscard]] float    colToX(const Tab& t, int line, int col) const;
		[[nodiscard]] int      xToCol(const Tab& t, int line, float x) const;

		void drawTabBar();
		void drawGutter(Tab& t, float textTop, float textH);
		void drawTextLines(Tab& t, float textX, float textY, float textW, float textH);
		void drawSelection(Tab& t, float textX, float textY, float textW, float textH);
		void drawCurrentLine(Tab& t, float textX, float textY, float textW);
		void drawCursor(Tab& t, float textX, float textY, float textH);
		void drawScrollBars(Tab& t);

		Renderer*	m_Renderer = nullptr;
		EditorTheme	m_Theme;

		std::vector<std::unique_ptr<Tab>> m_Tabs;
		int		m_Active = -1;

		float m_vpX = 0, m_vpY = 0, m_vpW = 0, m_vpH = 0;

		float m_TabBarH = 32.0f;
		float m_LineHeight = 20.0f;
		float m_GutterWidth = 60.0f;
		float m_ScrollBarSize = 14.0f;
		float m_CharScale = 1.0f;

		bool m_CursorVisible = true;
		bool m_Dragging = false;
	};

}
