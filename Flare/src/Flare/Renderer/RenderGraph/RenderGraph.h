#pragma once

#include "FlareCore/Collections/Span.h"

#include "Flare/Renderer/RenderGraph/DependecyGraph.h"
#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"
#include "Flare/Renderer/RenderGraph/RenderGraphPassSpecifications.h"
#include "Flare/Renderer/RenderGraph/RenderPassNode.h"
#include "Flare/Renderer/RenderGraph/RenderGraphCommon.h"
#include "Flare/Renderer/RenderGraph/RenderGraphResourceManager.h"

#include <optional>

namespace Flare
{
	struct RenderView;
	struct SceneSubmition;

	class Commanduffer;
	class FrameBuffer;
	class Viewport;

	class FLARE_API RenderGraph
	{
	public:
		RenderGraph(const Viewport& viewport);
		virtual ~RenderGraph() = default;

		void AddPass(const RenderGraphPassSpecifications& specifications, Ref<RenderGraphPass> pass);
		void InsertPass(const RenderGraphPassSpecifications& specifications, Ref<RenderGraphPass> pass, size_t index);

		inline RenderGraphTextureId CreateTexture(TextureFormat format, std::string_view debugName)
		{
			return m_ResourceManager.CreateTexture(format, debugName);
		}

		inline Ref<Texture> GetTexture(RenderGraphTextureId textureId) const { return m_ResourceManager.GetTexture(textureId); }
		inline bool IsValid() const { return m_IsValid; }

		const RenderPassNode* GetRenderPassNode(size_t index) const;
		std::optional<size_t> FindPassByName(std::string_view name) const;

		void AddExternalResource(const ExternalRenderGraphResource& resource);

		virtual void Execute(Ref<CommandBuffer> commandBuffer, const SceneSubmition& sceneSubmition, const RenderView& view) = 0;

		void Build();
		void Clear();

		void Prepare();

		inline bool NeedsRebuilding() const { return m_NeedsRebuilding; }
		inline void SetNeedsRebuilding() { m_NeedsRebuilding = true; }

		inline RenderGraphResourceManager& GetResourceManager() { return m_ResourceManager; }
		inline const RenderGraphResourceManager& GetResourceManager() const { return m_ResourceManager; }

		inline const std::vector<RenderPassNode>& GetNodes() const { return m_Nodes; }
		inline const std::vector<ExternalRenderGraphResource>& GetExternalResources() const { return m_ExternalResources; }
		inline const DependecyGraph& GetDependecyGraph() const { return m_DependecyGraph; }
		inline const Viewport& GetViewport() const { return m_Viewport; }

		static Scope<RenderGraph> Create(const Viewport& viewport);
	protected:
		virtual void OnPrepare() {}
		virtual void OnTexturesResize() {}
		virtual void OnClear() {}
		virtual void OnBuild() {}
	private:
		bool m_IsValid = false;
		const Viewport& m_Viewport;

		std::vector<RenderPassNode> m_Nodes;
		std::vector<ExternalRenderGraphResource> m_ExternalResources;
		std::vector<Ref<FrameBuffer>> m_RenderPassTargets;

		RenderGraphResourceManager m_ResourceManager;
		DependecyGraph m_DependecyGraph;

		bool m_NeedsRebuilding = false;

	protected:
		CompiledRenderGraph m_CompiledRenderGraph;
	};
}
