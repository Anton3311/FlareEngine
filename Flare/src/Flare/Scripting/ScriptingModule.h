#pragma once

#include "Flare/Core/Core.h"
#include "FlarePlatform/Platform.h"

#include <filesystem>
#include <optional>

namespace Flare
{
	using ScriptingModuleFunction = void(*)();

	class ScriptingModule
	{
	public:
		ScriptingModule();
		ScriptingModule(const std::filesystem::path& path);
		ScriptingModule(const ScriptingModule&) = delete;
		ScriptingModule(ScriptingModule&& other) noexcept;
		~ScriptingModule();

		void Load(const std::filesystem::path& path);
		bool IsLoaded() const { return m_Library != nullptr; }

		template<typename T>
		std::optional<T> LoadFunction(const std::string& name) const
		{
			void* function = Platform::LoadFunction(m_Library, name);
			if (function != nullptr)
				return (T)function;
			return {};
		}
	private:
		void* m_Library;
	};
}