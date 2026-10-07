#pragma once

#ifdef FX_PLATFORM_WINDOWS || defined(FX_PLATFORM_LINUX)

extern Frostnux::Application* Frostnux::CreateApplication();

int main(int argc, char** argv)
{
	auto app = Frostnux::CreateApplication();

	app->Run();

	delete app;
	return 0;
}

#endif