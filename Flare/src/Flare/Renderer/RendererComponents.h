#pragma once

#include "FlareCore/Serialization/TypeSerializer.h"
#include "FlareCore/Serialization/SerializationStream.h"
#include "FlareECS/Entity/ComponentInitializer.h"

#include "Flare/Renderer/RenderGraph/RenderGraph.h"

#include <glm/glm.hpp>

namespace Flare
{
	class DescriptorSet;
	class FrameBuffer;
	class GPUBuffer;
	class Texture;

	struct ViewportSettings
	{
		constexpr bool operator==(ViewportSettings other) const
		{
			return PostProcessingEnabled == other.PostProcessingEnabled
				&& ShadowMappingEnabled == other.ShadowMappingEnabled
				&& DebugRenderingEnabled == other.DebugRenderingEnabled;
		}

		constexpr bool operator!=(ViewportSettings other) const
		{
			return !operator==(other);
		}

		bool PostProcessingEnabled = true;
		bool ShadowMappingEnabled = true;
		bool DebugRenderingEnabled = false;
	};

	struct FLARE_API Viewport
	{
		FLARE_COMPONENT;

		inline bool IsValid() const { return Size.x != 0 && Size.y != 0; }
		inline float GetAspectRatio() const { return (float)Size.x / (float)Size.y; }

		glm::uvec2 Size = glm::uvec2(0, 0);
		glm::uvec2 Position = glm::uvec2(0, 0);
		ViewportSettings Settings;
	};

	struct ViewportRenderGraphState
	{
		FLARE_COMPONENT;

		glm::uvec2 RenderTargetSize = glm::uvec2(0, 0);
		ViewportSettings Settings;
	};

	struct FLARE_API ViewportFrameResources
	{
		FLARE_NONCOPYABLE(ViewportFrameResources);

		ViewportFrameResources() = default;
		~ViewportFrameResources();

		ViewportFrameResources(ViewportFrameResources&&) = default;
		ViewportFrameResources& operator=(ViewportFrameResources&&) = default;

		Ref<GPUBuffer> CameraBuffer = nullptr;
		Ref<GPUBuffer> LightBuffer = nullptr;
		Ref<GPUBuffer> PointLightsBuffer = nullptr;
		Ref<GPUBuffer> SpotLightsBuffer = nullptr;
		Ref<GPUBuffer> ShadowDataBuffer = nullptr;

		Ref<DescriptorSet> CameraDescriptorSet = nullptr; // Set 0
		Ref<DescriptorSet> GlobalDescriptorSet = nullptr; // Set 1
		Ref<DescriptorSet> GlobalDescriptorSetWithoutShadows = nullptr; // Set 1, but with all shadow cascades set to white textures
	};

	struct FLARE_API ViewportGlobalResources
	{
		FLARE_COMPONENT;

		FLARE_NONCOPYABLE(ViewportGlobalResources);

		ViewportGlobalResources() = default;

		ViewportGlobalResources(ViewportGlobalResources&&) = default;
		ViewportGlobalResources& operator=(ViewportGlobalResources&&) = default;

		void CreateResources();

		const ViewportFrameResources& GetCurrentFrameResources() const;

		void SetupGlobalDescriptorSet(const ViewportFrameResources& frameResources, Ref<DescriptorSet> set) const;
	public:
		std::vector<ViewportFrameResources> FrameResources;
	};

	struct FLARE_API ViewportColorOutput
	{
		FLARE_COMPONENT;

		RenderGraphTextureId Id;
	};

	struct FLARE_API ViewportDepthOutput
	{
		FLARE_COMPONENT;

		RenderGraphTextureId Id;
	};

	struct FLARE_API ViewportRenderGraph
	{
		FLARE_COMPONENT;

		ViewportRenderGraph() = default;

		// TODO: Shouldn't be ref counted
		Ref<RenderGraph> Graph = nullptr;
	};
}
