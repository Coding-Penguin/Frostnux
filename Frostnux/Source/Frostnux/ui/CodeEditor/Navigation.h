#pragma once
#include "TextBuffer.h"
#include <vector>

namespace Frostnux {

	struct Tab;

	struct NavLocation
	{
		Tab*		tab = nullptr;
		Position	pos;
		double		scrollX = 0;
		double		scrollY = 0;
	};

	class NavigationHistory
	{
	public:
		void push(const NavLocation& loc);

		[[nodiscard]] bool canBack()    const;
		[[nodiscard]] bool canForward() const;

		bool back(const NavLocation& current, NavLocation& out);
		bool forward(const NavLocation& current, NavLocation& out);

		void clear();

	private:
		std::vector<NavLocation> m_Stack;
		int m_Index = -1;

		static bool same(const NavLocation& a, const NavLocation& b);
	};

}
