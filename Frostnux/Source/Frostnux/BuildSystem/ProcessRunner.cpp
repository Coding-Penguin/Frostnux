#include "fxpch.h"
#include "ProcessRunner.h"

#ifdef FX_PLATFORM_WINDOWS

namespace Frostnux {

	static std::wstring Utf8ToWide(const std::string& s)
	{
		if (s.empty()) return {};
		int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
		std::wstring out(len - 1, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), len);
		return out;
	}

	static std::string WideToUtf8(const wchar_t* s)
	{
		if (!s) return {};
		int len = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr);
		if (len <= 1) return {};
		std::string out(len - 1, '\0');
		WideCharToMultiByte(CP_UTF8, 0, s, -1, out.data(), len, nullptr, nullptr);
		return out;
	}

	void ProcessRunner::Run(const std::string& command, const std::string& workingDir, std::function<void(const std::string&)> onOutput, std::function<void(int)> onExit)
	{
		std::thread([=]()
			{
			SECURITY_ATTRIBUTES saAttr;
			saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
			saAttr.bInheritHandle = TRUE;
			saAttr.lpSecurityDescriptor = NULL;

			HANDLE hStdOutRead, hStdOutWrite;
			if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &saAttr, 0)) return;
			SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

			STARTUPINFOW si;
			ZeroMemory(&si, sizeof(si));
			si.cb = sizeof(si);
			si.hStdError = hStdOutWrite;
			si.hStdOutput = hStdOutWrite;
			si.dwFlags |= STARTF_USESTDHANDLES;

			PROCESS_INFORMATION pi;
			ZeroMemory(&pi, sizeof(pi));

			std::wstring wCmd = Utf8ToWide(command);
			std::wstring wDir = Utf8ToWide(workingDir);

			BOOL success = CreateProcessW(NULL, wCmd.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, wDir.empty() ? NULL : wDir.c_str(), &si, &pi);

			if (!success)
			{
				onOutput("[Error] Failed to start process.");
				CloseHandle(hStdOutRead);
				CloseHandle(hStdOutWrite);
				onExit(-1);
				return;
			}

			CloseHandle(hStdOutWrite);

			const int bufSize = 16384;
			char buffer[bufSize];
			DWORD bytesRead;
			std::string currentLine;

			while (ReadFile(hStdOutRead, buffer, bufSize - 1, &bytesRead, NULL) && bytesRead > 0)
			{
				buffer[bytesRead] = '\0';
				std::string chunk(buffer);

				size_t start = 0;
				size_t pos;
				while ((pos = chunk.find('\n', start)) != std::string::npos)
				{
					currentLine += chunk.substr(start, pos - start);

					if (!currentLine.empty() && currentLine.back() == '\r') currentLine.pop_back();
					if (onOutput) onOutput(currentLine);
					currentLine.clear();
					start = pos + 1;
				}
				currentLine += chunk.substr(start);
			}

			if (!currentLine.empty())
			{
				if (onOutput) onOutput(currentLine);
			}

			WaitForSingleObject(pi.hProcess, INFINITE);
			DWORD exitCode;
			GetExitCodeProcess(pi.hProcess, &exitCode);

			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			CloseHandle(hStdOutRead);

			if (onExit) onExit((int)exitCode);

			}).detach();
	}

}

#endif
