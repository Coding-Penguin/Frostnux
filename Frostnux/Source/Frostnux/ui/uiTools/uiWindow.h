#pragma once
#include <fxpch.h>
#include "TextRenderer.h"
#include "Frostnux/Layer.h"
#include "uiButton.h"

namespace Frostnux {

	enum class ResizeEdge
	{
		None,
		Left, Right, Top, Bottom,
		TopLeft, TopRight, BottomLeft, BottomRight
	};

	enum class DockRegion
	{
		Left,
		Right,
		Top,
		Bottom,
		Center,
		TopLeft,
		TopRight,
		BottomLeft,
		BottomRight,
		None
	};

	class uiWindow : public Layer
	{
	public:
		uiWindow(std::string name);
		virtual ~uiWindow();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(float deltaTime) override;
		virtual bool OnEvent(Event& event) override;

		bool IsVisible() const { return m_IsVisible; }

		void SetSize(float x, float y, float width, float height);
		void SetStyle(bool isDark);

		void AddButton(uiButton* button);

		void DrawContent();
		std::string GetTitle() const { return m_name; }

		void SetTitle(const std::string& title) { m_name = title; }

		int GetX() const { return (int)m_RectX; }
		int GetY() const { return (int)m_RectY; }
		int GetWidth() const { return (int)m_RectWidth; }
		int GetHeight() const { return (int)m_RectHeight; }

		void SetDocked(bool docked) { m_IsDocked = docked; }
		bool IsDocked() const { return m_IsDocked; }

		static void InitDockSystem(float x, float y, float w, float h);
		static bool OnDockEvent(Event& event);

		static void AddWindowToRegion(uiWindow* window, DockRegion region);
		static void RemoveWindowFromDock(uiWindow* window);
		static void StopDragging();

		void SetDockRegion(DockRegion region) { m_DockRegion = region; }
		DockRegion GetDockRegion() const { return m_DockRegion; }
		void SetAutoHide(bool autoHide) { m_AutoHide = autoHide; }
		bool IsAutoHide() const { return m_AutoHide; }
		void SetFloating(bool floating) { m_IsFloating = floating; }
		bool IsFloating() const { return m_IsFloating; }

		void StartDrag(float mouseX, float mouseY);
		void OnDragMove(float mouseX, float mouseY);
		void EndDrag();

		static void DockWindow(uiWindow* window, DockRegion region);
		static void DrawDockAreas();
		static void DrawDockArea(DockRegion region);
		static void UpdateDockLayout();
		static void DrawDockPanel(float mx, float my);
		static DockRegion GetButtonAt(float mx, float my);

		static DockRegion GetPreviewRegion() { return s_PreviewRegion; }
		static float GetMainX() { return s_MainX; }
		static float GetMainY() { return s_MainY; }
		static float GetMainW() { return s_MainW; }
		static float GetMainH() { return s_MainH; }
		static float GetDynamicLeftWidth() { return s_DynamicLeftWidth; }
		static float GetDynamicRightWidth() { return s_DynamicRightWidth; }
		static float GetDynamicBottomHeight() { return s_DynamicBottomHeight; }
		static float GetDockSpacing() { return s_DockSpacing; }
		static void SetDockSpacing(float spacing) { s_DockSpacing = spacing; }
		static float GetCenterX() { return s_CenterX; }
		static float GetCenterY() { return s_CenterY; }
		static float GetCenterW() { return s_CenterW; }
		static float GetCenterH() { return s_CenterH; }
	public:
		float m_RectX = 100.0f;
		float m_RectY = 150.0f;
		float m_RectWidth = 500.0f;
		float m_RectHeight = 700.0f;

		std::string m_name = "uiWindow";
		float m_Color[4] = {};

		bool m_IsResizing = false;
		ResizeEdge m_ResizeEdge = ResizeEdge::None;
		float m_ResizeStartX = 0.0f, m_ResizeStartY = 0.0f;
		float m_ResizeStartRectX = 0.0f, m_ResizeStartRectY = 0.0f;
		float m_ResizeStartWidth = 0.0f, m_ResizeStartHeight = 0.0f;
		float m_MinWidth = 10.0f, m_MinHeight = 10.0f;
		int m_EdgeSize = 5;

		float m_CloseButtonSize = 20.0f;
		bool m_IsDragging = false;
		bool m_IsDocked = false;
		float m_DragStartX = 0.0f, m_DragStartY = 0.0f;
		float m_WindowStartX = 0.0f, m_WindowStartY = 0.0f;
		bool m_IsVisible = true;
		bool m_CloseButtonHovered = false;

		std::vector<uiButton*> m_Buttons;

		struct DockArea
		{
			std::vector<uiWindow*> windows;
			uiWindow* activeWindow = nullptr;
			DockRegion region;
			float x, y, w, h;
			float tabHeight = 25.0f;

			uiWindow* GetWindow()
			{
				if (windows.empty()) return nullptr;
				return windows[0];
			}
			void SetActiveWindow(uiWindow* window)
			{
				auto it = std::find(windows.begin(), windows.end(), window);
				if (it != windows.end())
				{
					activeWindow = window;
					std::iter_swap(windows.begin(), it);
				}
			}
		};

		static std::unordered_map<DockRegion, DockArea> s_DockAreas;
		static std::vector<uiWindow*> s_FloatingWindows;
		static uiWindow* s_DraggingWindow;
		static DockRegion s_PreviewRegion;
		static float s_MainX, s_MainY, s_MainW, s_MainH;
		static float s_DragStartX, s_DragStartY;
		static float s_DynamicLeftWidth;
		static float s_DynamicRightWidth;
		static float s_DynamicBottomHeight;

		static bool s_ShowDockPanel;
		static DockRegion s_HighlightedButton;

		static uiWindow* s_FocusedFloatingWindow;

		DockRegion m_DockRegion = DockRegion::None;
		bool m_AutoHide = false;
		bool m_IsFloating = false;

		bool m_IsDraggingForDock = false;

		static std::unordered_map<DockRegion, uiWindow*> s_DockedWindows;

		static float s_LeftWidth;
		static float s_RightWidth;
		static float s_BottomHeight;

		static float s_DockSpacing, s_CenterX, s_CenterY, s_CenterW, s_CenterH;
		static DockRegion DetectDockTarget(float mx, float my);
		static void UndockWindow(uiWindow* window);
		static void ToggleAutoHide(uiWindow* window);
		static bool HandleDockAreaEvent(DockArea& area, Event& event);
		static void DockWindowToRegion(uiWindow* window, DockRegion region);

		void DrawTitle() const;
		bool IsInResizeZone(float mx, float my) const;
		void SetResizeCursor(bool isResizeZone);
		ResizeEdge GetResizeEdge(float mx, float my) const;
		void UpdateResizeCursor(ResizeEdge edge);
		void DrawResizeGrip() const;
	};

}
