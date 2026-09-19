#include <fxpch.h>
#include "PropertiesWindow.h"
#include "Frostnux/Log.h"
#include "TextRenderer.h"
#include <glad/glad.h>
#include "Frostnux/Application.h"

namespace Frostnux {

	PropertiesWindow::PropertiesWindow(const std::string& name)
		: uiWindow(name)
	{
		m_Properties =
		{
			{"Selection", "None"},
			{"Type", "—"},
			{"Size", "—"},
			{"Location", "—"},
			{"Modified", "—"}
		};
	}

	PropertiesWindow::~PropertiesWindow()
	{
	}

	void PropertiesWindow::SetFileProperties(const std::string& filepath)
	{
		namespace fs = std::filesystem;
		if (!fs::exists(filepath))
		{
			ClearProperties();
			return;
		}

		fs::path p(filepath);
		fs::file_status status = fs::status(p);
		std::string type = fs::is_directory(p) ? "Folder" : "File";
		std::string size = fs::is_regular_file(p) ? std::to_string(fs::file_size(p)) + " bytes" : "—";
		auto ftime = fs::last_write_time(p);

		std::string modTime = "—";

		m_Properties =
		{
			{"Name", p.filename().string()},
			{"Type", type},
			{"Size", size},
			{"Location", p.parent_path().string()},
			{"Modified", modTime},
			{"Read-only", (status.permissions() & fs::perms::owner_write) == fs::perms::none ? "Yes" : "No"}
		};
		m_ScrollY = 0.0f;
	}

	void PropertiesWindow::ClearProperties()
	{
		m_Properties =
		{
			{"Selection", "None"},
			{"Type", "—"},
			{"Size", "—"},
			{"Location", "—"},
			{"Modified", "—"}
		};
	}

	void PropertiesWindow::OnUpdate(float deltaTime)
	{
		DrawContent();
		DrawProperties();
	}

	bool PropertiesWindow::OnEvent(Event& event)
	{
		if (event.GetEventType() == EventType::MouseScrolled)
		{
			MouseScrolledEvent& e = (MouseScrolledEvent&)event;
			float totalHeight = m_Properties.size() * m_ItemHeight;
			float maxScroll = std::max(0.0f, totalHeight - (m_RectHeight - 30));
			m_ScrollY -= e.GetYOffset() * 20.0f;
			m_ScrollY = std::max(0.0f, std::min(m_ScrollY, maxScroll));
			return true;
		}
		return uiWindow::OnEvent(event);
	}

	void PropertiesWindow::DrawProperties()
	{
		float startX = m_RectX + 10;
		float startY = m_RectY + 30 - m_ScrollY;
		float width = m_RectWidth - 20;

		Application& app = Application::Get();
		int winHeight = app.GetWindow().GetHeight();
		int scissorX = (int)m_RectX;
		int scissorY = winHeight - (int)(m_RectY + m_RectHeight);
		int scissorW = (int)m_RectWidth;
		int scissorH = (int)(m_RectHeight - 30);
		glEnable(GL_SCISSOR_TEST);
		glScissor(scissorX, scissorY, scissorW, scissorH);

		for (size_t i = 0; i < m_Properties.size(); ++i)
		{
			float y = startY + i * m_ItemHeight;
			if (y + m_ItemHeight < m_RectY + 30 || y > m_RectY + m_RectHeight)
			{
				continue;
			}
			DrawPropertyItem(m_Properties[i], startX, y, width);
		}

		glDisable(GL_SCISSOR_TEST);
	}

	void PropertiesWindow::DrawPropertyItem(const PropertyItem& item, float x, float y, float width)
	{
		float keyWidth = width * 0.35f;
		float valueX = x + keyWidth + 10;

		TextRenderer::Get().DrawText(item.key, x, y + 4, 0.7f, 0.7f, 0.8f, 1.0f);

		TextRenderer::Get().DrawText(item.value, valueX, y + 4, 0.9f, 0.9f, 0.9f, 1.0f);

		glColor4f(0.3f, 0.3f, 0.35f, 1.0f);
		glLineWidth(1.0f);
		glBegin(GL_LINES);
		glVertex2f(x, y + m_ItemHeight - 1);
		glVertex2f(x + width, y + m_ItemHeight - 1);
		glEnd();
	}

}