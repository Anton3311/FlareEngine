#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Assert.h"

#include <optional>
#include <stdint.h>

namespace Flare
{
	enum class BlendMode : uint8_t
	{
		Opaque,
		Transparent,
	};

	enum class CullingMode : uint8_t
	{
		None,
		Back,
		Front,
	};

	enum class DepthComparisonFunction : uint8_t
	{
		Less,
		Greater,

		LessOrEqual,
		GreaterOrEqual,

		Equal,
		NotEqual,

		Never,
		Always,
	};

	FLARE_API const char* CullingModeToString(CullingMode mode);
	FLARE_API CullingMode CullingModeFromString(std::string_view mode);
	FLARE_API const char* DepthComparisonFunctionToString(DepthComparisonFunction function);
	FLARE_API std::optional<DepthComparisonFunction> DepthComparisonFunctionFromString(std::string_view function);

	enum class ShaderTargetEnvironment
	{
		OpenGL,
		Vulkan,
	};

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

	enum class ShaderStageType
	{
		Vertex,
		Pixel
	};

	struct ShaderFeatures
	{
		ShaderFeatures()
			: Blending(BlendMode::Opaque),
			Culling(CullingMode::Back),
			DepthFunction(DepthComparisonFunction::Less),
			DepthTesting(true) {}

		BlendMode Blending;
		CullingMode Culling;
		DepthComparisonFunction DepthFunction;
		bool DepthTesting;
	};
}