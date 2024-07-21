#pragma once

#include "FlareCore/Collections/Span.h"

#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"
#include "Flare/Renderer/RenderGraph/RenderPassNode.h"
#include "Flare/Renderer/RenderGraph/RenderGraphCommon.h"
#include "Flare/Renderer/RenderGraph/RenderGraphResourceManager.h"

#include <optional>

namespace Flare
{
	class Viewport;
	class FrameBuffer;

	struct RenderView;

	class FLARE_API RenderGraph
	{
	public:
		RenderGraph(const Viewport& viewport);

		void AddPass(const RenderGraphPassSpecifications& specifications, Ref<RenderGraphPass> pass);
		void InsertPass(const RenderGraphPassSpecifications& specifications, Ref<RenderGraphPass> pass, size_t index);

		inline RenderGraphResourceManager& GetResourceManager() { return m_ResourceManager; }
		inline const RenderGraphResourceManager& GetResourceManager() const { return m_ResourceManager; }

		inline RenderGraphTextureId CreateTexture(TextureFormat format, std::string_view debugName)
		{
			return m_ResourceManager.CreateTexture(format, debugName);
		}

		inline Ref<Texture> GetTexture(RenderGraphTextureId textureId) const { return m_ResourceManager.GetTexture(textureId); }
		inline bool IsValid() const { return m_IsValid; }

		const RenderPassNode* GetRenderPassNode(size_t index) const;
		std::optional<size_t> FindPassByName(std::string_view name) const;

		void AddExternalResource(const ExternalRenderGraphResource& resource);

		void Execute(Ref<CommandBuffer> commandBuffer, const SceneSubmition& sceneSubmition, const RenderView& view);
		void Build();
		void Clear();

		void OnViewportResize();

		inline bool NeedsRebuilding() const { return m_NeedsRebuilding; }
		inline void SetNeedsRebuilding() { m_NeedsRebuilding = true; }
	private:
		void CreateRenderTargets();
		void ExecuteLayoutTransitions(Ref<CommandBuffer> commandBuffer, LayoutTransitionsRange range);
	private:
		bool m_IsValid = false;
		const Viewport& m_Viewport;

		std::vector<RenderPassNode> m_Nodes;
		std::vector<ExternalRenderGraphResource> m_ExternalResources;

		RenderGraphResourceManager m_ResourceManager;

		CompiledRenderGraph m_CompiledRenderGraph;

		bool m_NeedsRebuilding = false;
	};
}
