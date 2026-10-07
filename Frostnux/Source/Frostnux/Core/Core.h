#pragma once

#ifdef FX_PLATFORM_WINDOWS
#elif defined(FX_PLATFORM_LINUX)
#else
	#error Frostnux Studio only supports Windows and Linux for now!
#endif

#ifdef FX_ENABLE_ASSERTS
	#define FX_ASSERT(x, ...) { if(!(x)) { FX_ERROR("Assertion Failed: {0}", __VA_ARGS__); /*__debugbreak();*/ } }
	#define FX_CORE_ASSERT(x, ...) { if(!(x)) { FX_CORE_ERROR(__VA_ARGS__); /*__debugbreak();*/ } }
#else
	#define FX_ASSERT(x, ...)
	#define FX_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)

#define FX_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }
