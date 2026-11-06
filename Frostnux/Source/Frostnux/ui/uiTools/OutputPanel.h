#pragma once
#include "uiWindow.h"
#include <vector>
#include <mutex>

namespace Frostnux {

	class OutputPanel : public uiWindow
	{
	public:
		OutputPanel(const std::string& name);
		virtual ~OutputPanel();

		virtual void OnUpdate(float deltaTime) override;
		virtual void DrawContent() override;

		void AppendLine(const std::string& text);
		void Clear();

		void SetScrollToBottom() { m_ScrollY = -1.0f; }

	private:
		std::vector<std::string> m_Lines;
		std::mutex m_Mutex;
		float m_ScrollY = -1.0f;
		float m_LineHeight = 18.0f;
		float m_TotalHeight = 0.0f;
	};

}