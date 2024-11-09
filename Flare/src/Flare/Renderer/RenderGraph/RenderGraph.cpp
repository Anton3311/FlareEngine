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

	void RenderGraph::Build()
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(!m_IsValid);

		m_DependencyGraph = DependecyGraph(Span<const RenderPassNode>(m_Nodes.data(), m_Nodes.size()));
		m_DependencyGraph.Build();

		OnBuild();
		
		m_NeedsRebuilding = false;
		m_IsValid = true;
	}

	void RenderGraph::Clear()
	{
		FLARE_PROFILE_FUNCTION();
		m_Nodes.clear();
		m_CompiledRenderGraph.Reset();
		m_ResourceManager.Clear();

		OnClear();

		m_IsValid = false;
	}

	void RenderGraph::Prepare()
	{
		FLARE_PROFILE_FUNCTION();

		if (m_ResourceManager.ResizeTextures())
		{
			// Textures were resized, so recreate render targets
			OnTexturesResize();
		}

		OnPrepare();
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
