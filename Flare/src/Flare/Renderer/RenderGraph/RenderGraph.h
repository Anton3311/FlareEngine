#pragma once

#include "FlareCore/Collections/Span.h"

#include "FlareECS/Entity/Entity.h"

#include "Flare/Renderer/RenderGraph/DependecyGraph.h"
#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"
#include "Flare/Renderer/RenderGraph/RenderGraphPassSpecifications.h"
#include "Flare/Renderer/RenderGraph/RenderPassNode.h"
#include "Flare/Renderer/RenderGraph/RenderGraphCommon.h"
#include "Flare/Renderer/RenderGraph/RenderGraphResourceManager.h"

#include <optional>

namespace Flare
{
	class CommandBuffer;
	struct RenderView;
	struct SceneSubmition;
	class World;

	// TODO: Shouldn't be RefCounted
	class FLARE_API RenderGraph : public RefCounted<RenderGraph>
	{
	protected:
		RenderGraph(World& renderWorld, Entity viewportEntity);
	public:
		virtual ~RenderGraph() = default;

		void AddPass(const RenderGraphPassSpecifications& specifications, Ref<RenderGraphPass> pass);
		void InsertPass(const RenderGraphPassSpecifications& specifications, Ref<RenderGraphPass> pass, size_t index);

		inline RenderGraphTextureId CreateTexture(TextureFormat format, std::string_view debugName)
		{
			return m_ResourceManager.CreateTexture(format, debugName);
		}

		inline RenderGraphTextureId CreateTexture(TextureFormat format, std::string_view debugName, float scale)
		{
			return m_ResourceManager.CreateTexture(format, debugName, scale);
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

		bool IsPassEnabled(size_t index) const;
		void SetPassEnabled(size_t index, bool enabled);

		inline bool NeedsRebuilding() const { return m_NeedsRebuilding; }
		inline void SetNeedsRebuilding() { m_NeedsRebuilding = true; }

		inline RenderGraphResourceManager& GetResourceManager() { return m_ResourceManager; }
		inline const RenderGraphResourceManager& GetResourceManager() const { return m_ResourceManager; }

		inline const std::vector<RenderPassNode>& GetNodes() const { return m_Nodes; }
		inline const std::vector<ExternalRenderGraphResource>& GetExternalResources() const { return m_ExternalResources; }
		inline const DependencyGraph& GetDependencyGraph() const { return m_DependencyGraph; }

		static Ref<RenderGraph> Create(World& renderWorld, Entity viewportEntity);
	protected:
		virtual void ExecuteRenderPasses(Ref<CommandBuffer> commandBuffer, const SceneSubmition& sceneSubmition, const RenderView& view) = 0;
		virtual void OnPrepare() = 0;
		virtual void OnTexturesResize() = 0;
		virtual void OnClear() = 0;
		virtual void OnBuild() = 0;
	protected:
		Entity m_ViewportEntity;
		World& m_RenderWorld;
	private:
		bool m_IsValid = false;

		std::vector<RenderPassNode> m_Nodes;
		std::vector<ExternalRenderGraphResource> m_ExternalResources;

		RenderGraphResourceManager m_ResourceManager;
		DependencyGraph m_DependencyGraph;

		bool m_NeedsRebuilding = false;
	protected:
		CompiledRenderGraph m_CompiledRenderGraph;
	};
}
