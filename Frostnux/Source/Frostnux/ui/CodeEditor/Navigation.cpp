#include "fxpch.h"
#include "Navigation.h"
#include "Tab.h"

namespace Frostnux {

	bool NavigationHistory::same(const NavLocation& a, const NavLocation& b)
	{
		return a.tab == b.tab && a.pos == b.pos;
	}

	void NavigationHistory::push(const NavLocation& loc)
	{
		if (!loc.tab) return;

		if (m_Index + 1 < (int)m_Stack.size())
			m_Stack.erase(m_Stack.begin() + m_Index + 1, m_Stack.end());

		if (!m_Stack.empty() && same(m_Stack.back(), loc))
			return;

		m_Stack.push_back(loc);
		m_Index = (int)m_Stack.size() - 1;

		constexpr size_t kMaxSize = 200;
		if (m_Stack.size() > kMaxSize)
		{
			m_Stack.erase(m_Stack.begin());
			--m_Index;
		}
	}

	bool NavigationHistory::canBack()    const { return m_Index > 0; }
	bool NavigationHistory::canForward() const
	{
		return m_Index >= 0 && m_Index + 1 < (int)m_Stack.size();
	}

	bool NavigationHistory::back(const NavLocation& current, NavLocation& out)
	{
		(void)current;
		if (!canBack()) return false;
		--m_Index;
		out = m_Stack[m_Index];
		return true;
	}

	bool NavigationHistory::forward(const NavLocation& current, NavLocation& out)
	{
		(void)current;
		if (!canForward()) return false;
		++m_Index;
		out = m_Stack[m_Index];
		return true;
	}

	void NavigationHistory::clear()
	{
		m_Stack.clear();
		m_Index = -1;
	}

}
