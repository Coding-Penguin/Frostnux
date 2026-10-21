#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace Frostnux {

	class PopupMenu
	{
	public:
		struct Item
		{
			std::string				label;
			std::string				shortcut;
			std::function<void()>	onClick;
			bool					separator = false;
			bool					enabled = true;
			bool					isHeader = false;
			std::vector<Item>		submenu;
		};

		static PopupMenu& Get();

		void Open(float x, float y, std::vector<Item> items);

		void Close();
		void CloseAll();
		[[nodiscard]] bool IsOpen() const { return m_Open; }

		void Draw();

		bool HandleEvent(void* event);

		void SetParent(PopupMenu* p) { m_Parent = p; }
	private:
		PopupMenu() = default;

		bool ContainsPoint(float mx, float my) const;
		void CloseSubmenu();

		bool				m_Open = false;
		float				m_X = 0, m_Y = 0;
		float				m_W = 0, m_H = 0;
		std::vector<Item>	m_Items;
		int					m_Hovered = -1;
		int					m_Pressed = -1;

		std::unique_ptr<PopupMenu>	m_Submenu;
		int							m_SubmenuOwner = -1;
		PopupMenu* m_Parent = nullptr;
	};

}
