#pragma once

#include "FlareCore/Core.h"

#include "Flare/Renderer/CommandBuffer.h"

namespace Flare
{
	class FLARE_API GraphicsContext
	{
	public:
		virtual ~GraphicsContext() = default;

		virtual void Initialize() = 0;
		virtual void Release() = 0;
		virtual void BeginFrame() = 0;
		virtual void Present() = 0;

		virtual void WaitForDevice() = 0;
		virtual Ref<CommandBuffer> GetCommandBuffer() const = 0;
	public:
		static GraphicsContext& GetInstance();
		static bool IsInitialized();
		static void Create(void* windowHandle);
		static void Shutdown();
	};
}