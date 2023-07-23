#include "ScriptingModule.h"

#include "Flare/Core/Assert.h"
#include "Flare/Platform/Platform.h"

namespace Flare
{
	ScriptingModule::ScriptingModule()
		: m_Library(nullptr)
	{
	}

	ScriptingModule::ScriptingModule(const std::filesystem::path& path)
		: m_Library(nullptr)
	{
		std::optional<void*> library = Platform::LoadSharedLibrary(path);
		m_Library = library.value_or(nullptr);
	}

	ScriptingModule::ScriptingModule(ScriptingModule&& other) noexcept
		: m_Library(other.m_Library)
	{
		other.m_Library = nullptr;
	}

	void ScriptingModule::Load(const std::filesystem::path& path)
	{
		if (m_Library != nullptr)
			Platform::FreeSharedLibrary(m_Library);
		m_Library = Platform::LoadSharedLibrary(path);
	}

	ScriptingModule::~ScriptingModule()
	{
		if (m_Library)
		{
			Platform::FreeSharedLibrary(m_Library);
			m_Library = nullptr;
		}
	}
}
