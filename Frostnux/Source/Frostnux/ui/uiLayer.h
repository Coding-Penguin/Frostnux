#pragma once
#include "Frostnux/Layer.h"
#include "uiTools/uiStatusBar.h"
#include "uiTools/uiShortcutBar.h"
#include "uiTools/PropertiesWindow.h"
#include "CodeEditor/CodeEditor.h"
#include "CodeEditor/GLRenderer.h"
#include <vector>

namespace Frostnux {

	class uiWindow;
	class uiTitleBar;
	class uiButton;
	class MouseCircle;

	class uiLayer : public Layer
	{
	public:
		static uiLayer& Get();

		uiLayer();
		virtual ~uiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(float deltaTime) override;
		virtual bool OnEvent(Event& event) override;

		void AddWindow(uiWindow* window);

		static bool IsPointOverAnyWindow(float x, float y);
	private:
		std::vector<uiWindow*> m_Windows;
		uiTitleBar* m_TitleBar = nullptr;
		uiStatusBar* m_StatusBar = nullptr;
		uiShortcutBar* m_ShortcutBar = nullptr;

		GLRenderer m_EditorRenderer;
		CodeEditor m_Editor { &m_EditorRenderer };
	};

}
