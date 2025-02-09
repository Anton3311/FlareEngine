#include "PCH.h"
#include "SpotLightShadowPass.h"

#include "Flare/Renderer/Buffer.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/DescriptorSet.h"
#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/Passes/GeometryCullingPass.h"
#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererComponents.h"
#include "Flare/Renderer/SceneSubmition.h"

#include "FlareECS/World.h"

namespace Flare
{
	SpotLightShadowPass::SpotLightShadowPass(RenderGraphTextureId shadowMap, Ref<Material> perspectiveDepthOnly)
		: m_ShadowMap(shadowMap), m_PerspectiveDepthOnly(perspectiveDepthOnly)
	{
		FLARE_PROFILE_FUNCTION();

		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();
		m_FrameResources.reserve(frameInFlightCount);

		GPUBufferSpecifications specifications{};
		specifications.MemoryType = GPUBufferMemoryType::Dynamic;
		specifications.Size = sizeof(RenderView);
		specifications.Usage = GPUBufferUsage::UniformBuffer;

		constexpr uint32_t MAX_LIGHTS = 4;

		for (uint32_t frameIndex = 0; frameIndex < frameInFlightCount * MAX_LIGHTS; frameIndex++)
		{
			FrameResources& resources = m_FrameResources.emplace_back();
			resources.CameraBuffer = GPUBuffer::Create(specifications);
			resources.CameraDescriptorSet = Renderer::GetCameraDescriptorSetPool()->AllocateSet();
			resources.CameraDescriptorSet->WriteUniformBuffer(resources.CameraBuffer, 0);
			resources.CameraDescriptorSet->FlushWrites();
		}
	}

	SpotLightShadowPass::~SpotLightShadowPass()
	{
		for (auto& resources : m_FrameResources)
		{
			Renderer::GetCameraDescriptorSetPool()->ReleaseSet(resources.CameraDescriptorSet);
		}
	}

	void SpotLightShadowPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		constexpr float PROJECTION_NEAR = 0.01f;

		const SceneSubmition& sceneSubmition = context.GetSceneSubmition();
		m_HasSpotLight = sceneSubmition.ShadowCastingSpotLights.size() > 0;

		if (!m_HasSpotLight)
			return;

		size_t lightCount = sceneSubmition.ShadowCastingSpotLights.size();

		uint32_t frameCount = GraphicsContext::GetInstance().GetFrameInFlightCount();
		uint32_t frameIndex = GraphicsContext::GetInstance().GetCurrentFrameInFlight();

		std::vector<SpotLightShadowsEntry> shadowEntries;
		const ViewportGlobalResources& globalResources = context.RenderWorld.GetEntityComponent<const ViewportGlobalResources>(context.ViewportEntity);
		for (size_t lightIndex = 0; lightIndex < lightCount; lightIndex++)
		{
			const SpotLightSubmition& spotLight = sceneSubmition.SpotLights[sceneSubmition.ShadowCastingSpotLights[lightIndex]];

			float radius = glm::sqrt(spotLight.Intensity / 0.01f);

			// TODO: Get rid of acos
			float fov = glm::acos(spotLight.OuterAngleCos) * 2.0f;
			glm::mat4 projection = glm::perspectiveRH_ZO(fov, 1.0f, 0.01f, radius);

			glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
			glm::vec3 forward = spotLight.Direction;
			glm::vec3 right = -glm::cross(up, forward);

			glm::mat4 basis = static_cast<glm::mat4>(glm::mat3(right, up, forward));
			glm::mat4 viewMatrix = glm::inverse(glm::translate(basis, spotLight.Position));

			viewMatrix = glm::lookAt(spotLight.Position + spotLight.Direction, spotLight.Position, glm::vec3(0.0f, 1.0f, 0.0f));

			glm::uvec2 shadowMapSize = context.GetRenderGraphResourceManager().GetTexture(m_ShadowMap)->GetSize();

			RenderView view{};
			view.FOV = fov;
			view.Far = radius;
			view.Near = PROJECTION_NEAR;
			view.Position = spotLight.Position;
			view.ViewDirection = spotLight.Direction;
			view.ViewportSize = shadowMapSize / 2u;
			view.SetViewAndProjection(projection, viewMatrix);

			m_FrameResources[static_cast<uint32_t>(lightIndex) * frameCount + frameIndex].CameraBuffer->SetData(MemorySpan(&view, 1), 0);

			auto& entry = shadowEntries.emplace_back();
			entry.Projection = view.ViewProjection;
			entry.UVScale = glm::vec2(0.5f);
			entry.UVTranslation = glm::vec2(0.5f) * glm::vec2(static_cast<float>(lightIndex % 2), static_cast<float>(lightIndex / 2));
		}

		uint32_t shadowCastingLightsRange[2] =
		{
			static_cast<uint32_t>(sceneSubmition.SpotLights.size() - sceneSubmition.ShadowCastingSpotLights.size()),
			static_cast<uint32_t>(sceneSubmition.ShadowCastingSpotLights.size())
		};

		globalResources.FrameResources[frameIndex].SpotLightShadowDataBuffer->SetData(MemorySpan(shadowCastingLightsRange, 2), 0);
		globalResources.FrameResources[frameIndex].SpotLightShadowDataBuffer->SetData(MemorySpan::FromVector(shadowEntries), 16);
	}

	void SpotLightShadowPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		uint32_t frameIndex = GraphicsContext::GetInstance().GetCurrentFrameInFlight();
		uint32_t frameCount = GraphicsContext::GetInstance().GetFrameInFlightCount();
		const ViewportGlobalResources& globalResources = context.RenderWorld.GetEntityComponent<const ViewportGlobalResources>(context.ViewportEntity);
		const CulledGeometry& culledGeometry = context.RenderWorld.GetEntityComponent<const CulledGeometry>(context.ViewportEntity);
		const CulledGeometry::GPUFrameResources& culledGeometryResources = culledGeometry.FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];

		const SceneSubmition& sceneSubmition = context.GetSceneSubmition();

		commandBuffer->SetGlobalDescriptorSet(culledGeometryResources.InstanceBufferDescriptor, 2);

		for (size_t lightIndex = 0; lightIndex < sceneSubmition.ShadowCastingSpotLights.size(); lightIndex++)
		{
			const FrameResources& frameResources = m_FrameResources[lightIndex * frameCount + frameIndex];
			commandBuffer->SetGlobalDescriptorSet(frameResources.CameraDescriptorSet, 0);
			commandBuffer->ApplyMaterial(m_PerspectiveDepthOnly);

			Math::Rect viewport{};
			viewport.Min = glm::vec2(0.5f) * glm::vec2(static_cast<float>(lightIndex % 2), static_cast<float>(lightIndex / 2));
			viewport.Max = viewport.Min + glm::vec2(0.5f);
			viewport.Min *= context.RenderAreaSize;
			viewport.Max *= context.RenderAreaSize;

			commandBuffer->SetViewportAndScissors(viewport);

			for (const auto& culledBatch : culledGeometry.CulledBatches)
			{
				const Ref<const Mesh>& mesh = culledBatch.OriginalBatch->GetMesh();

				commandBuffer->DrawMeshIndexed(mesh,
					static_cast<uint32_t>(culledBatch.SubMeshIndex),
					static_cast<uint32_t>(culledBatch.TransformBufferOffset),
					static_cast<uint32_t>(culledBatch.CulledGeometryIndices.size()));
			}
		}

		commandBuffer->SetGlobalDescriptorSet(globalResources.FrameResources[frameIndex].CameraDescriptorSet, 0);
	}
}
