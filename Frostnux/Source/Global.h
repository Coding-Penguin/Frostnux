#pragma once

namespace Frostnux
{

	static std::string s_MainVersion = "2026.3";
	static std::string s_DetailedVersion = "2026.3.7-2";
	static float s_FontSize = 24.0f;
	static bool s_FontLoaded = false;

	static bool CreateFolder(const std::string &path)
	{
		try
		{
			if (std::filesystem::create_directories(path))
			{
				FX_CORE_INFO("Created directory: {}", path);
				return true;
			}
			else
			{
				FX_CORE_WARN("Directory already exists: {}", path);
				return true;
			}
		}
		catch (const std::filesystem::filesystem_error &e)
		{
			FX_CORE_ERROR("Failed to create directory: {}", e.what());
			return false;
		}
	}

}

/*
关于版本号的说明：
	"yyyy.a.b-c"(c 可省略) => yyyy 年中的 a 个大版本更新中的第 b 个小版本更新中的 c 个补丁.

	a 大版本是指架构或功能上的重大更新，通常会引入新的特性、模块或改变现有的架构设计。	例如：添加编译系统
	b 小版本是指在大版本基础上的功能增强、性能优化或修复已知问题的更新。				例如：优化编译速度
	c 补丁版本是指针对特定问题的修复或小的改进，通常不会引入新的功能。				例如：修改了主题的默认颜色
*/
