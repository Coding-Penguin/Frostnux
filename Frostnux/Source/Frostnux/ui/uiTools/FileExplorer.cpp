#include <fxpch.h>
#include "FileExplorer.h"
#include "TextRenderer.h"
#include "Frostnux/Log.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Frostnux/Application.h"
#include "../uiLayer.h"
#include "Frostnux/Events/Event.h"
#include "Frostnux/Events/MouseEvent.h"
#include "Frostnux/Events/KeyEvent.h"

namespace Frostnux {

	FileExplorer::FileExplorer(const std::string& name, const std::string& rootPath, PropertiesWindow* properties)
		: uiWindow(name), m_RootPath(rootPath), m_Properties(properties)
	{
		m_LineHeight = s_FontSize * 1.5f;
		SetSize(0, 110, 250, 600);
		RefreshTree();
		LoadState();

		// Load Icons
		m_Folder_Close_Icon.reset(new PhotoRenderer());
		m_Folder_Open_Icon.reset(new PhotoRenderer());
		m_CPP_File_Icon.reset(new PhotoRenderer());
		m_Header_File_Icon.reset(new PhotoRenderer());
		m_Python_File_Icon.reset(new PhotoRenderer());
		m_CS_File_Icon.reset(new PhotoRenderer());
		m_File_Icon.reset(new PhotoRenderer());
		m_Picture_Icon.reset(new PhotoRenderer());

		m_Folder_Close_Icon->LoadFromFile("Resources/Images/Folder_Close.png");
		m_Folder_Open_Icon->LoadFromFile("Resources/Images/Folder_Open.png");
		m_CPP_File_Icon->LoadFromFile("Resources/Images/CPP_File.png");
		m_Python_File_Icon->LoadFromFile("Resources/Images/Python_File.png");
		m_CS_File_Icon->LoadFromFile("Resources/Images/CS_File.png");
		if (ThemeManager::IsDarkTheme())
		{
			m_File_Icon->LoadFromFile("Resources/Images/File_White.png");
			m_Header_File_Icon->LoadFromFile("Resources/Images/Header_File_White.png");
			m_Picture_Icon->LoadFromFile("Resources/Images/Picture_White.png");
		}
		else
		{
			m_File_Icon->LoadFromFile("Resources/Images/File_Black.png");
			m_Header_File_Icon->LoadFromFile("Resources/Images/Header_File_Black.png");
			m_Picture_Icon->LoadFromFile("Resources/Images/Picture_Black.png");
		}
	}

	FileExplorer::~FileExplorer()
	{
		m_Folder_Close_Icon->Unload();
		m_Folder_Open_Icon->Unload();
		m_File_Icon->Unload();
		m_CPP_File_Icon->Unload();
		m_Header_File_Icon->Unload();
		m_Python_File_Icon->Unload();
	}

	void FileExplorer::SetFileOpenCallback(std::function<void(const std::string&)> callback)
	{
		m_FileOpenCallback = callback;
	}

	void FileExplorer::RefreshTree()
	{
		m_RootNode.name = std::filesystem::path(m_RootPath).filename().string();
		m_RootNode.path = m_RootPath;
		m_RootNode.isFolder = true;
		m_RootNode.expanded = true;
		m_RootNode.children.clear();
		PopulateNode(m_RootNode, m_RootPath);
		m_NeedsRefresh = false;
	}

	void FileExplorer::PopulateNode(FileNode& node, const std::filesystem::path& path)
	{
		try
		{
			for (const auto& entry : std::filesystem::directory_iterator(path))
			{
				FileNode child;
				child.name = entry.path().filename().string();
				child.path = entry.path().generic_string();
				child.isFolder = entry.is_directory();
				child.expanded = false;
				node.children.push_back(child);
			}
			std::sort(node.children.begin(), node.children.end(),
				[](const FileNode& a, const FileNode& b)
				{
					if (a.isFolder != b.isFolder)
					{
						return a.isFolder > b.isFolder;
					}
					return a.name < b.name;
				});
		}
		catch (const std::exception& e)
		{
			FX_CORE_WARN("Failed to read directory: {}", path.string());
		}
	}

	void FileExplorer::OnUpdate(float deltaTime)
	{
		if (!IsVisible()) return;
		DrawContent();
	}

	bool FileExplorer::OnEvent(Event& event)
	{
		if (!IsVisible()) return false;

		if (uiWindow::OnEvent(event)) return true;
		EventDispatcher dispatcher(event);

		dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& e)
			{
				float mx = e.GetMouseX();
				float my = e.GetMouseY();

				float contentX = GetX();
				float contentY = GetY() + 30;
				float contentW = GetWidth();
				float contentH = GetHeight() - 30;

				if (!(mx >= contentX && mx <= contentX + contentW && my >= contentY && my <= contentY + contentH))
					return false;

				float maxScroll = std::max(0.0f, m_TotalHeight - contentH);
				if (maxScroll <= 0) return false;

				m_ScrollY -= e.GetYOffset() * 50.0f;
				if (m_ScrollY < 0) m_ScrollY = 0;
				if (m_ScrollY > maxScroll) m_ScrollY = maxScroll;

				SaveState();

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
				BuildVisibleList(m_RootNode, 0, contentY, yOffset);

				const FileNode* clickedNode = nullptr;
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

				if (clickedNode->isFolder)
				{
					if (isOnTriangle)
					{
						std::function<bool(FileNode&)> findAndToggle = [&](FileNode& n) -> bool
							{
								if (&n == clickedNode)
								{
									n.expanded = !n.expanded;
									if (n.expanded && n.children.empty())
									{
										PopulateNode(n, n.path);
									}
									return true;
								}
								for (auto& child : n.children)
								{
									if (findAndToggle(child)) return true;
								}
								return false;
							};
						findAndToggle(m_RootNode);
						SaveState();
						return true;
					}
					else
					{
						if (m_LastClickedNode == clickedNode && (currentTime - m_LastClickTime) < 0.5f)
						{
							std::function<bool(FileNode&)> findAndToggle = [&](FileNode& n) -> bool
								{
									if (&n == clickedNode)
									{
										n.expanded = !n.expanded;
										if (n.expanded && n.children.empty())
										{
											PopulateNode(n, n.path);
										}
										return true;
									}
									for (auto& child : n.children)
									{
										if (findAndToggle(child)) return true;
									}
									return false;
								};
							findAndToggle(m_RootNode);
							m_LastClickedNode = nullptr;
							m_LastClickTime = 0.0f;
							SaveState();
							return true;
						}
						else
						{
							m_LastClickedNode = clickedNode;
							m_LastClickTime = currentTime;

							m_VisibleNodes.clear();
							float yOffset = 0.0f;
							BuildVisibleList(m_RootNode, 0, contentY, yOffset);
							m_SelectNode = nullptr;
							
							for (const auto& vn : m_VisibleNodes)
							{
								float yPos = vn.y;
								if (my >= yPos && my <= yPos + m_LineHeight)
								{
									m_SelectNode = vn.node;
									break;
								}
							}
							
							if (m_Properties)
								m_Properties->SetFileProperties(m_SelectNode->path);

							SaveState();
							return true;
						}
					}
				}
				else
				{
					if (m_LastClickedNode == clickedNode && (currentTime - m_LastClickTime) < 0.5f)
					{
						if (m_FileOpenCallback)
						{
							m_FileOpenCallback(clickedNode->path);
						}

						m_LastClickedNode = nullptr;
						m_LastClickTime = 0.0f;
						SaveState();
						return true;
					}
					else
					{
						m_LastClickedNode = clickedNode;
						m_LastClickTime = currentTime;

						m_VisibleNodes.clear();
						float yOffset = 0.0f;
						BuildVisibleList(m_RootNode, 0, contentY, yOffset);
						m_SelectNode = nullptr;
						
						for (const auto& vn : m_VisibleNodes)
						{
							float yPos = vn.y;
							if (my >= yPos && my <= yPos + m_LineHeight)
							{
								m_SelectNode = vn.node;
								break;
							}
						}
						
						if (m_Properties)
							m_Properties->SetFileProperties(m_SelectNode->path);
						SaveState();
						
						return true;
					}
				}

				return false;
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
				else
				{
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
					BuildVisibleList(m_RootNode, 0, contentY, yOffset);

					m_HoveredNode = nullptr;
					for (const auto& vn : m_VisibleNodes)
					{
						float yPos = vn.y;
						if (my >= yPos && my <= yPos + m_LineHeight)
						{
							m_HoveredNode = vn.node;
							break;
						}
					}
				}
				return false;
			});

		dispatcher.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& e)
			{
				if (e.GetMouseButton() == GLFW_MOUSE_BUTTON_LEFT)
				{
					m_IsDraggingScrollbar = false;
				}
				return false;
			});

		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e)
			{
				if (e.GetKeyCode() == FX_KEY_F2)
				{
					if (!m_SelectNode) return false;
					Rename(m_SelectNode);
				}

				if (e.GetKeyCode() == FX_KEY_ENTER)
				{
					if (!m_SelectNode) return false;
					if (m_SelectNode->isFolder)
					{
						std::function<bool(FileNode&)> findAndToggle = [&](FileNode& n) -> bool
							{
								if (&n == m_SelectNode)
								{
									n.expanded = !n.expanded;
									if (n.expanded && n.children.empty())
									{
										PopulateNode(n, n.path);
									}
									return true;
								}
								for (auto& child : n.children)
								{
									if (findAndToggle(child)) return true;
								}
								return false;
							};
						findAndToggle(m_RootNode);
					}
					else
					{
						if (m_FileOpenCallback)
						{
							m_FileOpenCallback(m_SelectNode->path);
						}
					}
				}
			}
		);

		return false;
	}

	void FileExplorer::DrawContent()
	{
		uiWindow::DrawContent();

		float contentX = GetX() + 10;
		float contentY = GetY() + 40 + m_YOffset;
		float contentW = GetWidth() - 10;
		float contentH = GetHeight() - 40 - m_YOffset;

		Application& app = Application::Get();
		int winHeight = app.GetWindow().GetHeight();
		glEnable(GL_SCISSOR_TEST);
		glScissor((int)contentX, winHeight - (int)(contentY + contentH), (int)contentW, (int)contentH);

		m_TotalHeight = 0.0f;
		CalcTreeHeight(m_RootNode, 0, m_TotalHeight);

		float maxScroll = std::max(0.0f, m_TotalHeight - contentH);
		if (m_ScrollY > maxScroll) m_ScrollY = maxScroll;
		if (m_ScrollY < 0) m_ScrollY = 0;

		float y = contentY - m_ScrollY;
		DrawNode(m_RootNode, 0, y, contentX, contentW);

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

	void FileExplorer::CalcTreeHeight(const FileNode& node, int depth, float& total)
	{
		total += m_LineHeight;
		if (node.isFolder && node.expanded)
		{
			for (const auto& child : node.children)
			{
				CalcTreeHeight(child, depth + 1, total);
			}
		}
	}

	void FileExplorer::DrawNode(const FileNode& node, int depth, float& y, float x, float width)
	{
		float indent = depth * 16.0f;
		float iconSize = 16.0f;
		float textX = x + indent + iconSize + 4;

		if (y + m_LineHeight < GetY() + 30 || y > GetY() + GetHeight())
		{
			y += m_LineHeight;
			if (node.isFolder && node.expanded)
			{
				for (const auto& child : node.children)
				{
					DrawNode(child, depth + 1, y, x, width);
				}
			}
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
		if (node.isFolder)
		{
			DrawFolderIcon(iconX, iconY, node.expanded);
		}
		else
		{
			DrawFileIcon(iconX, iconY, GetFileExtension(node.path));
		}

		if (TextRenderer::Get().IsInitialized())
		{
			if (ThemeManager::IsDarkTheme())
				TextRenderer::Get().DrawText(node.name, textX + m_LineHeight, y + 2, 0.9f, 0.9f, 0.9f, 1.0f);
			else
				TextRenderer::Get().DrawText(node.name, textX + m_LineHeight, y + 2, 0.1f, 0.1f, 0.1f, 1.0f);
		}
		else
		{
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glBegin(GL_QUADS);
			glVertex2f(textX, y);
			glVertex2f(textX + 50, y);
			glVertex2f(textX + 50, y + m_LineHeight);
			glVertex2f(textX, y + m_LineHeight);
			glEnd();
		}

		y += m_LineHeight;

		if (node.isFolder && node.expanded)
		{
			for (const auto& child : node.children)
			{
				DrawNode(child, depth + 1, y, x, width);
			}
		}
	}

	void FileExplorer::DrawFolderIcon(float x, float y, bool expanded) const
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

	void FileExplorer::DrawFileIcon(float x, float y, Filetype type) const
	{
		if (type == Filetype::CPP)
		{
			m_CPP_File_Icon->Draw(x + 20, y - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
		}
		else if (type == Filetype::Header)
		{
			m_Header_File_Icon->Draw(x + 20, y - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
		}
		else if (type == Filetype::Python)
		{
			m_Python_File_Icon->Draw(x + 20, y - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
		}
		else if (type == Filetype::CSharp)
		{
			m_CS_File_Icon->Draw(x + 20, y - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
		}
		else if (type == Filetype::Picture)
		{
			m_Picture_Icon->Draw(x + 20, y - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
		}
		else
		{
			m_File_Icon->Draw(x + 20, y - 5, m_LineHeight * 0.7f, m_LineHeight * 0.7f);
		}
	}

	void FileExplorer::BuildVisibleList(const FileNode& node, int depth, float startY, float& yOffset) const
	{
		float screenY = startY + yOffset - m_ScrollY;
		m_VisibleNodes.push_back({ &node, depth, screenY });
		yOffset += m_LineHeight;
		if (node.isFolder && node.expanded)
		{
			for (const auto& child : node.children)
			{
				BuildVisibleList(child, depth + 1, startY, yOffset);
			}
		}
	}

	Filetype FileExplorer::GetFileExtension(const std::string& path) const
	{
		size_t pos = path.find_last_of('.');
		if (pos != std::string::npos)
		{
			std::string ext = path.substr(pos + 1);
			if (ext == "cpp") return Filetype::CPP;
			else if (ext == "c") return Filetype::C;
			else if (ext == "h") return Filetype::Header;
			else if (ext == "py") return Filetype::Python;
			else if (ext == "java") return Filetype::Java;
			else if (ext == "cs") return Filetype::CSharp;
			else if (ext == "md") return Filetype::Markdown;
			else if (ext == "json") return Filetype::JSON;
			else if (ext == "lua") return Filetype::Lua;
			else if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "jfif" || ext == "pjpeg" || ext == "pjp" || ext == "gif" || ext == "bmp" || ext == "dib" || ext == "webp" || ext == "avif" || ext == "tif" || ext == "tiff" || ext == "svg" || ext == "eps" || ext == "ai" || ext == "cdr" || ext == "wmf" || ext == "emf" || ext == "raw" || ext == "psd" || ext == "ico" || ext == "icns" || ext == "heic" || ext == "tga" || ext == "pcx" || ext == "apng" || ext == "xpm") return Filetype::Picture;
		}
		return Filetype::Unknown;
	}

	void FileExplorer::Rename(const FileNode* node)
	{
		if (!node) return;
	}

	void FileExplorer::LoadState()
	{
		std::unordered_map<std::string, bool> expandedState;
		float scrollY = 0.0f;
		SettingsManager::Get().LoadFileExplorerState(scrollY, expandedState);
		m_ScrollY = scrollY;

		std::function<void(FileNode&)> setState = [&](FileNode& n)
			{
			auto it = expandedState.find(n.path);
			if (it != expandedState.end())
			{
				n.expanded = it->second;
				if (n.expanded && n.children.empty() && n.isFolder)
				{
					PopulateNode(n, n.path);
				}
			}
			for (auto& child : n.children)
			{
				setState(child);
			}
			};
		setState(m_RootNode);
	}

	void FileExplorer::SaveState()
	{
		std::unordered_map<std::string, bool> expandedState;
		std::function<void(const FileNode&)> collectExpanded = [&](const FileNode& n)
			{
			if (n.expanded)
			{
				expandedState[n.path] = true;
			}
			for (const auto& child : n.children)
			{
				collectExpanded(child);
			}
			};
		collectExpanded(m_RootNode);

		SettingsManager::Get().SaveFileExplorerState(m_ScrollY, expandedState);
	}

}
