#pragma once

#include "FlareCore/Core.h"

#include "FlareECS/Entity/Entity.h"

namespace Flare
{
	class PostProcessingManager;
	class RenderGraph;
	class SerializableObjectDescriptor;
	class World;

	enum class PostProcessingExecutionOrder
	{
		AfterDepthPrePass,
		EndOfFrame,
	};

	class FLARE_API PostProcessingEffect : public RefCounted<PostProcessingEffect>
	{
	public:
		PostProcessingEffect(PostProcessingExecutionOrder executionOrder = PostProcessingExecutionOrder::EndOfFrame);
		virtual ~PostProcessingEffect() = default;

		void OnAttach(PostProcessingManager& postProcessingManager);

		virtual void RegisterRenderPasses(RenderGraph& renderGraph, Entity viewportEntity, const World& renderWorld) = 0;
		virtual const SerializableObjectDescriptor& GetSerializationDescriptor() const = 0;

		inline PostProcessingExecutionOrder GetExecutionOrder() const { return m_ExecutionOrder; }
	public:
		inline bool IsEnabled() const { return m_IsEnabled; }

		void SetEnabled(bool enabled);
	private:
		PostProcessingManager* m_PostProcessingManager = nullptr;
		bool m_IsEnabled = false;
		PostProcessingExecutionOrder m_ExecutionOrder;
	};
}
