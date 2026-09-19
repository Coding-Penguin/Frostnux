#pragma once
#include "uiWindow.h"
#include <vector>
#include <string>
#include <filesystem>

namespace Frostnux {

	class PropertiesWindow : public uiWindow
	{
	public:
		PropertiesWindow(const std::string& name);
		virtual ~PropertiesWindow();

		void SetFileProperties(const std::string& filepath);
		void ClearProperties();

		virtual void OnUpdate(float deltaTime) override;
		virtual bool OnEvent(Event& event) override;

	private:
		struct PropertyItem
		{
			std::string key;
			std::string value;
		};
		std::vector<PropertyItem> m_Properties;
		float m_ScrollY = 0.0f;
		float m_ItemHeight = 25.0f;

		void DrawProperties();
		void DrawPropertyItem(const PropertyItem& item, float x, float y, float width);
	};

}