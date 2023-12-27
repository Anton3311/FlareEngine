#include "FlarePlatform/Platform.h"

#include "FlarePlatform/Windows/WindowsPlatform.h"

#include "FlareCore/Assert.h"
#include "FlareCore/Assert.h"

#include "FlarePlatform/Window.h"

#include <optional>
#include <filesystem>

#ifdef FLARE_PLATFORM_WINDOWS

#include <windows.h>
#include <commdlg.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace Flare
{
	void LogError()
	{
		DWORD errorCode = GetLastError();
		LPSTR messageBuffer = nullptr;

		size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

		std::string message(messageBuffer, size);
		FLARE_CORE_ERROR("Error {0}: {1}", errorCode, message);
	}

	float Platform::GetTime()
	{
		return (float)glfwGetTime();
	}

	void* Platform::LoadSharedLibrary(const std::filesystem::path& path)
	{
		HMODULE library = LoadLibraryW(path.c_str());
		DWORD errorCode = GetLastError();
		if (library == NULL)
		{
			FLARE_CORE_ERROR("Failed to load scripting module: {0}", path.generic_string());
			LogError();
			return nullptr;
		}
		return library;
	}

	void Platform::FreeSharedLibrary(void* library)
	{
		FLARE_CORE_ASSERT(library != nullptr);
		FreeLibrary((HMODULE)library);
	}

	void* Platform::LoadFunction(void* library, const std::string& name)
	{
		FLARE_CORE_ASSERT(library != nullptr);
		auto function = GetProcAddress((HMODULE)library, name.c_str());

		DWORD errorCode = GetLastError();
		if (function == NULL)
		{
			FLARE_CORE_ERROR("Failed to load function {0}", name);
			LogError();
			return nullptr;
		}
		else
			return function;
	}

#undef CreateProcess

	int32_t Platform::CreateProcess(std::filesystem::path& path, const ProcessCreationSettings& settings)
	{
		STARTUPINFO startUpInfo;
		PROCESS_INFORMATION processInfo;
		ZeroMemory(&startUpInfo, sizeof(startUpInfo));
		ZeroMemory(&processInfo, sizeof(processInfo));

		startUpInfo.cb = sizeof(startUpInfo);

		FLARE_CORE_ASSERT(std::filesystem::exists(settings.WorkingDirectory), "Invalid working directory");

		bool result = CreateProcessW(path.c_str(), 
			(LPWSTR)settings.Arguments.c_str(), 
			nullptr, nullptr, 
			false, 0, nullptr,
			settings.WorkingDirectory.c_str(),
			&startUpInfo,
			&processInfo);

		if (!result)
		{
			FLARE_CORE_ERROR("Failed to create process: {0}", path.generic_string());
			LogError();
			return -1;
		}
		
		if (WaitForSingleObject(processInfo.hProcess, settings.MillisecondsTimeout) == WAIT_FAILED)
		{
			FLARE_CORE_ERROR("Failed to wait for child process: {0}", path.generic_string());
			LogError();
			return -1;
		}

		DWORD exitCode;
		GetExitCodeProcess(processInfo.hProcess, &exitCode);

		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);

		return (int32_t)exitCode;
	}

	std::optional<std::filesystem::path> Platform::ShowOpenFileDialog(const wchar_t* filter, const Ref<Window>& window)
	{
		wchar_t buffer[256] = { 0 };

		OPENFILENAMEW openFile{};
		ZeroMemory(&openFile, sizeof(OPENFILENAMEW));

		GLFWwindow* nativeWindow = (GLFWwindow*)window->GetNativeWindow();

		openFile.lStructSize = sizeof(OPENFILENAMEW);
		openFile.lpstrFile = buffer;
		openFile.nMaxFile = sizeof(buffer) / sizeof(wchar_t);
		openFile.lpstrFilter = filter;
		openFile.nFilterIndex = 1;
		openFile.nMaxFileTitle = 0;
		openFile.lpstrFileTitle = nullptr;
		openFile.lpstrInitialDir = nullptr;
		openFile.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
		openFile.hwndOwner = glfwGetWin32Window((GLFWwindow*)window->GetNativeWindow());

		if (GetOpenFileNameW(&openFile) == TRUE)
			return std::filesystem::path(buffer);
		return {};
	}

	std::optional<std::filesystem::path> Platform::ShowSaveFileDialog(const wchar_t* filter, const Ref<Window>& window)
	{
		wchar_t buffer[256] = { 0 };

		OPENFILENAMEW openFile{};
		ZeroMemory(&openFile, sizeof(OPENFILENAMEW));

		openFile.lStructSize = sizeof(OPENFILENAMEW);
		openFile.lpstrFile = buffer;
		openFile.nMaxFile = sizeof(buffer) / sizeof(wchar_t);
		openFile.lpstrFilter = filter;
		openFile.nFilterIndex = 1;
		openFile.nMaxFileTitle = 0;
		openFile.lpstrFileTitle = nullptr;
		openFile.lpstrInitialDir = nullptr;
		openFile.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
		openFile.hwndOwner = glfwGetWin32Window((GLFWwindow*)window->GetNativeWindow());

		if (GetSaveFileNameW(&openFile) == TRUE)
			return std::filesystem::path(buffer);
		return {};
	}
}
#endif