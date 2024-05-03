#include "ComputeShader.h"

#include "Flare/Renderer/RendererAPI.h"

#include "Flare/Platform/Vulkan/VulkanComputeShader.h"

namespace Flare
{
	FLARE_IMPL_ASSET(ComputeShader);
	FLARE_SERIALIZABLE_IMPL(ComputeShader);
	
	ComputeShader::ComputeShader()
		: Asset(AssetType::ComputeShader) {}

	ComputeShader::~ComputeShader() {}

	Ref<ComputeShader> ComputeShader::Create()
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanComputeShader>(Span<Ref<VulkanDescriptorSetLayout>>());
		}

		return nullptr;
	}
}