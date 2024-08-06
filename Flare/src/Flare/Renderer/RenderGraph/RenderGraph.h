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

		void Prepare();

		inline bool NeedsRebuilding() const { return m_NeedsRebuilding; }
		inline void SetNeedsRebuilding() { m_NeedsRebuilding = true; }

		inline const DependecyGraph& GetDependecyGraph() const { return m_DependecyGraph; }

		static Scope<RenderGraph> Create(const Viewport& viewport);
	protected:
		virtual void OnPrepare() {}
		virtual void OnTexturesResize() {}
		virtual void OnClear() {}
		virtual void OnBuild() {}
	private:
		void CreateRenderTargets();
		void ExecuteLayoutTransitions(Ref<CommandBuffer> commandBuffer, LayoutTransitionsRange range);
	private:
		bool m_IsValid = false;
		const Viewport& m_Viewport;

		std::vector<RenderPassNode> m_Nodes;
		std::vector<ExternalRenderGraphResource> m_ExternalResources;
		std::vector<Ref<FrameBuffer>> m_RenderPassTargets;

		RenderGraphResourceManager m_ResourceManager;
		DependecyGraph m_DependecyGraph;

		CompiledRenderGraph m_CompiledRenderGraph;

		bool m_NeedsRebuilding = false;
	};
}
