#include "Shader.h"

#include "Flare/Platform/OpenGL/OpenGLShader.h"
#include "Flare/Renderer/RendererAPI.h"

namespace Flare
{
	static std::filesystem::path SHADER_CACHE_LOCATION = "Cache/Shaders/";

	std::filesystem::path GetShaderCachePath(const std::filesystem::path& cacheDirectory,
		const std::filesystem::path& initialPath,
		std::string_view apiName,
		std::string_view stageName)
	{
		std::filesystem::path parent = std::filesystem::relative(initialPath.parent_path(), std::filesystem::current_path());
		std::filesystem::path cachePath = cacheDirectory / parent / fmt::format("{0}.{1}.cache.{2}", initialPath.filename().string(), apiName, stageName);

		FLARE_CORE_INFO("Cache path: {}", cachePath.string());

		return cachePath;
	}

	Ref<Shader> Shader::Create()
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLShader>();
		}

		return nullptr;
	}

	Ref<Shader> Shader::Create(const std::filesystem::path& path)
	{
		std::filesystem::path cacheDirection = SHADER_CACHE_LOCATION / std::filesystem::relative(path.parent_path(), std::filesystem::current_path());

		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLShader>(path, cacheDirection);
		}

		return nullptr;
	}

	Ref<Shader> Shader::Create(const std::filesystem::path& path, const std::filesystem::path& cacheDirectory)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLShader>(path, cacheDirectory);
		}

		return nullptr;
	}

	std::string Shader::GetCacheFileName(const std::string& shaderFileName, std::string_view apiName, std::string_view stageName)
	{
		return fmt::format("{}.{}.cache.{}", shaderFileName, apiName, stageName);
	}
}
