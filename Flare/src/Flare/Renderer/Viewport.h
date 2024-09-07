#pragma once

#include "FlareCore/Core.h"

#include "Flare/Renderer/RenderGraph/RenderGraph.h"

#include <vector>
#include <glm/glm.hpp>

namespace Flare
{
	class DescriptorSet;
	class FrameBuffer;
	class GPUBuffer;
	class Texture;

	struct ViewportFrameResources
	{
		Ref<GPUBuffer> CameraBuffer = nullptr;
		Ref<GPUBuffer> LightBuffer = nullptr;
		Ref<GPUBuffer> PointLightsBuffer = nullptr;
		Ref<GPUBuffer> SpotLightsBuffer = nullptr;
		Ref<GPUBuffer> ShadowDataBuffer = nullptr;

		Ref<DescriptorSet> CameraDescriptorSet = nullptr; // Set 0
		Ref<DescriptorSet> GlobalDescriptorSet = nullptr; // Set 1
		Ref<DescriptorSet> GlobalDescriptorSetWithoutShadows = nullptr; // Set 1, but with all shadow cascades set to white textures
	};

	struct ViewportGlobalResources
	{
		std::vector<ViewportFrameResources> FrameResources;
	};

	class FLARE_API Viewport
	{
	public:
		Viewport();
		~Viewport();

		inline glm::ivec2 GetPosition() const { return m_Position; }
		inline glm::ivec2 GetSize() const { return m_Size; }

		inline float GetAspectRatio() const { return (float)m_Size.x / (float)m_Size.y; }

		inline const Scope<RenderGraph>& GetRenderGraph() const { return m_RenderGraph; }

		void Resize(glm::ivec2 position, glm::ivec2 size);
		void UpdateGlobalDescriptorSets();

		void OnBuildRenderGraph();
		void PrepareViewport();

		inline bool IsPostProcessingEnabled() const { return m_PostProcessingEnabled; }
		void SetPostProcessingEnabled(bool enabled);

		inline bool IsShadowMappingEnabled() const { return m_ShadowMappingEnabled; }
		void SetShadowMappingEnabled(bool enabled);

		inline bool IsDebugRenderingEnabled() const { return m_DebugRenderingEnabled; }
		void SetDebugRenderingEnabled(bool enabled);

		// Returns frame resources for the current frame in flight
		const ViewportFrameResources& GetFrameResources() const;

		// Returns frame resources of a specific frame in flight
		inline const ViewportFrameResources& GetFrameResources(uint32_t frameIndex) const { return m_GlobalResources.FrameResources[frameIndex]; }
	public:
		RenderGraphTextureId ColorTextureId;
		RenderGraphTextureId NormalsTextureId;
		RenderGraphTextureId DepthTextureId;
	private:
		void SetupGlobalDescriptorSet(const ViewportFrameResources& frameResources, Ref<DescriptorSet> set);
	private:
		bool m_PostProcessingEnabled = true;
		bool m_ShadowMappingEnabled = true;
		bool m_DebugRenderingEnabled = false;

		bool m_ShouldResizeRenderGraphTextures = false;

		Scope<RenderGraph> m_RenderGraph;

		TextureFormat m_ColorTextureFormat = TextureFormat::R11G11B10;
		TextureFormat m_NormalsTextureFormat = TextureFormat::RGB8;
		TextureFormat m_DepthTextureFormat = TextureFormat::Depth32;

		ViewportGlobalResources m_GlobalResources;
		ViewportFrameResources* m_CurrentFrameResources = nullptr;

		glm::ivec2 m_Position = glm::ivec2(0, 0);
		glm::ivec2 m_Size = glm::ivec2(0, 0);
	};
}