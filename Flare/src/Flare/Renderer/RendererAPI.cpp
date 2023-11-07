#include "RendererAPI.h"

#include "FlareCore/Assert.h"
#include "Flare/Platform/OpenGL/OpenGLRendererAPI.h"

namespace Flare
{
	RendererAPI::API s_API = RendererAPI::API::OpenGL;

	Scope<RendererAPI> RendererAPI::Create()
	{
		switch (s_API)
		{
		case API::OpenGL:
			return CreateScope<OpenGLRendererAPI>();
		default:
			FLARE_CORE_ASSERT(false, "Unsupported rendering API");
		}

		return nullptr;
	}

	RendererAPI::API RendererAPI::GetAPI()
	{
		return s_API;
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

	CullingMode CullinModeFromString(std::string_view mode)
	{
		if (mode == "None")
			return CullingMode::None;
		if (mode == "Front")
			return CullingMode::Front;
		if (mode == "Back")
			return CullingMode::Back;

		FLARE_CORE_ASSERT(false, "Invalid culling mode");
		return CullingMode::None;
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
}