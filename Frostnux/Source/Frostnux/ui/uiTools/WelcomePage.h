#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "../CodeEditor/TextBuffer.h"
#include "../CodeEditor/CodeEditor.h"

namespace Frostnux {

	class PhotoRenderer;

	class WelcomePage
	{
	public:
		WelcomePage();
		~WelcomePage();

		void OnAttach();

		void Draw(float x, float y, float w, float h);

		bool OnEvent(Event& event);

		void SetCallbacks(
			std::function<void()> onNewFile,
			std::function<void()> onOpenFile,
			std::function<void()> onNewProject,
			std::function<void()> onOpenProject
		);

		void RefreshText();
	private:
		struct LinkButton
		{
			std::string label;
			std::string desc;
			Rect rect;
			bool hovered = false;
			std::function<void()> onClick;
		};

		std::unique_ptr<PhotoRenderer> m_Logo;
		std::vector<LinkButton> m_Buttons;
		std::string m_LanguageCode;

		void Layout(float x, float y, float w, float h);
		void DrawRect(float x, float y, float w, float h, Color color);
	};

}