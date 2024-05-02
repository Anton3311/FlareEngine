#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Assert.h"

#include "FlareCore/Serialization/SerializationStream.h"

#include <optional>
#include <stdint.h>

namespace Flare
{
	enum class ShaderType
	{
		Unknown,
		_2D,
		Surface,
		FullscreenQuad,
		Debug,
	};

	inline uint32_t GetMaterialDescriptorSetIndex(ShaderType type)
	{
		switch (type)
		{
		case ShaderType::_2D:
			return 2;
		case ShaderType::Surface:
			return 2;
		case ShaderType::FullscreenQuad:
			return 2;
		}

		FLARE_CORE_ASSERT(false);
		return 0;
	}

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

	enum class MeshTopology
	{
		Triangles,
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
	FLARE_API std::optional<CullingMode> CullingModeFromString(std::string_view mode);

	FLARE_API const char* DepthComparisonFunctionToString(DepthComparisonFunction function);
	FLARE_API std::optional<DepthComparisonFunction> DepthComparisonFunctionFromString(std::string_view function);

	FLARE_API const char* BlendModeToString(BlendMode blendMode);
	FLARE_API std::optional<BlendMode> BlendModeFromString(std::string_view string);

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

	struct ShaderProperty
	{
		ShaderProperty() = default;
		ShaderProperty(std::string_view name, ShaderDataType type, size_t offset)
			: Name(name),
			Type(type),
			Offset(offset),
			Binding(UINT32_MAX),
			SamplerIndex(UINT32_MAX),
			Size(ShaderDataTypeSize(type)) {}

		ShaderProperty(std::string_view name, ShaderDataType type, size_t size, size_t offset)
			: Name(name),
			Type(type),
			Size(size),
			Binding(UINT32_MAX),
			SamplerIndex(UINT32_MAX),
			Offset(offset) {}

		std::string Name;
		std::string DisplayName;
		ShaderDataType Type;
		uint32_t Binding;
		uint32_t SamplerIndex;
		size_t Offset;
		size_t Size;

		bool Hidden = true;
		SerializationValueFlags Flags = SerializationValueFlags::None;
	};

	using ShaderProperties = std::vector<ShaderProperty>;

	// Indices of frame buffer attachments to which the shader writes
	using ShaderOutputs = std::vector<uint32_t>;

	enum class ShaderStageType
	{
		Vertex,
		Pixel,
		Compute,
	};

	FLARE_API const char* ShaderStageTypeToString(ShaderStageType stage);

	struct ShaderFeatures
	{
		ShaderFeatures()
			: Blending(BlendMode::Opaque),
			Culling(CullingMode::Back),
			DepthFunction(DepthComparisonFunction::Less),
			DepthTesting(true),
			DepthWrite(true) {}

		BlendMode Blending;
		CullingMode Culling;
		DepthComparisonFunction DepthFunction;
		bool DepthTesting;
		bool DepthWrite;
	};

	struct ShaderPushConstantsRange
	{
		ShaderStageType Stage = ShaderStageType::Vertex;
		size_t Offset = 0;
		size_t Size = 0;
	};

	struct ShaderMetadata
	{
		std::string Name;
		ShaderType Type = ShaderType::Unknown;
		ShaderFeatures Features;
		ShaderOutputs Outputs;
		std::vector<ShaderProperty> Properties;
		std::vector<ShaderStageType> Stages;
		std::vector<ShaderPushConstantsRange> PushConstantsRanges;
	};
}