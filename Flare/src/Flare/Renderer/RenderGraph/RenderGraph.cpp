#include "PCH.h"

#include "RenderGraph.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererAPI.h"

#include "Flare/Renderer/RenderGraph/DependecyGraph.h"

#include "Flare/Platform/Vulkan/RenderGraph/VulkanRenderGraph.h"

namespace Flare
{
	RenderGraph::RenderGraph(World& renderWorld, Entity viewportEntity)
		: m_ResourceManager(renderWorld, viewportEntity), m_ViewportEntity(viewportEntity), m_RenderWorld(renderWorld)
	{
	}

	void RenderGraph::AddPass(const RenderGraphPassSpecifications& specifications, Ref<RenderGraphPass> pass)
	{
		FLARE_CORE_ASSERT(pass != nullptr);

		auto& node = m_Nodes.emplace_back();
		node.Pass = pass;
		node.Specifications = specifications;
	}

	void RenderGraph::InsertPass(const RenderGraphPassSpecifications& specifications, Ref<RenderGraphPass> pass, size_t index)
	{
		FLARE_CORE_ASSERT(pass != nullptr);
		FLARE_CORE_ASSERT(index <= m_Nodes.size());

		RenderPassNode passNode{};
		passNode.Pass = pass;
		passNode.Specifications = specifications;
		m_Nodes.insert(m_Nodes.begin() + index, passNode);
	}

	const RenderPassNode* RenderGraph::GetRenderPassNode(size_t index) const
	{
		if (index < m_Nodes.size())
			return &m_Nodes[index];

		return nullptr;
	}

	std::optional<size_t> RenderGraph::FindPassByName(std::string_view name) const
	{
		FLARE_PROFILE_FUNCTION();
		for (size_t i = 0; i < m_Nodes.size(); i++)
		{
			if (m_Nodes[i].Specifications.GetDebugName() == name)
			{
				return i;
			}
		}

		return {};
	}

	void RenderGraph::AddExternalResource(const ExternalRenderGraphResource& resource)
	{
		FLARE_CORE_ASSERT(resource.FinalLayout != ImageLayout::Undefined);
		m_ExternalResources.push_back(resource);
	}

	void RenderGraph::Execute(Ref<CommandBuffer> commandBuffer, const SceneSubmition& sceneSubmition, const RenderView& view)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_VERIFY(m_IsValid);

		if (m_NeedsRebuilding)
		{
			m_IsValid = false;
			Build();
		}

		std::unordered_set<RenderGraphTextureId> updatedTextures;
		m_ResourceManager.UpdateOnDemandAllocatedTextures(updatedTextures);

		OnAfterAllocatingOnDemandTextures(updatedTextures);

		ExecuteRenderPasses(std::move(commandBuffer), sceneSubmition, view);
	}

	void RenderGraph::Build()
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(!m_IsValid);

		m_DependencyGraph = DependencyGraph(Span<const RenderPassNode>(m_Nodes.data(), m_Nodes.size()));
		m_DependencyGraph.Build();

		OnBuild();
		
		m_NeedsRebuilding = false;
		m_IsValid = true;

		for (size_t i : m_DependencyGraph.GetExecutionOrder())
		{
			for (const auto& output : m_Nodes[i].Specifications.GetOutputs())
			{
				m_ResourceManager.GetTextureResource(output.AttachmentTexture).WritingPassesCount++;
			}
		}
	}

	void RenderGraph::Clear()
	{
		FLARE_PROFILE_FUNCTION();

		for (size_t i : m_DependencyGraph.GetExecutionOrder())
		{
			for (const auto& output : m_Nodes[i].Specifications.GetOutputs())
			{
				m_ResourceManager.GetTextureResource(output.AttachmentTexture).WritingPassesCount--;
			}
		}

		m_Nodes.clear();
		m_CompiledRenderGraph.Reset();
		m_ResourceManager.Clear();
		m_ExternalResources.clear();

		OnClear();

		m_IsValid = false;
	}

	void RenderGraph::Prepare()
	{
		FLARE_PROFILE_FUNCTION();

#if 0
		if (m_ResourceManager.ResizeTextures())
		{
			// Textures were resized, so recreate render targets
			OnTexturesResize();
		}
#endif

		OnPrepare();
	}

	bool RenderGraph::IsPassEnabled(size_t index) const
	{
		FLARE_CORE_ASSERT(index < m_Nodes.size());
		return m_Nodes[index].Enabled;
	}

	void RenderGraph::SetPassEnabled(size_t index, bool enabled)
	{
		FLARE_CORE_ASSERT(index < m_Nodes.size());

		bool changed = m_Nodes[index].Enabled != enabled;
	 	m_Nodes[index].Enabled = enabled;

		m_NeedsRebuilding |= changed;
	}

	Ref<RenderGraph> RenderGraph::Create(World& renderWorld, Entity viewportEntity)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return Ref<VulkanRenderGraph>::New(renderWorld, viewportEntity);
		}

		FLARE_VERIFY_UNREACHABLE();
		return nullptr;
	}
}
