#pragma once

#include "Flare.h"
#include "Flare/Core/Layer.h"

namespace Flare
{
	class ImGUILayer : public Layer
	{
	public:
		ImGUILayer();
	public:
		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnUpdate(float deltaTime) override;

		virtual void OnEvent(Event& event) override;
		
		void Begin();
		void End();
	};
}