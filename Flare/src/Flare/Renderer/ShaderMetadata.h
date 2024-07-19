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
		Decal,
	};

	FLARE_API uint32_t GetMaterialDescriptorSetIndex(ShaderType type);

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

		StorageImage,

		Matrix4x4,
	};

	FLARE_API uint32_t ShaderDataTypeSize(ShaderDataType dataType);
	FLARE_API uint32_t ShaderDataTypeComponentCount(ShaderDataType dataType);

	enum class ShaderDescriptorType
	{
		UniformBuffer,
		StorageBuffer,
		Sampler,
		StorageImage,
	};

	struct ShaderDescriptorProperty
	{
		ShaderDescriptorProperty(ShaderDescriptorType type)
			: Type(type) {}
		ShaderDescriptorProperty(std::string_view name, ShaderDescriptorType type, uint32_t set, uint32_t binding, uint32_t count)
			: Name(name), Type(type), Set(set), Binding(binding), DescriptorCount(count) {}

		std::string Name;
		ShaderDescriptorType Type;
		uint32_t DescriptorCount = 0;
		uint32_t Set = UINT32_MAX;
		uint32_t Binding = UINT32_MAX;
	};

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
			DepthWrite(true),
			DepthBiasEnabled(false) {}

		BlendMode Blending;
		CullingMode Culling;
		DepthComparisonFunction DepthFunction;
		bool DepthTesting;
		bool DepthWrite;
		bool DepthBiasEnabled;
		bool DepthClampEnabled;
	};

	struct ShaderPushConstantsRange
	{
		ShaderStageType Stage = ShaderStageType::Vertex;
		size_t Offset = 0;
		size_t Size = 0;
	};

	struct VertexShaderInput
	{
		uint32_t Location = 0;
		ShaderDataType Type = ShaderDataType::Float;
	};

	enum class ShaderDescriptorSetUsage
	{
		Used,
		NotUsed,

		// Empty descriptor sets are used to fill gaps.
		// 
		// For example, if the shader uses descriptor sets 0 and 2
		// an empty descriptor set is used to fill slot at index 1
		Empty,
	};

	struct ShaderMetadata
	{
		std::string Name;
		ShaderType Type = ShaderType::Unknown;
		ShaderFeatures Features;
		ShaderOutputs Outputs;

		ShaderDescriptorSetUsage DescriptorSetUsage[4] = { ShaderDescriptorSetUsage::NotUsed };

		std::vector<ShaderProperty> Properties;
		std::vector<ShaderDescriptorProperty> DescriptorProperties;
		std::vector<ShaderStageType> Stages;
		std::vector<ShaderPushConstantsRange> PushConstantsRanges;
		std::vector<VertexShaderInput> VertexShaderInputs;
	};
}