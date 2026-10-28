#pragma once
#include "uiWindow.h"
#include "PhotoRenderer.h"
#include "PropertiesWindow.h"
#include "Frostnux/ProjectSystem/Solution.h"

namespace Frostnux {

	class SolutionExplorer : public uiWindow
	{
	public:
		SolutionExplorer(const std::string& name, Solution* solution, PropertiesWindow* properties);
		virtual ~SolutionExplorer();

		virtual void OnUpdate(float deltaTime) override;
		virtual bool OnEvent(Event& event) override;
		virtual void DrawContent() override;

		void SetSolution(Solution* s);
		void Refresh();

		void SetFileOpenCallback(std::function<void(const std::string&)> callback)
		{
			m_FileOpenCallback = std::move(callback);
		}

		static float GetScrollY() { return m_ScrollY; }
		static void SetScrollY(float y) { m_ScrollY = y; }

		virtual void RefreshText() override;
	private:
		enum class NodeKind { SolutionRoot, Project, Folder, File };

		struct Node
		{
			NodeKind		kind = NodeKind::File;
			std::string		name;
			std::string		fullPath;
			int				projectIndex = -1;
			bool			expanded = false;
			std::vector<Node> children;
		};

		struct VisibleNode
		{
			const Node*	node;
			int			depth;
			float		y;
		};

		Solution* m_Solution = nullptr;
		PropertiesWindow* m_Properties = nullptr;
		std::function<void(const std::string&)>	m_FileOpenCallback;

		Node			m_Root;
		float			m_LineHeight = 0.0f;
		static float	m_ScrollY;
		float			m_TotalHeight = 0.0f;
		bool			m_IsDraggingScrollbar = false;
		float			m_DragStartY = 0.0f;
		float			m_DragStartScrollY = 0.0f;

		const Node* m_HoveredNode = nullptr;
		const Node* m_SelectNode = nullptr;
		float		m_LastClickTime = 0.0f;
		const Node* m_LastClickedNode = nullptr;

		mutable std::vector<VisibleNode> m_VisibleNodes;

		std::unique_ptr<PhotoRenderer> m_Folder_Close_Icon;
		std::unique_ptr<PhotoRenderer> m_Folder_Open_Icon;
		std::unique_ptr<PhotoRenderer> m_CPP_File_Icon;
		std::unique_ptr<PhotoRenderer> m_Header_File_Icon;
		std::unique_ptr<PhotoRenderer> m_File_Icon;

		void BuildTree();
		void InsertFileRecursive(Node& parent, const std::filesystem::path& relPath, const std::filesystem::path& absPath);

		void DrawNode(const Node& node, int depth, float& y, float x, float width);
		void CalcTreeHeight(const Node& node, float& total) const;
		void BuildVisibleList(const Node& node, int depth, float startY, float& yOffset) const;

		void DrawFolderIcon(float x, float y, bool expanded) const;
		void DrawFileIcon(float x, float y, const std::string& filepath) const;

		void ToggleNode(const Node* target);
		bool IsFolderLike(const Node& n) const { return n.kind != NodeKind::File; }
	};

}
