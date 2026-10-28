#pragma once
#include <string>
#include <functional>
#include "Frostnux/Events/Event.h"

namespace Frostnux {

	class TextInputDialog
	{
	public:
		static TextInputDialog& Get();

		void Open(const std::string& title, const std::string& defaultText, std::function<void(const std::string&)> onConfirm);

		void Close();
		[[nodiscard]] bool IsOpen() const { return m_IsOpen; }

		void Draw();
		bool OnEvent(Event& event);

	private:
		TextInputDialog() = default;

		bool m_IsOpen = false;
		std::string m_Title;
		std::string m_Text;
		int m_Caret = 0;
		std::function<void(const std::string&)> m_OnConfirm;

		float m_X = 0, m_Y = 0, m_W = 400, m_H = 150;
	};

}
