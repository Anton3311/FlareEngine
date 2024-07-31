#pragma once

#include "FlareCore/Core.h"

namespace Flare
{
	class Window;
	class CommandBuffer;
	class FLARE_API GraphicsContext
	{
	public:
		virtual ~GraphicsContext() = default;

		virtual void Initialize() = 0;
		virtual void Release() = 0;
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void Present() = 0;

		virtual uint32_t GetFrameInFlightCount() const = 0;
		virtual uint32_t GetCurrentFrameInFlight() const = 0;

		virtual void WaitForDevice() = 0;
		virtual Ref<CommandBuffer> GetCommandBuffer() const = 0;
	public:
		static GraphicsContext& GetInstance();
		static bool IsInitialized();
		static void Create(Ref<Window> window);
		static void Shutdown();
	};
}