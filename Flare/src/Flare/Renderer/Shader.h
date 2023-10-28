#pragma once

#include "FlareCore/Core.h"

#include "Flare/AssetManager/Asset.h"

#include <glm/glm.hpp>

#include <filesystem>
#include <optional>

namespace Flare
{
	enum class ShaderDataType
	{
		Int,
		Int2,
		Int3,
		Int4,
		Float,
		Float2,
		Float3,
		Float4,

		Sampler,
		SamplerArray,

		Matrix4x4,
	};

	constexpr uint32_t ShaderDataTypeSize(ShaderDataType dataType)
	{
		switch (dataType)
		{
		case ShaderDataType::Int:
		case ShaderDataType::Float:
			return 4;
		case ShaderDataType::Float2:
			return 4 * 2;
		case ShaderDataType::Float3:
			return 4 * 3;
		case ShaderDataType::Float4:
			return 4 * 4;
		case ShaderDataType::Matrix4x4:
			return 4 * 4 * 4;
		case ShaderDataType::Sampler:
			return 4;
		}

		return 0;
	}

	constexpr uint32_t ShaderDataTypeComponentCount(ShaderDataType dataType)
	{
		switch (dataType)
		{
		case ShaderDataType::Int:
		case ShaderDataType::Float:
			return 1;
		case ShaderDataType::Float2:
			return 2;
		case ShaderDataType::Float3:
			return 3;
		case ShaderDataType::Float4:
			return 4;
		case ShaderDataType::Matrix4x4:
			return 16;
		}

		return 0;
	}

	std::filesystem::path GetShaderCachePath(const std::filesystem::path& cacheDirectory,
		const std::filesystem::path& initialPath,
		std::string_view apiName,
		std::string_view stageName);

	struct ShaderParameter
	{
		ShaderParameter(std::string_view name, ShaderDataType type, size_t offset)
			: Name(name),
			Type(type),
			Offset(offset),
			Size(ShaderDataTypeSize(type)) {}

		ShaderParameter(std::string_view name, ShaderDataType type, size_t size, size_t offset)
			: Name(name),
			Type(type),
			Size(size),
			Offset(offset) {}

		std::string Name;
		ShaderDataType Type;
		size_t Offset;
		size_t Size;
	};

	using ShaderProperties = std::vector<ShaderParameter>;
	// Indices of frame buffer attachments to which the shader writes
	using ShaderOutputs = std::vector<uint32_t>;

	class FLARE_API Shader : public Asset
	{
	public:
		Shader()
			: Asset(AssetType::Shader) {}

		virtual ~Shader() = default;

		virtual void Bind() = 0;

		virtual const ShaderProperties& GetProperties() const = 0;
		virtual const ShaderOutputs& GetOutputs() const = 0;
		virtual std::optional<uint32_t> GetPropertyIndex(std::string_view name) const = 0;

		virtual void SetInt(const std::string& name, int value) = 0;
		virtual void SetFloat(const std::string& name, float value) = 0;
		virtual void SetFloat2(const std::string& name, glm::vec2 value) = 0;
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) = 0;
		virtual void SetFloat4(const std::string& name, const glm::vec4& value) = 0;
		virtual void SetIntArray(const std::string& name, const int* values, uint32_t count) = 0;
		virtual void SetMatrix4(const std::string& name, const glm::mat4& matrix) = 0;
	public:
		static Ref<Shader> Create(const std::filesystem::path& path);
		static Ref<Shader> Create(const std::filesystem::path& path, const std::filesystem::path& cacheDirectory);

		static std::string GetCacheFileName(const std::string& shaderFileName, std::string_view apiName, std::string_view stageName);
	};
}