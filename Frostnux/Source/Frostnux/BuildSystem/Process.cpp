#include "fxpch.h"
#include "Process.h"

#ifndef FX_PLATFORM_WINDOWS
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#endif

namespace Frostnux {

#ifdef FX_PLATFORM_WINDOWS

	static std::string GetTempFilePath()
	{
		char tmpDir[MAX_PATH] = {};
		GetTempPathA(MAX_PATH, tmpDir);

		char tmpFile[MAX_PATH] = {};
		GetTempFileNameA(tmpDir, "fxp", 0, tmpFile);
		return tmpFile;
	}

	static std::string QuoteArg(const std::string& s)
	{
		if (!s.empty()
			&& s.find_first_of(" \t\n\v\"") == std::string::npos)
			return s;

		std::string out = "\"";
		for (size_t i = 0; i < s.size(); ++i)
		{
			if (s[i] == '\\' && i + 1 < s.size() && s[i + 1] == '"')
			{
				out += "\\\\\"";
				++i;
			}
			else if (s[i] == '"')
			{
				out += "\\\"";
			}
			else
			{
				out += s[i];
			}
		}
		out += "\"";
		return out;
	}

	ProcessResult Process::Run(const std::string& command, const std::vector<std::string>& args, const std::string& workingDir)
	{
		ProcessResult result;

		std::string cmdLine = QuoteArg(command);
		for (const auto& a : args)
		{
			cmdLine += " ";
			cmdLine += QuoteArg(a);
		}

		const std::string outFile = GetTempFilePath();

		SECURITY_ATTRIBUTES sa = {};
		sa.nLength = sizeof(sa);
		sa.bInheritHandle = TRUE;
		sa.lpSecurityDescriptor = nullptr;

		HANDLE hFile = CreateFileA(outFile.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);

		if (hFile == INVALID_HANDLE_VALUE)
			return result;

		STARTUPINFOA si = {};
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdOutput = hFile;
		si.hStdError = hFile;
		si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

		PROCESS_INFORMATION pi = {};

		std::vector<char> cmdBuf(cmdLine.begin(), cmdLine.end());
		cmdBuf.push_back('\0');

		const char* cwd = workingDir.empty() ? nullptr : workingDir.c_str();

		BOOL ok = CreateProcessA(nullptr, cmdBuf.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, cwd, &si, &pi);

		CloseHandle(hFile);

		if (!ok)
		{
			DeleteFileA(outFile.c_str());
			return result;
		}

		WaitForSingleObject(pi.hProcess, INFINITE);

		DWORD exitCode = 0;
		GetExitCodeProcess(pi.hProcess, &exitCode);
		result.exitCode = static_cast<int>(exitCode);

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		std::ifstream in(outFile, std::ios::binary);
		if (in)
		{
			std::stringstream ss;
			ss << in.rdbuf();
			result.output = ss.str();
		}

		DeleteFileA(outFile.c_str());
		return result;
	}

	bool Process::Exists(const std::string& command)
	{
		const std::string cmd = "where " + command + " >nul 2>&1";
		return std::system(cmd.c_str()) == 0;
	}

#else  // Linux

	ProcessResult Process::Run(const std::string& command, const std::vector<std::string>& args, const std::string& workingDir)
	{
		ProcessResult result;

		char tmpName[] = "/tmp/frostnux_XXXXXX";
		const int fd = mkstemp(tmpName);
		if (fd < 0) return result;

		const pid_t pid = fork();
		if (pid < 0)
		{
			close(fd);
			unlink(tmpName);
			return result;
		}

		if (pid == 0)
		{
			dup2(fd, STDOUT_FILENO);
			dup2(fd, STDERR_FILENO);
			close(fd);

			if (!workingDir.empty())
				chdir(workingDir.c_str());

			std::vector<char*> argv;
			argv.push_back(const_cast<char*>(command.c_str()));
			for (const auto& a : args)
				argv.push_back(const_cast<char*>(a.c_str()));
			argv.push_back(nullptr);

			execvp(command.c_str(), argv.data());
			_exit(127);
		}

		close(fd);

		int status = 0;
		waitpid(pid, &status, 0);

		if (WIFEXITED(status))
			result.exitCode = WEXITSTATUS(status);
		else if (WIFSIGNALED(status))
			result.exitCode = 128 + WTERMSIG(status);

		std::ifstream in(tmpName, std::ios::binary);
		if (in)
		{
			std::stringstream ss;
			ss << in.rdbuf();
			result.output = ss.str();
		}

		unlink(tmpName);
		return result;
	}

	bool Process::Exists(const std::string& command)
	{
		const std::string cmd = "command -v " + command + " >/dev/null 2>&1";
		return std::system(cmd.c_str()) == 0;
	}

#endif

}
