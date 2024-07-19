#include "UniformBuffer.h"

#include "Flare/Renderer/RendererAPI.h"

#include "Flare/Platform/Vulkan/VulkanUniformBuffer.h"

namespace Flare
{
    Ref<UniformBuffer> Flare::UniformBuffer::Create(size_t size)
    {
        switch (RendererAPI::GetAPI())
        {
        case RendererAPI::API::Vulkan:
            return CreateRef<VulkanUniformBuffer>(size);
        }

        return nullptr;
    }
}
