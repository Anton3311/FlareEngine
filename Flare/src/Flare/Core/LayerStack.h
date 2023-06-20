#pragma once

#include "Flare.h"
#include "Flare/Core/Layer.h"

#include <vector>

namespace Flare
{
	class LayerStack
	{
	public:
		LayerStack();
	public:
		void PushLayer(const Ref<Layer>& layer);
		void PushOverlay(const Ref<Layer>& layer);

		std::vector<Ref<Layer>>& GetLayers() { return m_Layers; }
		const std::vector<Ref<Layer>>& GetLayers() const { return m_Layers; }
	private:
		std::vector<Ref<Layer>> m_Layers;
		size_t m_LayerInsertPosition;
	};
}