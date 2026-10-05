#pragma once
#include "Tab.h"

#include "Frostnux/Events/Event.h"
#include "Frostnux/Events/KeyEvent.h"
#include "Frostnux/Events/MouseEvent.h"

#include <vector>
#include <memory>
#include <string_view>
#include <functional>
#include <string>

#include "Completion.h"
#include "Navigation.h"
#include "ProjectIndex.h"

namespace Frostnux {

	class Renderer
	{
	public:
		virtual ~Renderer() = default;

		virtual void drawText(std::u32string_view text, float x, float y, float scale, Color color, FontStyle style = FontStyle::Regular) = 0;

		virtual void drawRect(float x, float y, float w, float h, Color color) = 0;

		virtual void drawLine(float x1, float y1, float x2, float y2, float thickness, Color color) = 0;

		[[nodiscard]] virtual float measureText(std::u32string_view text, float scale) const = 0;

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
		Tab* openFile(const std::string& path);
		void closeTab(int idx);
		void switchTab(int dir);
		Tab* activeTab();
		Tab& newFile(const std::string& title = "Untitled");
		[[nodiscard]] int activeIndex()	const { return m_Active; }
		[[nodiscard]] int tabCount()	const { return static_cast<int>(m_Tabs.size()); }

		void update(double dt, double now);
		void render(float x, float y, float w, float h);

		void SetProjectIndex(ProjectIndex* idx) { m_ProjectIndex = idx; }

		[[nodiscard]] EditorTheme& theme() { return m_Theme; }
		[[nodiscard]] const EditorTheme& theme() const { return m_Theme; }

		void SetClipboardFunctions(std::function<std::string()> getter, std::function<void(const std::string&)> setter)
		{
			m_GetClipboard = std::move(getter);
			m_SetClipboard = std::move(setter);
		}

		void SetSaveAsDialog(std::function<std::string()> fn)
		{
			m_SaveAsDialog = std::move(fn);
		}

		void SetCtrlStateGetter(std::function<bool()> fn)
		{
			m_IsCtrlDown = std::move(fn);
		}

		void SaveActiveTab(bool saveAs = false)
		{
			if (Tab* t = activeTab())
				saveTab(*t, saveAs);
		}

		void SaveAllTabs()
		{
			for (auto& t : m_Tabs)
				if (t->dirty)
					saveTab(*t, false);
		}
	private:
		void doCopy(Tab& t);
		void doCut(Tab& t);
		void doPaste(Tab& t);

		std::function<std::string()>			m_GetClipboard;
		std::function<void(const std::string&)>	m_SetClipboard;

		Renderer*	m_Renderer = nullptr;
		EditorTheme	m_Theme;

		std::vector<std::unique_ptr<Tab>> m_Tabs;
		int m_Active = -1;

		float m_vpX = 0, m_vpY = 0, m_vpW = 0, m_vpH = 0;

		float m_TabBarH = 32.0f;
		float m_LineHeight = 20.0f;
		float m_GutterWidth = 60.0f;
		float m_ScrollBarSize = 14.0f;
		float m_CharScale = 1.0f;
		double m_CurrentTime = 0.0;

		float m_MouseX = -1.0f;
		float m_MouseY = -1.0f;

		bool m_CursorVisible = true;
		bool m_Dragging = false;

		struct CompletionState
		{
			bool active = false;
			std::vector<CompletionItem> items;
			int		selected = 0;
			float	popupX = 0;
			float	popupY = 0;
			int		replaceStart = 0;
			int		replaceEnd = 0;
		};

		CompletionEngine m_Completion;
		CompletionState  m_Comp;

		NavigationHistory m_Nav;
		void goToDefinition();
		void jumpTo(Tab* target, int line, int col);
		void navBack();
		void navForward();
		NavLocation captureNavLocation();
		void jumpToMatchingBrace(Tab& t, bool forward);

		struct RefHighlight
		{
			bool active = false;
			std::vector<std::pair<int, std::pair<int, int>>> occurrences;
		};
		RefHighlight	m_RefHighlight;
		Position		m_LastCursorPos;

		std::function<std::string()> m_SaveAsDialog;

		std::function<bool()> m_IsCtrlDown;

		ProjectIndex* m_ProjectIndex = nullptr;
		Tab* FindTabByPath(const std::string& path);

		bool saveTab(Tab& t, bool saveAs);

		void updateReferenceHighlights(Tab& t);
		void drawReferenceHighlights(Tab& t, float textX, float textY, float textW, float textH);

		void triggerCompletion(bool force);
		void cancelCompletion();
		void moveCompletion(int dir);
		void acceptCompletion();
		void drawCompletionPopup(Tab& t, float textX, float textY, float textH);

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

		[[nodiscard]] Position	pixelToPosition(Tab& t, float x, float y) const;
		[[nodiscard]] float		colToX(const Tab& t, int line, int col) const;
		[[nodiscard]] int		xToCol(const Tab& t, int line, float x) const;

		void drawTabBar();
		void drawGutter(Tab& t, float textTop, float textH);
		void drawTextLines(Tab& t, float textX, float textY, float textW, float textH);
		void drawSelection(Tab& t, float textX, float textY, float textW, float textH);
		void drawCurrentLine(Tab& t, float textX, float textY, float textW);
		void drawCursor(Tab& t, float textX, float textY, float textH);
		void drawScrollBars(Tab& t);
	};

}
