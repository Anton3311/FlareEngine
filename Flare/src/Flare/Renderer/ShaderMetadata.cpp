#include "ShaderMetadata.h"

namespace Flare
{
	FLARE_API uint32_t GetMaterialDescriptorSetIndex(ShaderType type)
	{
		switch (type)
		{
		case ShaderType::_2D:
			return 2;
		case ShaderType::Surface:
			return 2;
		case ShaderType::FullscreenQuad:
			return 2;
		case ShaderType::Unknown:
			return 0;
		}

		FLARE_CORE_ASSERT(false);
		return 0;
	}

	const char* CullingModeToString(CullingMode mode)
	{
		switch (mode)
		{
		case CullingMode::Front:
			return "Front";
		case CullingMode::Back:
			return "Back";
		case CullingMode::None:
			return "None";
		}

		FLARE_CORE_ASSERT(false, "Unhandled culling mode");
		return nullptr;
	}

	std::optional<CullingMode> CullingModeFromString(std::string_view mode)
	{
		if (mode == "None")
			return CullingMode::None;
		if (mode == "Front")
			return CullingMode::Front;
		if (mode == "Back")
			return CullingMode::Back;

		return {};
	}

	const char* DepthComparisonFunctionToString(DepthComparisonFunction function)
	{
		switch (function)
		{
		case DepthComparisonFunction::Less:
			return "Less";
		case DepthComparisonFunction::Greater:
			return "Greater";
		case DepthComparisonFunction::LessOrEqual:
			return "LessOrEqual";
		case DepthComparisonFunction::GreaterOrEqual:
			return "GreaterOrEqual";
		case DepthComparisonFunction::Equal:
			return "Equal";
		case DepthComparisonFunction::NotEqual:
			return "NotEqual";
		case DepthComparisonFunction::Never:
			return "Never";
		case DepthComparisonFunction::Always:
			return "Always";
		}

		FLARE_CORE_ASSERT(false, "Unhandled depth comparison function");
		return nullptr;
	}

	std::optional<DepthComparisonFunction> DepthComparisonFunctionFromString(std::string_view function)
	{
		if (function == "Less")
			return DepthComparisonFunction::Less;
		if (function == "Greater")
			return DepthComparisonFunction::Greater;
		if (function == "LessOrEqual")
			return DepthComparisonFunction::LessOrEqual;
		if (function == "GreaterOrEqual")
			return DepthComparisonFunction::GreaterOrEqual;
		if (function == "Equal")
			return DepthComparisonFunction::Equal;
		if (function == "NotEqual")
			return DepthComparisonFunction::NotEqual;
		if (function == "Never")
			return DepthComparisonFunction::Never;
		if (function == "Always")
			return DepthComparisonFunction::Always;

		return {};
	}

	const char* BlendModeToString(BlendMode blendMode)
	{
		switch (blendMode)
		{
		case BlendMode::Opaque:
			return "Opaque";
		case BlendMode::Transparent:
			return "Transparent";
		}

		FLARE_CORE_ASSERT(false, "Unhandled material blend mode");
		return "";
	}

	std::optional<BlendMode> BlendModeFromString(std::string_view string)
	{
		if (string == "Opaque")
			return BlendMode::Opaque;
		if (string == "Transparent")
			return BlendMode::Transparent;

		return {};
	}

	FLARE_API uint32_t ShaderDataTypeSize(ShaderDataType dataType)
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
		case ShaderDataType::StorageImage:
			return 4;
		}

		return 0;
	}

	FLARE_API uint32_t ShaderDataTypeComponentCount(ShaderDataType dataType)
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

	const char* ShaderStageTypeToString(ShaderStageType stage)
	{
		switch (stage)
		{
		case ShaderStageType::Vertex:
			return "Vertex";
		case ShaderStageType::Pixel:
			return "Pixel";
		case ShaderStageType::Compute:
			return "Compute";
		}

		FLARE_CORE_ASSERT(false, "Unhandled ShaderStageType");
		return nullptr;
	}
}