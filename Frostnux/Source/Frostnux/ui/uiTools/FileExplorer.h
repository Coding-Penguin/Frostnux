#pragma once
#include "uiWindow.h"
#include "PhotoRenderer.h"
#include "PropertiesWindow.h"

namespace Frostnux {

	struct FileNode
	{
		std::string name;
		std::string path;

		std::vector<FileNode> children;

		bool isFolder = false;
		bool expanded = false;
	};

	enum class Filetype
	{
		CPP,
		C,
		Header,
		Python,
		Java,
		CSharp,
		Markdown,
		JSON,
		Lua,
		Picture,
		Unknown
	};

	class FileExplorer : public uiWindow
	{
	public:
		FileExplorer(const std::string& name, const std::string& rootPath, PropertiesWindow* properties);
		virtual ~FileExplorer();

		virtual void OnUpdate(float deltaTime) override;
		virtual bool OnEvent(Event& event) override;
		virtual void DrawContent() override;

		void SetFileOpenCallback(std::function<void(const std::string& path)> callback);

		Filetype GetFileExtension(const std::string& path) const;

		static float GetScrollY() { return m_ScrollY; }
		static std::unordered_map<std::string, bool> GetExpandedState();

		static void SetScrollY(float scrollY) { m_ScrollY = scrollY; }
		static void SetExpandedState(const std::unordered_map<std::string, bool>& expandedState);
	private:
		std::string m_RootPath;
		static FileNode m_RootNode;
		bool m_NeedsRefresh = true;

		int m_HoveredLine = -1;
		int m_SelectedLine = -1;

		float m_LineHeight = 0.0f;

		static float m_ScrollY;
		float m_TotalHeight = 0.0f;

		bool m_IsDraggingScrollbar = false;
		float m_DragStartY = 0.0f;
		float m_DragStartScrollY = 0.0f;

		PropertiesWindow* m_Properties = nullptr;

		const FileNode* m_HoveredNode = nullptr;
		const FileNode* m_SelectNode = nullptr;

		float m_LastClickTime = 0.0f;
		const FileNode* m_LastClickedNode = nullptr;

		float m_ContentX = 0.0f, m_ContentY = 0.0f, m_ContentW = 0.0f, m_ContentH = 0.0f;
		float m_YOffset = 0.0f;

		std::unique_ptr<PhotoRenderer> m_Folder_Close_Icon;
		std::unique_ptr<PhotoRenderer> m_Folder_Open_Icon;
		std::unique_ptr<PhotoRenderer> m_File_Icon;
		std::unique_ptr<PhotoRenderer> m_Picture_Icon;
		std::unique_ptr<PhotoRenderer> m_CPP_File_Icon;
		std::unique_ptr<PhotoRenderer> m_Header_File_Icon;
		std::unique_ptr<PhotoRenderer> m_Python_File_Icon;
		std::unique_ptr<PhotoRenderer> m_CS_File_Icon;

		void RefreshTree();
		static void PopulateNode(FileNode& node, const std::filesystem::path& path);
		void DrawNode(const FileNode& node, int depth, float& y, float x, float width);
		void CalcTreeHeight(const FileNode& node, int depth, float& total);
		void DrawFolderIcon(float x, float y, bool expanded) const;
		void DrawFileIcon(float x, float y, Filetype type) const;
		int GetNodeIndexAtPosition(float mouseX, float mouseY) const;
		bool HitTestNode(const FileNode& node, int depth, float& y, float x, float width, float mouseX, float mouseY, int& hitIndex) const;
		std::string GetNodePath(const FileNode* node) const;

		std::function<void(const std::string&)> m_FileOpenCallback;

		struct VisibleNode
		{
			const FileNode* node;
			int depth;
			float y;
		};
		mutable std::vector<VisibleNode> m_VisibleNodes;
		void BuildVisibleList(const FileNode& node, int depth, float startY, float& yOffset) const;

		void Rename(const FileNode* node);
	};

}
