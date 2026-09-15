#include <fxpch.h>
#include "uiShortcutBar.h"
#include "Frostnux/Application.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "Frostnux/Log.h"
#include "Frostnux/Events/Event.h"
#include "MouseCircle.h"

namespace Frostnux {

	uiShortcutBar::uiShortcutBar() {}

	uiShortcutBar::~uiShortcutBar() {}

	void uiShortcutBar::OnAttach()
	{
	}

	void uiShortcutBar::OnDetach()
	{
		m_Buttons.clear();
		m_Groups.clear();
	}

	void uiShortcutBar::OnUpdate(float deltaTime)
	{
		for (auto& btn : m_Buttons)
		{
			btn->OnTooltipUpdate(deltaTime);
		}
	}

	bool uiShortcutBar::OnEvent(Event& event)
	{
		for (auto& btn : m_Buttons)
		{
			if (btn->OnEvent(event, 0, 0, true))
				return true;
		}
		return false;
	}

	void uiShortcutBar::AddGroup(const std::vector<ShortcutItem>& items, bool separatorAfter)
	{
		ShortcutGroup group;
		group.items = items;
		group.hasSeparatorAfter = separatorAfter;
		m_Groups.push_back(group);
		RebuildButtons();
	}

	void uiShortcutBar::Clear()
	{
		m_Groups.clear();
		m_Buttons.clear();
	}

	void uiShortcutBar::RebuildButtons()
	{
		m_Buttons.clear();
		m_GroupEndX.clear();
		for (auto& group : m_Groups)
		{
			group.buttons.clear();
		}

		if (ChannelManager::GetChannel() == Channel::Preview)
		{
			m_OffsetY = 80.0f;
		}
		else
		{
			m_OffsetY = 50.0f;
		}

		float x = 10.0f;
		float y = m_OffsetY + (m_BarHeight - m_ButtonHeight) / 2.0f;
		float width = m_ButtonWidth;
		float height = m_ButtonHeight;
		float padding = m_Padding;
		float separatorMargin = m_SeparatorMargin;

		for (size_t g = 0; g < m_Groups.size(); ++g)
		{
			auto& group = m_Groups[g];
			for (auto& item : group.items)
			{
				auto btn = std::make_unique<uiButton>(item.text, x, y, width, height, ButtonStyles::NoBackgroundOrLine);
				btn->SetCallback([callback = item.callback]() { if (callback) callback(); });
				btn->SetTooltip(item.tooltip);
				group.buttons.push_back(btn.get());
				m_Buttons.push_back(std::move(btn));
				x += width + padding;
			}
			m_GroupEndX.push_back(x);
			if (group.hasSeparatorAfter && g != m_Groups.size() - 1)
			{
				x += separatorMargin;
			}
		}
	}

	void uiShortcutBar::Draw()
	{
		Application& app = Application::Get();
		int width = app.GetWindow().GetWidth();
		int height = app.GetWindow().GetHeight();
		if (width == 0 || height == 0) return;

		float yStart = m_OffsetY + 10;
		float yEnd = m_OffsetY + m_BarHeight - 5;
		glColor4f(0.4f, 0.4f, 0.45f, 1.0f);
		glLineWidth(1.0f);
		glBegin(GL_LINES);
		for (size_t i = 0; i < m_GroupEndX.size() - 1; ++i)
		{
			if (i < m_Groups.size() && m_Groups[i].hasSeparatorAfter)
			{
				float lineX = m_GroupEndX[i] + m_SeparatorMargin / 2;
				glVertex2f(lineX, yStart);
				glVertex2f(lineX, yEnd);
			}
		}
		glEnd();

		for (auto& btn : m_Buttons)
		{
			btn->OnUpdate(0, 0, true);
			btn->DrawTooltip(0, 0);
		}
	}

}
