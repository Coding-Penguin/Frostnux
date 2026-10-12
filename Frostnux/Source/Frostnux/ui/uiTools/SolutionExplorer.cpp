#include <fxpch.h>
#include "SolutionExplorer.h"
#include "TextRenderer.h"
#include "Frostnux/Core/Log.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Frostnux/Core/Application.h"
#include "../uiLayer.h"
#include "Frostnux/Events/Event.h"
#include "Frostnux/Events/MouseEvent.h"
#include "Frostnux/Events/KeyEvent.h"

namespace Frostnux {

	SolutionExplorer::SolutionExplorer(const std::string& name, Solution* solution, PropertiesWindow* properties)
		: uiWindow(name), m_Solution(solution), m_Properties(properties)
	{
		m_LineHeight = s_FontSize * 1.5f;
		SetSize(0, 110, 250, 600);

		m_Folder_Close_Icon.reset(new PhotoRenderer());
		m_Folder_Open_Icon.reset(new PhotoRenderer());
		m_CPP_File_Icon.reset(new PhotoRenderer());
		m_Header_File_Icon.reset(new PhotoRenderer());
		m_File_Icon.reset(new PhotoRenderer());

		m_Folder_Close_Icon->LoadFromFile("Resources/Images/Folder_Close.png");
		m_Folder_Open_Icon->LoadFromFile("Resources/Images/Folder_Open.png");
		m_CPP_File_Icon->LoadFromFile("Resources/Images/CPP_File.png");

		if (ThemeManager::IsDarkTheme())
		{
			m_File_Icon->LoadFromFile("Resources/Images/File_White.png");
			m_Header_File_Icon->LoadFromFile("Resources/Images/Header_File_White.png");
		}
		else
		{
			m_File_Icon->LoadFromFile("Resources/Images/File_Black.png");
			m_Header_File_Icon->LoadFromFile("Resources/Images/Header_File_Black.png");
		}

		BuildTree();
	}

	SolutionExplorer::~SolutionExplorer()
	{
		if (m_Folder_Close_Icon)	m_Folder_Close_Icon->Unload();
		if (m_Folder_Open_Icon)		m_Folder_Open_Icon->Unload();
		if (m_CPP_File_Icon)		m_CPP_File_Icon->Unload();
		if (m_Header_File_Icon)		m_Header_File_Icon->Unload();
		if (m_File_Icon)			m_File_Icon->Unload();
	}

	void SolutionExplorer::SetSolution(Solution* s)
	{
		m_Solution = s;
		BuildTree();
	}

	void SolutionExplorer::Refresh()
	{
		BuildTree();
	}

	void SolutionExplorer::BuildTree()
	{
		m_Root = Node{};
		m_Root.kind = NodeKind::SolutionRoot;

		if (!m_Solution)
		{
			m_Root.name = "(no solution)";
			return;
		}

		m_Root.name = m_Solution->name.empty() ? "Solution" : m_Solution->name;
		m_Root.expanded = true;

		for (int pi = 0; pi < static_cast<int>(m_Solution->projects.size()); ++pi)
		{
			auto* proj = m_Solution->projects[pi].get();

			Node pNode;
			pNode.kind = NodeKind::Project;
			pNode.name = proj->name;
			pNode.projectIndex = pi;
			pNode.expanded = true;

			auto files = proj->ResolveSources();
			auto base = proj->Directory();

			for (auto& f : files)
			{
				std::error_code ec;
				auto rel = std::filesystem::relative(f, base, ec);
				if (ec) rel = f.filename();
				InsertFileRecursive(pNode, rel, f);
			}

			m_Root.children.push_back(std::move(pNode));
		}
	}

	void SolutionExplorer::InsertFileRecursive(Node& parent, const std::filesystem::path& relPath, const std::filesystem::path& absPath)
	{
		auto it = relPath.begin();
		auto end = relPath.end();
		Node* cur = &parent;

		while (it != end)
		{
			auto next = it;
			++next;

			if (next == end)
			{
				Node leaf;
				leaf.kind = NodeKind::File;
				leaf.name = it->string();
				leaf.fullPath = absPath.generic_string();
				cur->children.push_back(std::move(leaf));
				return;
			}

			std::string folderName = it->string();
			Node* found = nullptr;
			for (auto& c : cur->children)
			{
				if (c.kind == NodeKind::Folder && c.name == folderName)
				{
					found = &c;
					break;
				}
			}
			if (!found)
			{
				Node f;
				f.kind = NodeKind::Folder;
				f.name = folderName;
				f.expanded = true;
				cur->children.push_back(std::move(f));
				found = &cur->children.back();
			}
			cur = found;
			it = next;
		}
	}

	void SolutionExplorer::OnUpdate(float deltaTime)
	{
		(void)deltaTime;
		if (!IsVisible()) return;
		DrawContent();
	}

	bool SolutionExplorer::OnEvent(Event& event)
	{
		if (!IsVisible()) return false;
		if (uiWindow::OnEvent(event)) return true;

		EventDispatcher dispatcher(event);

		dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& e)
			{
				float mx = e.GetMouseX(), my = e.GetMouseY();
				float contentX = GetX();
				float contentY = GetY() + 30;
				float contentW = GetWidth();
				float contentH = GetHeight() - 30;

				if (!(mx >= contentX && mx <= contentX + contentW &&
					my >= contentY && my <= contentY + contentH))
					return false;

				float maxScroll = std::max(0.0f, m_TotalHeight - contentH);
				if (maxScroll <= 0) return false;

				m_ScrollY -= e.GetYOffset() * 50.0f;
				if (m_ScrollY < 0) m_ScrollY = 0;
				if (m_ScrollY > maxScroll) m_ScrollY = maxScroll;
				return true;
			});

		dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& e)
			{
				if (e.GetMouseButton() != GLFW_MOUSE_BUTTON_LEFT) return false;

				float mx = e.GetMouseX(), my = e.GetMouseY();
				float contentX = GetX();
				float contentY = GetY() + 30;
				float contentW = GetWidth();
				float contentH = GetHeight() - 30;

				if (mx < contentX || mx > contentX + contentW || my < contentY || my > contentY + contentH)
					return false;

				float currentTime = (float)glfwGetTime();

				m_VisibleNodes.clear();
				float yOffset = 0.0f;
				BuildVisibleList(m_Root, 0, contentY, yOffset);

				const Node* clickedNode = nullptr;
				int clickedDepth = 0;
				float clickedY = 0.0f;
				for (const auto& vn : m_VisibleNodes)
				{
					if (my >= vn.y && my <= vn.y + m_LineHeight)
					{
						clickedNode = vn.node;
						clickedDepth = vn.depth;
						clickedY = vn.y;
						break;
					}
				}
				if (!clickedNode) return false;

				float indent = clickedDepth * 16.0f;
				float iconX = contentX + indent;
				float iconSize = 16.0f;
				bool isOnTriangle = (mx >= iconX && mx <= iconX + iconSize + 10 && my >= clickedY && my <= clickedY + m_LineHeight);

				if (IsFolderLike(*clickedNode))
				{
					bool doubleClick = (m_LastClickedNode == clickedNode &&
						(currentTime - m_LastClickTime) < 0.5f);

					if (isOnTriangle || doubleClick)
					{
						ToggleNode(clickedNode);
						m_LastClickedNode = nullptr;
						m_LastClickTime = 0.0f;
						return true;
					}

					m_LastClickedNode = clickedNode;
					m_LastClickTime = currentTime;
					m_SelectNode = clickedNode;
					return true;
				}
				else
				{
					bool doubleClick = (m_LastClickedNode == clickedNode && (currentTime - m_LastClickTime) < 0.5f);

					if (doubleClick)
					{
						if (m_FileOpenCallback && !clickedNode->fullPath.empty())
							m_FileOpenCallback(clickedNode->fullPath);
						m_LastClickedNode = nullptr;
						m_LastClickTime = 0.0f;
						return true;
					}

					m_LastClickedNode = clickedNode;
					m_LastClickTime = currentTime;
					m_SelectNode = clickedNode;

					if (m_Properties && !clickedNode->fullPath.empty())
						m_Properties->SetFileProperties(clickedNode->fullPath);
					return true;
				}
			});

		dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent& e)
			{
				if (m_IsDraggingScrollbar)
				{
					float contentY = GetY() + 30;
					float contentH = GetHeight() - 30;
					float maxScroll = std::max(0.0f, m_TotalHeight - contentH);
					float deltaY = e.GetY() - m_DragStartY;
					float ratio = deltaY / contentH;
					m_ScrollY = m_DragStartScrollY + ratio * maxScroll;
					if (m_ScrollY < 0) m_ScrollY = 0;
					if (m_ScrollY > maxScroll) m_ScrollY = maxScroll;
					return true;
				}

				float mx = e.GetX(), my = e.GetY();
				float contentX = GetX();
				float contentY = GetY() + 30;
				float contentW = GetWidth();
				float contentH = GetHeight() - 30;

				if (mx < contentX || mx > contentX + contentW || my < contentY || my > contentY + contentH)
				{
					m_HoveredNode = nullptr;
					return false;
				}

				m_VisibleNodes.clear();
				float yOffset = 0.0f;
				BuildVisibleList(m_Root, 0, contentY, yOffset);

				m_HoveredNode = nullptr;
				for (const auto& vn : m_VisibleNodes)
				{
					if (my >= vn.y && my <= vn.y + m_LineHeight)
					{
						m_HoveredNode = vn.node;
						break;
					}
				}
				return false;
			});

		dispatcher.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& e)
			{
				if (e.GetMouseButton() == GLFW_MOUSE_BUTTON_LEFT)
					m_IsDraggingScrollbar = false;
				return false;
			});

		return false;
	}

	void SolutionExplorer::DrawContent()
	{
		uiWindow::DrawContent();

		float contentX = GetX() + 10;
		float contentY = GetY() + 40;
		float contentW = GetWidth() - 10;
		float contentH = GetHeight() - 40;

		Application& app = Application::Get();
		int winHeight = app.GetWindow().GetHeight();

		glEnable(GL_SCISSOR_TEST);
		glScissor((int)contentX, winHeight - (int)(contentY + contentH), (int)contentW, (int)contentH);

		m_TotalHeight = 0.0f;
		CalcTreeHeight(m_Root, m_TotalHeight);

		float maxScroll = std::max(0.0f, m_TotalHeight - contentH);
		if (m_ScrollY > maxScroll) m_ScrollY = maxScroll;
		if (m_ScrollY < 0) m_ScrollY = 0;

		float y = contentY - m_ScrollY;
		DrawNode(m_Root, 0, y, contentX, contentW);

		if (m_TotalHeight > contentH)
		{
			float thumbH = contentH * (contentH / m_TotalHeight);
			float thumbY = contentY + (m_ScrollY / (m_TotalHeight - contentH)) * (contentH - thumbH);
			glColor4f(0.5f, 0.5f, 0.5f, 0.8f);
			glBegin(GL_QUADS);
			glVertex2f(contentX + contentW - 8, thumbY);
			glVertex2f(contentX + contentW - 2, thumbY);
			glVertex2f(contentX + contentW - 2, thumbY + thumbH);
			glVertex2f(contentX + contentW - 8, thumbY + thumbH);
			glEnd();
		}

		glDisable(GL_SCISSOR_TEST);
	}

	void SolutionExplorer::CalcTreeHeight(const Node& node, float& total) const
	{
		total += m_LineHeight;
		if (IsFolderLike(node) && node.expanded)
			for (const auto& c : node.children)
				CalcTreeHeight(c, total);
	}

	void SolutionExplorer::DrawNode(const Node& node, int depth, float& y, float x, float width)
	{
		float indent = depth * 16.0f;
		float iconSize = 16.0f;
		float textX = x + indent + iconSize + 4;

		if (y + m_LineHeight < GetY() + 30 || y > GetY() + GetHeight())
		{
			y += m_LineHeight;
			if (IsFolderLike(node) && node.expanded)
				for (const auto& c : node.children)
					DrawNode(c, depth + 1, y, x, width);
			return;
		}

		if (&node == m_HoveredNode)
		{
			if (ThemeManager::IsDarkTheme())
				glColor4f(0.2f, 0.2f, 0.2f, 0.5f);
			else
				glColor4f(0.8f, 0.8f, 0.8f, 0.5f);

			glBegin(GL_QUADS);
			glVertex2f(x, y);
			glVertex2f(x + width, y);
			glVertex2f(x + width, y + m_LineHeight);
			glVertex2f(x, y + m_LineHeight);
			glEnd();
		}

		if (&node == m_SelectNode)
		{
			glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
			glBegin(GL_QUADS);
			glVertex2f(x, y);
			glVertex2f(x + width, y);
			glVertex2f(x + width, y + m_LineHeight);
			glVertex2f(x, y + m_LineHeight);
			glEnd();

			glColor4f(0.7f, 0.7f, 0.7f, 1.0f);
			glBegin(GL_LINE_LOOP);
			glVertex2f(x, y);
			glVertex2f(x + width, y);
			glVertex2f(x + width, y + m_LineHeight);
			glVertex2f(x, y + m_LineHeight);
			glEnd();
		}

		float iconX = x + indent;
		float iconY = y + (m_LineHeight - iconSize) / 2;

		if (IsFolderLike(node))
			DrawFolderIcon(iconX, iconY, node.expanded);
		else
			DrawFileIcon(iconX, iconY, node.fullPath);

		if (TextRenderer::Get().IsInitialized())
		{
			if (ThemeManager::IsDarkTheme())
				TextRenderer::Get().DrawText(node.name, textX + m_LineHeight, y + 2, 0.9f, 0.9f, 0.9f, 1.0f);
			else
				TextRenderer::Get().DrawText(node.name, textX + m_LineHeight, y + 2, 0.1f, 0.1f, 0.1f, 1.0f);
		}

		y += m_LineHeight;

		if (IsFolderLike(node) && node.expanded)
			for (const auto& c : node.children)
				DrawNode(c, depth + 1, y, x, width);
	}

	void SolutionExplorer::DrawFolderIcon(float x, float y, bool expanded) const
	{
		if (ThemeManager::IsDarkTheme())
			glColor4f(0.8f, 0.8f, 0.8f, 1.0f);
		else
			glColor4f(0.2f, 0.2f, 0.2f, 1.0f);

		if (expanded)
		{
			glBegin(GL_TRIANGLES);
			glVertex2f(x, y);
			glVertex2f(x + 10, y);
			glVertex2f(x + 5, y + 10);
			glEnd();
			m_Folder_Open_Icon->Draw(x + 20, y - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
		}
		else
		{
			glBegin(GL_LINE_LOOP);
			glVertex2f(x, y);
			glVertex2f(x, y + 10);
			glVertex2f(x + 7, y + 5);
			glEnd();
			m_Folder_Close_Icon->Draw(x + 20, y - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
		}
	}

	void SolutionExplorer::DrawFileIcon(float x, float y, const std::string& filepath) const
	{
		auto dot = filepath.find_last_of('.');
		std::string ext = (dot == std::string::npos) ? "" : filepath.substr(dot + 1);

		if (ext == "cpp" || ext == "c" || ext == "cc" || ext == "cxx")
			m_CPP_File_Icon->Draw(x + 20, y - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
		else if (ext == "h" || ext == "hpp" || ext == "hxx")
			m_Header_File_Icon->Draw(x + 20, y - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
		else
			m_File_Icon->Draw(x + 20, y - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
	}

	void SolutionExplorer::BuildVisibleList(const Node& node, int depth, float startY, float& yOffset) const
	{
		float screenY = startY + yOffset - m_ScrollY;
		m_VisibleNodes.push_back({ &node, depth, screenY });
		yOffset += m_LineHeight;

		if (IsFolderLike(node) && node.expanded)
			for (const auto& c : node.children)
				BuildVisibleList(c, depth + 1, startY, yOffset);
	}

	void SolutionExplorer::ToggleNode(const Node* target)
	{
		if (!target) return;

		std::function<bool(Node&)> walk = [&](Node& n) -> bool
			{
				if (&n == target)
				{
					n.expanded = !n.expanded;
					return true;
				}
				for (auto& c : n.children)
					if (walk(c)) return true;
				return false;
			};
		walk(m_Root);
	}

}
