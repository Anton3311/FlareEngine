#pragma once

#include <Flare/Core/Core.h>

#include <filesystem>

namespace Flare
{
	class Shader
	{
	public:
		virtual void Bind() = 0;
	public:
		static Ref<Shader> Create(const std::filesystem::path& path);
	};
}