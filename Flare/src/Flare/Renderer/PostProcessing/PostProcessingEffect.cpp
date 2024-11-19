#include "PCH.h"

#include "PostProcessingEffect.h"

#include "Flare/Renderer/PostProcessing/PostProcessingManager.h"

namespace Flare
{
	PostProcessingEffect::PostProcessingEffect(PostProcessingExecutionOrder executionOrder)
		: m_ExecutionOrder(executionOrder)
	{
	}

	void PostProcessingEffect::OnAttach(PostProcessingManager& postProcessingManager)
	{
		m_PostProcessingManager = &postProcessingManager;
	}

	void PostProcessingEffect::SetEnabled(bool enabled)
	{
		if (m_IsEnabled == enabled)
			return;

		m_IsEnabled = enabled;

		m_PostProcessingManager->MarkAsDirty();
	}
}