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

		for (uint32_t frameIndex = 0; frameIndex < frameInFlightCount; frameIndex++)
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

		const SpotLightSubmition& spotLight = sceneSubmition.SpotLights[sceneSubmition.ShadowCastingSpotLights[0]];

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
		view.ViewportSize = shadowMapSize;
		view.SetViewAndProjection(projection, viewMatrix);

		uint32_t frameIndex = GraphicsContext::GetInstance().GetCurrentFrameInFlight();
		m_FrameResources[frameIndex].CameraBuffer->SetData(MemorySpan(&view, 1), 0);

		const ViewportGlobalResources& globalResources = context.RenderWorld.GetEntityComponent<const ViewportGlobalResources>(context.ViewportEntity);
		globalResources.FrameResources[frameIndex].SpotLightShadowDataBuffer->SetData(MemorySpan(&view.ViewProjection, 1), 0);
	}

	void SpotLightShadowPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		uint32_t frameIndex = GraphicsContext::GetInstance().GetCurrentFrameInFlight();
		const FrameResources& frameResources = m_FrameResources[frameIndex];
		const ViewportGlobalResources& globalResources = context.RenderWorld.GetEntityComponent<const ViewportGlobalResources>(context.ViewportEntity);
		const CulledGeometry& culledGeometry = context.RenderWorld.GetEntityComponent<const CulledGeometry>(context.ViewportEntity);
		const CulledGeometry::GPUFrameResources& culledGeometryResources = culledGeometry.FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];

		commandBuffer->SetGlobalDescriptorSet(frameResources.CameraDescriptorSet, 0);
		commandBuffer->SetGlobalDescriptorSet(culledGeometryResources.InstanceBufferDescriptor, 2);

		commandBuffer->SetDefaultViewportAndScissors();

		commandBuffer->ApplyMaterial(m_PerspectiveDepthOnly);

		for (const auto& culledBatch : culledGeometry.CulledBatches)
		{
			const Ref<const Mesh>& mesh = culledBatch.OriginalBatch->GetMesh();

			commandBuffer->DrawMeshIndexed(mesh,
				static_cast<uint32_t>(culledBatch.SubMeshIndex),
				static_cast<uint32_t>(culledBatch.TransformBufferOffset),
				static_cast<uint32_t>(culledBatch.CulledGeometryIndices.size()));
		}

		commandBuffer->SetGlobalDescriptorSet(globalResources.FrameResources[frameIndex].CameraDescriptorSet, 0);
	}
}
