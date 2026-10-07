#pragma once
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace Frostnux {

	class Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};

}

// Core Log Macros
#define FX_CORE_DEBUG(...)		::Frostnux::Log::GetCoreLogger()->debug(__VA_ARGS__)
#define FX_CORE_TRACE(...)		::Frostnux::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define FX_CORE_INFO(...)		::Frostnux::Log::GetCoreLogger()->info(__VA_ARGS__)
#define FX_CORE_WARN(...)		::Frostnux::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define FX_CORE_ERROR(...)		::Frostnux::Log::GetCoreLogger()->error(__VA_ARGS__)
#define FX_CORE_FATAL(...)		::Frostnux::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client Log Macros
#define FX_DEBUG(...)		::Frostnux::Log::GetClientLogger()->debug(__VA_ARGS__)
#define FX_TRACE(...)		::Frostnux::Log::GetClientLogger()->trace(__VA_ARGS__)
#define FX_INFO(...)		::Frostnux::Log::GetClientLogger()->info(__VA_ARGS__)
#define FX_WARN(...)		::Frostnux::Log::GetClientLogger()->warn(__VA_ARGS__)
#define FX_ERROR(...)		::Frostnux::Log::GetClientLogger()->error(__VA_ARGS__)
#define FX_FATAL(...)		::Frostnux::Log::GetClientLogger()->critical(__VA_ARGS__)