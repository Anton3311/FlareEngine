#pragma once

#include "FlareCore/Core.h"
#include "Flare/Renderer/RenderGraph/RenderGraphContext.h"
#include "Flare/Renderer/RenderGraph/RenderGraphCommon.h"
#include "Flare/Renderer/RenderGraph/RenderGraphResourceManager.h"

#include "Flare/Renderer/Texture.h"

#include <string>
#include <string_view>

namespace Flare
{
	class FLARE_API RenderGraphPass : public RefCounted<RenderGraphPass>
	{
	public:
		virtual ~RenderGraphPass() = default;

		virtual void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) = 0;
		virtual void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) = 0;
	};
}
