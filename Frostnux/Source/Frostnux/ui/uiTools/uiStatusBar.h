#pragma once
#include "Frostnux/Core/Layer.h"

namespace Frostnux {

	class uiStatusBar : public Layer
	{
	public:
		uiStatusBar();
		virtual ~uiStatusBar();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(float deltaTime) override;
		virtual bool OnEvent(Event& event) override;

		void SetStatusText(const std::string& text);

		void SetProgress(float progress);
	private:
		void DrawText();
		void DrawProgress() const;

		std::string m_StatusText;

		float m_Progress = -1.0f;
		float m_Height = 35.0f;
	};

}
