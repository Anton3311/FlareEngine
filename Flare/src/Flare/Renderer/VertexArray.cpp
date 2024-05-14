#include "VertexArray.h"

#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Platform/Vulkan/VulkanVertexArray.h"

namespace Flare
{
	Ref<VertexArray> VertexArray::Create()
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanVertexArray>();
		}

		return nullptr;
	}
}